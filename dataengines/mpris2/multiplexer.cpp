/*
 * Copyright 2012  Alex Merry <alex.merry@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "multiplexer.h"
#include <mprisplayer.h>

#include <KActionCollection>
#include <KGlobalAccel>
#include <KLocalizedString>

#include <QDebug> // for Q_ASSERT
#include <QAction>


// the '@' at the start is not valid for D-Bus names, so it will
// never interfere with an actual MPRIS2 player
const QLatin1String Multiplexer::sourceName = QLatin1String("@multiplex");

Multiplexer::Multiplexer(QObject* parent)
    : DataContainer(parent)
{
    setObjectName(sourceName);

    // setup actions
    KActionCollection *actionCollection = new KActionCollection(this, QStringLiteral("mediacontrol"));
    actionCollection->setComponentDisplayName(i18nc("Name for global shortcuts category", "Media Controller"));
    QAction *playPauseAction = actionCollection->addAction(QStringLiteral("playpausemedia"));
    playPauseAction->setText(i18n("Play/Pause media playback"));
    KGlobalAccel::setGlobalShortcut(playPauseAction, Qt::Key_MediaPlay);
    connect(playPauseAction, &QAction::triggered, this,
        [this] {
            if (PlayerContainer *active = activePlayer()) {
                active->playerInterface()->PlayPause();
            }
        }
    );

    QAction *nextAction = actionCollection->addAction(QStringLiteral("nextmedia"));
    nextAction->setText(i18n("Media playback next"));
    KGlobalAccel::setGlobalShortcut(nextAction, Qt::Key_MediaNext);
    connect(nextAction, &QAction::triggered, this,
        [this] {
            if (PlayerContainer *active = activePlayer()) {
                active->playerInterface()->Next();
            }
        }
    );

    QAction *previousAction = actionCollection->addAction(QStringLiteral("previousmedia"));
    previousAction->setText(i18n("Media playback previous"));
    KGlobalAccel::setGlobalShortcut(previousAction, Qt::Key_MediaPrevious);
    connect(previousAction, &QAction::triggered, this,
        [this] {
            if (PlayerContainer *active = activePlayer()) {
                active->playerInterface()->Previous();
            }
        }
    );

    QAction *stopAction = actionCollection->addAction(QStringLiteral("stopmedia"));
    stopAction->setText(i18n("Stop media playback"));
    KGlobalAccel::setGlobalShortcut(stopAction, Qt::Key_MediaStop);
    connect(stopAction, &QAction::triggered, this,
        [this] {
            if (PlayerContainer *active = activePlayer()) {
                active->playerInterface()->Stop();
            }
        }
    );
}

void Multiplexer::addPlayer(PlayerContainer *container)
{
    bool makeActive = m_activeName.isEmpty();

    if (container->data().value(QStringLiteral("PlaybackStatus")) == QLatin1String("Playing")) {
        m_playing.insert(container->objectName(), container);
        if (!makeActive &&
                data().value(QStringLiteral("PlaybackStatus")) != QLatin1String("Playing")) {
            makeActive = true;
        }
    } else if (container->data().value(QStringLiteral("PlaybackStatus")) == QLatin1String("Paused")) {
        m_paused.insert(container->objectName(), container);
        if (!makeActive &&
                data().value(QStringLiteral("PlaybackStatus")) != QLatin1String("Playing") &&
                data().value(QStringLiteral("PlaybackStatus")) != QLatin1String("Paused")) {
            makeActive = true;
        }
    } else {
        m_stopped.insert(container->objectName(), container);
    }

    connect(container, &Plasma::DataContainer::dataUpdated,
            this,      &Multiplexer::playerUpdated);

    if (makeActive) {
        m_activeName = container->objectName();
        replaceData(container->data());
        checkForUpdate();
        emit activePlayerChanged(container);
    }
}

void Multiplexer::removePlayer(const QString &name)
{
    PlayerContainer *container = m_playing.take(name);
    if (!container)
        container = m_paused.take(name);
    if (!container)
        container = m_stopped.take(name);
    if (container)
        container->disconnect(this);

    if (name == m_activeName) {
        setBestActive();
    }
}

PlayerContainer *Multiplexer::activePlayer() const
{
    if (m_activeName.isEmpty()) {
        return 0;
    }

    PlayerContainer *container = m_playing.value(m_activeName);
    if (!container)
        container = m_paused.value(m_activeName);
    if (!container)
        container = m_stopped.value(m_activeName);
    Q_ASSERT(container);
    return container;
}

void Multiplexer::playerUpdated(const QString &name, const Plasma::DataEngine::Data &newData)
{
    if (newData.value(QStringLiteral("PlaybackStatus")) == QLatin1String("Playing")) {
        if (!m_playing.contains(name)) {
            PlayerContainer *container = m_paused.take(name);
            if (!container) {
                container = m_stopped.take(name);
            }
            Q_ASSERT(container);
            m_playing.insert(name, container);
        }
        if (m_activeName != name &&
                data().value(QStringLiteral("PlaybackStatus")) != QLatin1String("Playing")) {
            m_activeName = name;
            replaceData(newData);
            checkForUpdate();
            emit activePlayerChanged(activePlayer());
            return;
        }
    } else if (newData.value(QStringLiteral("PlaybackStatus")) == QLatin1String("Paused")) {
        if (!m_paused.contains(name)) {
            PlayerContainer *container = m_playing.take(name);
            if (!container) {
                container = m_stopped.take(name);
            }
            Q_ASSERT(container);
            m_paused.insert(name, container);
        }
        if (m_activeName != name &&
                data().value(QStringLiteral("PlaybackStatus")) != QLatin1String("Playing") &&
                data().value(QStringLiteral("PlaybackStatus")) != QLatin1String("Paused")) {
            m_activeName = name;
            replaceData(newData);
            checkForUpdate();
            emit activePlayerChanged(activePlayer());
            return;
        }
    } else {
        if (!m_stopped.contains(name)) {
            PlayerContainer *container = m_playing.take(name);
            if (!container) {
                container = m_paused.take(name);
            }
            Q_ASSERT(container);
            m_stopped.insert(name, container);
        }
    }

    if (m_activeName == name) {
        bool isPaused = newData.value(QStringLiteral("PlaybackStatus")) == QLatin1String("Paused");
        bool isStopped = !isPaused && newData.value(QStringLiteral("PlaybackStatus")) != QLatin1String("Playing");
        if (isPaused && !m_playing.isEmpty()) {
            setBestActive();
        } else if (isStopped && (!m_playing.isEmpty() || !m_paused.isEmpty())) {
            setBestActive();
        } else {
            replaceData(newData);
            checkForUpdate();
        }
    }
}

void Multiplexer::setBestActive()
{
    QHash<QString,PlayerContainer*>::const_iterator it = m_playing.constBegin();
    if (it != m_playing.constEnd()) {
        m_activeName = it.key();
        replaceData(it.value()->data());
        emit activePlayerChanged(it.value());
    } else {
        it = m_paused.constBegin();
        if (it != m_paused.constEnd()) {
            m_activeName = it.key();
            replaceData(it.value()->data());
            emit activePlayerChanged(it.value());
        } else {
            it = m_stopped.constBegin();
            if (it != m_stopped.constEnd()) {
                m_activeName = it.key();
                replaceData(it.value()->data());
                emit activePlayerChanged(it.value());
            } else {
                m_activeName = QString();
                removeAllData();
                emit activePlayerChanged(0);
            }
        }
    }
    checkForUpdate();
}

void Multiplexer::replaceData(const Plasma::DataEngine::Data &data)
{
    removeAllData();

    Plasma::DataEngine::Data::const_iterator it = data.constBegin();
    while (it != data.constEnd()) {
        setData(it.key(), it.value());
        ++it;
    }
    setData(QStringLiteral("Source Name"), m_activeName);
}

