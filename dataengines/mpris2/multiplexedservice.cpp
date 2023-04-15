/*
    SPDX-FileCopyrightText: 2012 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "multiplexedservice.h"

#include "multiplexer.h"
#include <mprisplayer.h>

#include <KActionCollection>
#include <KGlobalAccel>
#include <KLocalizedString>

MultiplexedService::MultiplexedService(Multiplexer *multiplexer, QObject *parent)
    : Plasma::Service(parent)
{
    setObjectName(Multiplexer::sourceName + QLatin1String(" controller"));
    setName(QStringLiteral("mpris2"));
    setDestination(Multiplexer::sourceName);

    connect(multiplexer, &Multiplexer::activePlayerChanged, this, &MultiplexedService::activePlayerChanged);

    activePlayerChanged(multiplexer->activePlayer());
}

Plasma::ServiceJob *MultiplexedService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    if (m_control) {
        return m_control.data()->createJob(operation, parameters);
    }
    return nullptr;
}

void MultiplexedService::updateEnabledOperations()
{
    if (m_control) {
        foreach (const QString &op, operationNames()) {
            setOperationEnabled(op, m_control.data()->isOperationEnabled(op));
        }
    } else {
        foreach (const QString &op, operationNames()) {
            setOperationEnabled(op, false);
        }
    }
}

void MultiplexedService::activePlayerChanged(PlayerContainer *container)
{
    if (m_control && m_control->container() == container) {
        return;
    }

    delete m_control.data();

    if (container) {
        m_control = new PlayerControl(container, container->getDataEngine());
        connect(m_control.data(), &PlayerControl::enabledOperationsChanged, this, &MultiplexedService::updateEnabledOperations);
    }

    updateEnabledOperations();
}

void MultiplexedService::enableGlobalShortcuts()
{
    if (m_actionCollection) {
        return;
    }

    m_actionCollection = new KActionCollection(this, QStringLiteral("mediacontrol"));
    m_actionCollection->setComponentDisplayName(i18nc("Name for global shortcuts category", "Media Controller"));
    QAction *playPauseAction = m_actionCollection->addAction(QStringLiteral("playpausemedia"));
    playPauseAction->setText(i18n("Play/Pause media playback"));
    KGlobalAccel::setGlobalShortcut(playPauseAction, Qt::Key_MediaPlay);
    connect(playPauseAction, &QAction::triggered, this, [this] {
        if (m_control && m_control->capabilities() & PlayerContainer::CanControl) {
            const QString playbackStatus = m_control->rawData().value(QStringLiteral("PlaybackStatus")).toString();
            if (playbackStatus == QLatin1String("Playing")) {
                if (m_control->capabilities() & PlayerContainer::CanPause) {
                    m_control->playerInterface()->Pause();
                }
            } else {
                if (m_control->capabilities() & PlayerContainer::CanPlay) {
                    m_control->playerInterface()->Play();
                }
            }
        }
    });

    QAction *nextAction = m_actionCollection->addAction(QStringLiteral("nextmedia"));
    nextAction->setText(i18n("Media playback next"));
    KGlobalAccel::setGlobalShortcut(nextAction, Qt::Key_MediaNext);
    connect(nextAction, &QAction::triggered, this, [this] {
        if (m_control && m_control->capabilities() & (PlayerContainer::CanControl | PlayerContainer::CanGoNext)) {
            m_control->playerInterface()->Next();
        }
    });

    QAction *previousAction = m_actionCollection->addAction(QStringLiteral("previousmedia"));
    previousAction->setText(i18n("Media playback previous"));
    KGlobalAccel::setGlobalShortcut(previousAction, Qt::Key_MediaPrevious);
    connect(previousAction, &QAction::triggered, this, [this] {
        if (m_control && m_control->capabilities() & (PlayerContainer::CanControl | PlayerContainer::CanGoPrevious)) {
            m_control->playerInterface()->Previous();
        }
    });

    QAction *stopAction = m_actionCollection->addAction(QStringLiteral("stopmedia"));
    stopAction->setText(i18n("Stop media playback"));
    KGlobalAccel::setGlobalShortcut(stopAction, Qt::Key_MediaStop);
    connect(stopAction, &QAction::triggered, this, [this] {
        if (m_control && m_control->capabilities() & PlayerContainer::CanStop) {
            m_control->playerInterface()->Stop();
        }
    });

    QAction *pauseAction = m_actionCollection->addAction(QStringLiteral("pausemedia"));
    pauseAction->setText(i18n("Pause media playback"));
    KGlobalAccel::setGlobalShortcut(pauseAction, Qt::Key_MediaPause);
    connect(pauseAction, &QAction::triggered, this, [this] {
        if (m_control && m_control->capabilities() & PlayerContainer::CanPause) {
            m_control->playerInterface()->Pause();
        }
    });

    QAction *playAction = m_actionCollection->addAction(QStringLiteral("playmedia"));
    playAction->setText(i18n("Play media playback"));
    KGlobalAccel::setGlobalShortcut(playAction, QKeySequence());
    connect(playAction, &QAction::triggered, this, [this] {
        if (m_control && m_control->capabilities() & PlayerContainer::CanPlay) {
            m_control->playerInterface()->Play();
        }
    });

    QAction *volumeupAction = m_actionCollection->addAction(QStringLiteral("mediavolumeup"));
    volumeupAction->setText(i18n("Media volume up"));
    KGlobalAccel::setGlobalShortcut(volumeupAction, QKeySequence());
    connect(volumeupAction, &QAction::triggered, this, [this] {
        if (m_control && m_control->capabilities() & PlayerContainer::CanControl) {
            m_control->changeVolume(0.05, true);
        }
    });

    QAction *volumedownAction = m_actionCollection->addAction(QStringLiteral("mediavolumedown"));
    volumedownAction->setText(i18n("Media volume down"));
    KGlobalAccel::setGlobalShortcut(volumedownAction, QKeySequence());
    connect(volumedownAction, &QAction::triggered, this, [this] {
        if (m_control && m_control->playerInterface()->canControl()) {
            m_control->changeVolume(-0.05, true);
        }
    });
}
