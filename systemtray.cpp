/***************************************************************************
 *   Copyright (C) 2015 Marco Martin <mart@kde.org>                        *
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

#include "systemtray.h"
#include "debug.h"

#include <QDebug>
#include <QProcess>

#include <Plasma/PluginLoader>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDBusPendingCallWatcher>
#include <QRegExp>

Q_LOGGING_CATEGORY(SYSTEMTRAY, "systemtray")

SystemTray::SystemTray(QObject *parent, const QVariantList &args)
    : Plasma::Containment(parent, args)
{
    setHasConfigurationInterface(true);
}

SystemTray::~SystemTray()
{
}

void SystemTray::init()
{
    Containment::init();

    //TODO: take this from config
    QStringList applets;
    for (auto info : Plasma::PluginLoader::self()->listAppletInfo(QString())) {
        if (info.isValid() && info.property(QStringLiteral("X-Plasma-NotificationArea")).toBool() == true) {
            applets << info.pluginName();
        }
    }

    //applets << "org.kde.plasma.battery";
    setAllowedPlugins(applets);
}

void SystemTray::newTask(const QString &task)
{
    foreach (Plasma::Applet *applet, applets()) {
        if (!applet->pluginInfo().isValid()) {
            continue;
        }
        if (task == applet->pluginInfo().pluginName()) {
            return;
        }
    }

    createApplet(task);
}

void SystemTray::cleanupTask(const QString &task)
{
    foreach (Plasma::Applet *applet, applets()) {
        if (!applet->pluginInfo().isValid() && task == applet->pluginInfo().pluginName()) {
            applet->destroy();
        }
    }
}

void SystemTray::restorePlasmoids()
{
    //First: remove all that are not allowed anymore
    QStringList tasksToDelete;
    foreach (Plasma::Applet *applet, applets()) {
        const QString task = applet->pluginInfo().pluginName();
        if (!m_allowedPlugins.contains(task)) {
            applet->destroy();
        }
    }

    KConfigGroup cg = config();
    cg = KConfigGroup(&cg, "Applets");
    foreach (const QString &group, cg.groupList()) {

        KConfigGroup appletConfig(&cg, group);
        QString plugin = appletConfig.readEntry("plugin");
        if (!plugin.isEmpty()) {
            m_knownPlugins[plugin] = group.toInt();
        }
    }
    qWarning() << "Known plasmoid ids:"<< m_knownPlugins;

    //X-Plasma-NotificationArea
    const QString constraint = QStringLiteral("[X-Plasma-NotificationArea] == true");

    KPluginInfo::List applets;
    for (auto info : Plasma::PluginLoader::self()->listAppletInfo(QString())) {
        if (info.isValid() && info.property(QStringLiteral("X-Plasma-NotificationArea")).toBool() == true) {
            applets << info;
        }
    }

    QStringList ownApplets;

    QMap<QString, KPluginInfo> sortedApplets;
    foreach (const KPluginInfo &info, applets) {
        const QString dbusactivation = info.property(QStringLiteral("X-Plasma-DBusActivationService")).toString();
        if (!dbusactivation.isEmpty()) {
            qCDebug(SYSTEMTRAY) << "ST Found DBus-able Applet: " << info.pluginName() << dbusactivation;
            m_dbusActivatableTasks[info.pluginName()] = dbusactivation;
            continue;
        }

        if (m_allowedPlugins.contains(info.pluginName()) &&
            //FIXME
            //!m_tasks.contains(info.pluginName()) &&
            dbusactivation.isEmpty()) {
            // if we already have a plugin with this exact name in it, then check if it is the
            // same plugin and skip it if it is indeed already listed
            if (sortedApplets.contains(info.name())) {

                bool dupe = false;
                // it is possible (though poor form) to have multiple applets
                // with the same visible name but different plugins, so we hve to check all values
                foreach (const KPluginInfo &existingInfo, sortedApplets.values(info.name())) {
                    if (existingInfo.pluginName() == info.pluginName()) {
                        dupe = true;
                        break;
                    }
                }

                if (dupe) {
                    continue;
                }
            }

            // insertMulti becase it is possible (though poor form) to have multiple applets
            // with the same visible name but different plugins
            sortedApplets.insertMulti(info.name(), info);
        }
    }

    foreach (const KPluginInfo &info, sortedApplets) {
        //qCDebug(SYSTEMTRAY) << " Adding applet: " << info.name();
        qCDebug(SYSTEMTRAY) << "\n\n ==========================================================================================";
        if (m_allowedPlugins.contains(info.pluginName())) {
            newTask(info.pluginName());
        }
    }

    initDBusActivatables();
}

QStringList SystemTray::allowedPlugins() const
{
    return m_allowedPlugins;
}

void SystemTray::setAllowedPlugins(const QStringList &allowed)
{
    m_allowedPlugins = allowed;

    restorePlasmoids();
}

void SystemTray::initDBusActivatables()
{
    /* Loading and unloading Plasmoids when dbus services come and go
     *
     * This works as follows:
     * - we collect a list of plugins and related services in m_dbusActivatableTasks
     * - we query DBus for the list of services, async (initDBusActivatables())
     * - we go over that list, adding tasks when a service and plugin match (serviceNameFetchFinished())
     * - we start watching for new services, and do the same (serviceNameFetchFinished())
     * - whenever a service is gone, we check whether to unload a Plasmoid (serviceUnregistered())
     */
    QDBusPendingCall async = QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished,
            [=](QDBusPendingCallWatcher *callWatcher){
                SystemTray::serviceNameFetchFinished(callWatcher, QDBusConnection::sessionBus());
            });

    QDBusPendingCall systemAsync = QDBusConnection::systemBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *systemCallWatcher = new QDBusPendingCallWatcher(systemAsync, this);
    connect(systemCallWatcher, &QDBusPendingCallWatcher::finished,
            [=](QDBusPendingCallWatcher *callWatcher){
                SystemTray::serviceNameFetchFinished(callWatcher, QDBusConnection::systemBus());
            });
}

