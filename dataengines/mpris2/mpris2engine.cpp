/*
    SPDX-FileCopyrightText: 2007-2012 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "mpris2engine.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QStringList>

#include "debug.h"
#include "multiplexedservice.h"
#include "multiplexer.h"
#include "playercontainer.h"
#include "playercontrol.h"

Mpris2Engine::Mpris2Engine(QObject *parent, const QVariantList &args)
    : Plasma5Support::DataEngine(parent, args)
{
    auto watcher =
        new QDBusServiceWatcher(QStringLiteral("org.mpris.MediaPlayer2*"), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &Mpris2Engine::serviceOwnerChanged);

    QDBusPendingCall async = QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this, &Mpris2Engine::serviceNameFetchFinished);
}

Plasma5Support::Service *Mpris2Engine::serviceForSource(const QString &source)
{
    if (source == Multiplexer::sourceName) {
        if (!m_multiplexer) {
            createMultiplexer();
        }
        return new MultiplexedService(m_multiplexer.data(), this);
    } else {
        PlayerContainer *container = qobject_cast<PlayerContainer *>(containerForSource(source));
        if (container) {
            return new PlayerControl(container, this);
        } else {
            return DataEngine::serviceForSource(source);
        }
    }
}

QStringList Mpris2Engine::sources() const
{
    if (m_multiplexer)
        return DataEngine::sources();
    else
        return DataEngine::sources() << Multiplexer::sourceName;
}

void Mpris2Engine::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    if (!serviceName.startsWith(QLatin1String("org.mpris.MediaPlayer2.")))
        return;

    QString sourceName = serviceName.mid(23);

    if (!oldOwner.isEmpty()) {
        qCDebug(MPRIS2) << "MPRIS service" << serviceName << "just went offline";
        if (m_multiplexer) {
            m_multiplexer.data()->removePlayer(sourceName);
        }
        removeSource(sourceName);
    }

    if (!newOwner.isEmpty()) {
        qCDebug(MPRIS2) << "MPRIS service" << serviceName << "just came online";
        addMediaPlayer(serviceName, sourceName);
    }
}

bool Mpris2Engine::updateSourceEvent(const QString &source)
{
    if (source == Multiplexer::sourceName) {
        return false;
    } else {
        PlayerContainer *container = qobject_cast<PlayerContainer *>(containerForSource(source));
        if (container) {
            container->refresh();
            return true;
        } else {
            return false;
        }
    }
}

bool Mpris2Engine::sourceRequestEvent(const QString &source)
{
    if (source == Multiplexer::sourceName) {
        createMultiplexer();
        return true;
    }
    return false;
}

void Mpris2Engine::initialFetchFinished(PlayerContainer *container)
{
    qCDebug(MPRIS2) << "Props fetch for" << container->objectName() << "finished; adding";

    // don't let future refreshes trigger this
    disconnect(container, &PlayerContainer::initialFetchFinished, this, &Mpris2Engine::initialFetchFinished);
    disconnect(container, &PlayerContainer::initialFetchFailed, this, &Mpris2Engine::initialFetchFailed);

    // Check if the player follows the specification dutifully.
    const auto data = container->data();
    if (data.value(QStringLiteral("Identity")).toString().isEmpty()) {
        qCDebug(MPRIS2) << "MPRIS2 service" << container->objectName() << "isn't standard-compliant, ignoring";
        return;
    }

    if (!data.value(QStringLiteral("SupportedUriSchemes")).isValid() || !data.value(QStringLiteral("SupportedMimeTypes")).isValid()) {
        // BUG 466288: BlueZ mpris-proxy doesn't provide the two properties
        qCDebug(MPRIS2) << "MPRIS2 service" << container->objectName() << "does not provide valid SupportedUriSchemes or SupportedMimeTypes.";
    }

    addSource(container);
    if (m_multiplexer) {
        m_multiplexer.data()->addPlayer(container);
    }
}

void Mpris2Engine::initialFetchFailed(PlayerContainer *container)
{
    qCWarning(MPRIS2) << "Failed to find working MPRIS2 interface for" << container->dbusAddress();
    container->deleteLater();
}

void Mpris2Engine::serviceNameFetchFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> propsReply = *watcher;
    watcher->deleteLater();

    if (propsReply.isError()) {
        qCWarning(MPRIS2) << "Could not get list of available D-Bus services";
    } else {
        foreach (const QString &serviceName, propsReply.value()) {
            if (serviceName.startsWith(QLatin1String("org.mpris.MediaPlayer2."))) {
                qCDebug(MPRIS2) << "Found MPRIS2 service" << serviceName;
                // watch out for race conditions; the media player could
                // have appeared between starting the service watcher and
                // this call being dealt with
                // NB: _disappearing_ between sending this call and doing
                // this processing is fine
                QString sourceName = serviceName.mid(23);
                PlayerContainer *container = qobject_cast<PlayerContainer *>(containerForSource(sourceName));
                if (!container) {
                    qCDebug(MPRIS2) << "Haven't already seen" << serviceName;
                    addMediaPlayer(serviceName, sourceName);
                }
            }
        }
    }
}

void Mpris2Engine::addMediaPlayer(const QString &serviceName, const QString &sourceName)
{
    PlayerContainer *container = new PlayerContainer(serviceName, this);
    container->setObjectName(sourceName);
    connect(container, &PlayerContainer::initialFetchFinished, this, &Mpris2Engine::initialFetchFinished);
    connect(container, &PlayerContainer::initialFetchFailed, this, &Mpris2Engine::initialFetchFailed);
}

void Mpris2Engine::createMultiplexer()
{
    Q_ASSERT(!m_multiplexer);
    m_multiplexer = new Multiplexer(this);

    SourceDict dict = containerDict();
    SourceDict::const_iterator i = dict.constBegin();
    while (i != dict.constEnd()) {
        PlayerContainer *container = qobject_cast<PlayerContainer *>(i.value());
        m_multiplexer.data()->addPlayer(container);
        ++i;
    }
    addSource(m_multiplexer.data());
    // Don't delete sourceName because currentData refers to it
    connect(m_multiplexer, &Multiplexer::playerListEmptied, m_multiplexer, &Multiplexer::deleteLater, Qt::UniqueConnection);
}

K_PLUGIN_CLASS_WITH_JSON(Mpris2Engine, "plasma-dataengine-mpris2.json")

#include "mpris2engine.moc"
