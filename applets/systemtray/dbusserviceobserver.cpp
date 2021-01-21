/***************************************************************************
 *   Copyright (C) 2015 Marco Martin <mart@kde.org>                        *
 *   Copyright (C) 2020 Konrad Materka <materka@gmail.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "dbusserviceobserver.h"
#include "debug.h"

#include "systemtraysettings.h"

#include <KPluginMetaData>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>

DBusServiceObserver::DBusServiceObserver(QPointer<SystemTraySettings> settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_sessionServiceWatcher(new QDBusServiceWatcher(this))
    , m_systemServiceWatcher(new QDBusServiceWatcher(this))
{
    m_sessionServiceWatcher->setConnection(QDBusConnection::sessionBus());
    m_systemServiceWatcher->setConnection(QDBusConnection::systemBus());

    connect(m_settings, &SystemTraySettings::enabledPluginsChanged, this, &DBusServiceObserver::initDBusActivatables);

    // Watch for new services
    connect(m_sessionServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &serviceName) {
        if (!m_dbusSessionServiceNamesFetched) {
            return;
        }
        serviceRegistered(serviceName);
    });
    connect(m_sessionServiceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName) {
        if (!m_dbusSessionServiceNamesFetched) {
            return;
        }
        serviceUnregistered(serviceName);
    });
    connect(m_systemServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &serviceName) {
        if (!m_dbusSystemServiceNamesFetched) {
            return;
        }
        serviceRegistered(serviceName);
    });
    connect(m_systemServiceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName) {
        if (!m_dbusSystemServiceNamesFetched) {
            return;
        }
        serviceUnregistered(serviceName);
    });
}

void DBusServiceObserver::registerPlugin(const KPluginMetaData &pluginMetaData)
{
    const QString dbusactivation = pluginMetaData.value(QStringLiteral("X-Plasma-DBusActivationService"));
    if (!dbusactivation.isEmpty()) {
        qCDebug(SYSTEM_TRAY) << "Found DBus-able Applet: " << pluginMetaData.pluginId() << dbusactivation;
        QRegExp rx(dbusactivation);
        rx.setPatternSyntax(QRegExp::Wildcard);
        m_dbusActivatableTasks[pluginMetaData.pluginId()] = rx;

        const QString watchedService = QString(dbusactivation).replace(".*", "*");
        m_sessionServiceWatcher->addWatchedService(watchedService);
        m_systemServiceWatcher->addWatchedService(watchedService);
    }
}

void DBusServiceObserver::unregisterPlugin(const QString &pluginId)
{
    if (m_dbusActivatableTasks.contains(pluginId)) {
        QRegExp rx = m_dbusActivatableTasks.take(pluginId);
        const QString watchedService = rx.pattern().replace(".*", "*");
        m_sessionServiceWatcher->removeWatchedService(watchedService);
        m_systemServiceWatcher->removeWatchedService(watchedService);
    }
}

bool DBusServiceObserver::isDBusActivable(const QString &pluginId)
{
    return m_dbusActivatableTasks.contains(pluginId);
}

/* Loading and unloading Plasmoids when dbus services come and go
 *
 * This works as follows:
 * - we collect a list of plugins and related services in m_dbusActivatableTasks
 * - we query DBus for the list of services, async (initDBusActivatables())
 * - we go over that list, adding tasks when a service and plugin match (serviceNameFetchFinished())
 * - we start watching for new services, and do the same (serviceNameFetchFinished())
 * - whenever a service is gone, we check whether to unload a Plasmoid (serviceUnregistered())
 *
 * Order of events has to be:
 * - create a match rule for new service on DBus daemon
 * - start fetching a list of names
 * - ignore all changes that happen in the meantime
 * - handle the list of all names
 */
void DBusServiceObserver::initDBusActivatables()
{
    // fetch list of existing services
    QDBusPendingCall async = QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, [=](QDBusPendingCallWatcher *callWatcher) {
        serviceNameFetchFinished(callWatcher);
        m_dbusSessionServiceNamesFetched = true;
    });

    QDBusPendingCall systemAsync = QDBusConnection::systemBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *systemCallWatcher = new QDBusPendingCallWatcher(systemAsync, this);
    connect(systemCallWatcher, &QDBusPendingCallWatcher::finished, [=](QDBusPendingCallWatcher *callWatcher) {
        serviceNameFetchFinished(callWatcher);
        m_dbusSystemServiceNamesFetched = true;
    });
}

void DBusServiceObserver::serviceNameFetchFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> propsReply = *watcher;
    watcher->deleteLater();

    if (propsReply.isError()) {
        qCWarning(SYSTEM_TRAY) << "Could not get list of available D-Bus services";
    } else {
        const auto propsReplyValue = propsReply.value();
        for (const QString &serviceName : propsReplyValue) {
            serviceRegistered(serviceName);
        }
    }
}

void DBusServiceObserver::serviceRegistered(const QString &service)
{
    if (service.startsWith(QLatin1Char(':'))) {
        return;
    }

    for (auto it = m_dbusActivatableTasks.constBegin(), end = m_dbusActivatableTasks.constEnd(); it != end; ++it) {
        const QString &plugin = it.key();
        if (!m_settings->isEnabledPlugin(plugin)) {
            continue;
        }

        const auto &rx = it.value();
        if (rx.exactMatch(service)) {
            qCDebug(SYSTEM_TRAY) << "DBus service" << service << "matching" << m_dbusActivatableTasks[plugin] << "appeared. Loading" << plugin;
            emit serviceStarted(plugin);
            m_dbusServiceCounts[plugin]++;
        }
    }
}

void DBusServiceObserver::serviceUnregistered(const QString &service)
{
    for (auto it = m_dbusActivatableTasks.constBegin(), end = m_dbusActivatableTasks.constEnd(); it != end; ++it) {
        const QString &plugin = it.key();
        if (!m_settings->isEnabledPlugin(plugin)) {
            continue;
        }

        const auto &rx = it.value();
        if (rx.exactMatch(service)) {
            m_dbusServiceCounts[plugin]--;
            Q_ASSERT(m_dbusServiceCounts[plugin] >= 0);
            if (m_dbusServiceCounts[plugin] == 0) {
                qCDebug(SYSTEM_TRAY) << "DBus service" << service << "matching" << m_dbusActivatableTasks[plugin] << "disappeared. Unloading" << plugin;
                emit serviceStopped(plugin);
            }
        }
    }
}