void SystemTray::serviceNameFetchFinished(QDBusPendingCallWatcher* watcher, const QDBusConnection &connection)
{
    QDBusPendingReply<QStringList> propsReply = *watcher;
    watcher->deleteLater();

    if (propsReply.isError()) {
        qCWarning(SYSTEMTRAY) << "Could not get list of available D-Bus services";
    } else {
        foreach (const QString& serviceName, propsReply.value()) {
            serviceRegistered(serviceName);
        }
    }

    // Watch for new services
    // We need to watch for all of new services here, since we want to "match" the names,
    // not just compare them
    // This makes mpris work, since it wants to match org.mpris.MediaPlayer2.dragonplayer
    // against org.mpris.MediaPlayer2
    QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(QString(),
                                                connection,
                                                QDBusServiceWatcher::WatchForOwnerChange,
                                                this);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &SystemTray::serviceRegistered);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &SystemTray::serviceUnregistered);
}



void SystemTray::serviceRegistered(const QString &service)
{
    qDebug() << "DBus service appeared:" << service;
    foreach (const QString &plugin, m_dbusActivatableTasks.keys()) {
        if (!m_allowedPlugins.contains(plugin)) {
            continue;
        }
        const QString& pattern = m_dbusActivatableTasks.value(plugin);
        QRegExp rx(pattern);
        rx.setPatternSyntax(QRegExp::Wildcard);
        if (rx.exactMatch(service)) {
            qDebug() << "ST : DBus service " << m_dbusActivatableTasks[plugin] << "appeared. Loading " << plugin;
            newTask(plugin);
            m_dbusServiceCounts[plugin]++;
        }
    }
}

void SystemTray::serviceUnregistered(const QString &service)
{
    qDebug() << "DBus service disappeared:" << service;
    foreach (const QString &plugin, m_dbusActivatableTasks.keys()) {
        if (!m_allowedPlugins.contains(plugin)) {
            continue;
        }
        const QString& pattern = m_dbusActivatableTasks.value(plugin);
        QRegExp rx(pattern);
        rx.setPatternSyntax(QRegExp::Wildcard);
        if (rx.exactMatch(service)) {
            m_dbusServiceCounts[plugin]--;
            Q_ASSERT(m_dbusServiceCounts[plugin] >= 0);
            if (m_dbusServiceCounts[plugin] == 0) {
                qDebug() << "ST : DBus service " << m_dbusActivatableTasks[plugin] << " disappeared. Unloading " << plugin;
                cleanupTask(plugin);
            }
        }
    }
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(systemtray, SystemTray, "metadata.json")

#include "systemtray.moc"
