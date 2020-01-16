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

#include <KLocalizedString>

#include <QDebug> // for Q_ASSERT
#include <QAction>

#include <algorithm>

#include "debug.h"

// the '@' at the start is not valid for D-Bus names, so it will
// never interfere with an actual MPRIS2 player
const QLatin1String Multiplexer::sourceName = QLatin1String("@multiplex");

Multiplexer::Multiplexer(QObject* parent)
    : DataContainer(parent)
{
    setObjectName(sourceName);
}

void Multiplexer::evaluatePlayer(PlayerContainer *container)
{
    bool makeActive = m_activeName.isEmpty();

    QString name = container->objectName();
    const QString containerPlaybackStatus = container->data().value(QStringLiteral("PlaybackStatus")).toString();
    const QString multiplexerPlaybackStatus = data().value(QStringLiteral("PlaybackStatus")).toString();

    m_playing.remove(name);
    m_paused.remove(name);
    m_stopped.remove(name);

    // Ensure the actual player is always in the correct category
    if (containerPlaybackStatus == QLatin1String("Playing")) {
        m_playing.insert(name, container);
    } else if (containerPlaybackStatus == QLatin1String("Paused")) {
        m_paused.insert(name, container);
    } else {
        m_stopped.insert(name, container);
    }

    const auto proxyPid = container->data().value(QStringLiteral("Metadata")).toMap().value(QStringLiteral("kde:pid")).toUInt();
    if (proxyPid) {
        auto it = m_proxies.find(proxyPid);
        if (it == m_proxies.end()) {
            m_proxies.insert(proxyPid, container);
        }
    }

    const auto containerPid = container->data().value(QStringLiteral("InstancePid")).toUInt();
    PlayerContainer *proxy = m_proxies.value(containerPid);
    if (proxy) {
        // Operate on the proxy from now on
        container = proxy;
        name = container->objectName();
    }

    if (!makeActive) {
        // If this player has higher status than the current multiplexer player, switch over to it
        if (m_playing.value(name) && multiplexerPlaybackStatus != QLatin1String("Playing")) {
            qCDebug(MPRIS2) << "Player" << name << "is now playing but current was not";
            makeActive = true;
        } else if (m_paused.value(name)
           && multiplexerPlaybackStatus != QLatin1String("Playing")
           && multiplexerPlaybackStatus != QLatin1String("Paused")) {
            qCDebug(MPRIS2) << "Player" << name << "is now paused but current was stopped";
            makeActive = true;
        }
    }

    if (m_activeName == name) {
        // If we are the current player and move to a lower status, switch to another one, if necessary
        if (m_paused.value(name) && !m_playing.isEmpty()) {
            qCDebug(MPRIS2) << "Current player" << m_activeName << "is now paused but there is another playing one, switching players";
            setBestActive();
            makeActive = false;
        } else if (m_stopped.value(name) && (!m_playing.isEmpty() || !m_paused.isEmpty())) {
            qCDebug(MPRIS2) << "Current player" << m_activeName << "is now stopped but there is another playing or paused one, switching players";
            setBestActive();
            makeActive = false;
        } else {
            makeActive = true;
        }
    }

    if (makeActive) {
        if (m_activeName != name) {
            qCDebug(MPRIS2) << "Switching from" << m_activeName << "to" << name;
            m_activeName = name;
        }
        replaceData(container->data());
        checkForUpdate();
        emit activePlayerChanged(container);
    }
}

void Multiplexer::addPlayer(PlayerContainer *container)
{
    evaluatePlayer(container);

    connect(container, &Plasma::DataContainer::dataUpdated,
            this,      &Multiplexer::playerUpdated);
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

    // Remove proxy by value (container), not key (pid), which could have changed
    const auto pid = m_proxies.key(container);
    if (pid) {
        m_proxies.remove(pid);
    }

    if (name == m_activeName) {
        setBestActive();
    }
}

PlayerContainer *Multiplexer::activePlayer() const
{
    if (m_activeName.isEmpty()) {
        return nullptr;
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
    Q_UNUSED(name);
    Q_UNUSED(newData);
    evaluatePlayer(qobject_cast<PlayerContainer *>(sender()));
}

PlayerContainer *Multiplexer::firstPlayerFromHash(const QHash<QString, PlayerContainer *> &hash, PlayerContainer **proxyCandidate) const
{
    if (proxyCandidate) {
        *proxyCandidate = nullptr;
    }

    auto it = hash.begin();
    if (it == hash.end()) {
        return nullptr;
    }

    PlayerContainer *container = it.value();
    const auto containerPid = container->data().value(QStringLiteral("InstancePid")).toUInt();

    // Check if this player is being proxied by someone else and prefer the proxy
    // but only if it is in the same hash (same state)
    if (PlayerContainer *proxy = m_proxies.value(containerPid)) {
        if (std::find(hash.begin(), hash.end(), proxy) == hash.end()) {
            if (proxyCandidate) {
                *proxyCandidate = proxy;
            }
            return nullptr;
            //continue;
        }
        return proxy;
    }

    return container;
}

void Multiplexer::setBestActive()
{
    qCDebug(MPRIS2) << "Activating best player";
    PlayerContainer *proxyCandidate = nullptr;

    PlayerContainer *container = firstPlayerFromHash(m_playing, &proxyCandidate);
    if (!container) {
        // If we found a proxy earlier, prefer it over a random other player in that category
        if (proxyCandidate && std::find(m_paused.constBegin(), m_paused.constEnd(), proxyCandidate) != m_paused.constEnd()) {
            container = proxyCandidate;
        } else {
            container = firstPlayerFromHash(m_paused, &proxyCandidate);
        }
    }
    if (!container) {
        if (proxyCandidate && std::find(m_stopped.constBegin(), m_stopped.constEnd(), proxyCandidate) != m_stopped.constEnd()) {
            container = proxyCandidate;
        } else {
            container = firstPlayerFromHash(m_stopped, &proxyCandidate);
        }
    }

    if (!container) {
        qCDebug(MPRIS2) << "There is currently no player";
        m_activeName.clear();
        removeAllData();
    } else {
        m_activeName = container->objectName();
        qCDebug(MPRIS2) << "Determined" << m_activeName << "to be the best player";
        replaceData(container->data());
        checkForUpdate();
    }

    emit activePlayerChanged(container);
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

