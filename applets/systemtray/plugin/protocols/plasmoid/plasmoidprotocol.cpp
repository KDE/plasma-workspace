/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
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

#include "plasmoidtask.h"
#include "plasmoidprotocol.h"

#include "debug.h"

#include "../../host.h"

#include <Plasma/PluginLoader>
#include <Plasma/Containment>
#include <kdeclarative/qmlobject.h>
#include <KLocalizedString>
#include <kplugintrader.h>

#include <QLoggingCategory>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDBusPendingCallWatcher>
#include <QRegExp>

namespace SystemTray
{

PlasmoidProtocol::PlasmoidProtocol(QObject *parent)
    : Protocol(parent),
      m_tasks(),
      m_containment(0),
      m_systrayApplet(0)
{
}

PlasmoidProtocol::~PlasmoidProtocol()
{
}

void PlasmoidProtocol::init()
{
    //this should never happen
    if (m_containment) {
        return;
    }

    Host* h = qobject_cast<Host*>(parent());
    QQuickItem* rootItem = h->rootItem();
    if (rootItem) {
        m_systrayApplet = rootItem->property("_plasma_applet").value<Plasma::Applet*>();
    }

    if (!m_systrayApplet) {
        qWarning() << "Don't have a parent applet, Can't initialize the Plasmoid protocol!!!";
        return;
    }

    int containmentId = 0;

    KConfigGroup cg = m_systrayApplet->config();
    cg = KConfigGroup(&cg, "Containments");
    if (cg.isValid() && cg.groupList().size()) {
        containmentId = cg.groupList().first().toInt();
    }

    m_containment = new Plasma::Containment(m_systrayApplet, "null", containmentId);
    m_containment->setImmutability(Plasma::Types::Mutable);
    m_containment->setFormFactor(Plasma::Types::Horizontal);
    m_containment->setLocation(m_systrayApplet->location());
    m_containment->setContainmentActions("RightButton;NoModifier", "org.kde.contextmenu");
    m_containment->init();

    connect(m_systrayApplet, &Plasma::Applet::locationChanged, [=]() {
        m_containment->setLocation(m_systrayApplet->location());
    });

    m_systrayApplet->setProperty("containment", QVariant::fromValue(m_containment));

    restorePlasmoids();
}

void PlasmoidProtocol::restorePlasmoids()
{
    //First: remove all that are not allowed anymore
    QStringList tasksToDelete;
    foreach (const QString &task, m_tasks.keys()) {
        if (!m_allowedPlugins.contains(task)) {
            tasksToDelete << task;
        }
    }
    foreach (const QString &task, tasksToDelete) {
        cleanupTask(task);
    }

    KConfigGroup cg = m_systrayApplet->config();
    cg = m_containment->config();
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
    const QString constraint = QString("[X-Plasma-NotificationArea] == true");

    KPluginInfo::List applets;
    for (auto info : Plasma::PluginLoader::self()->listAppletInfo(QString())) {
        if (info.isValid() && info.property("X-Plasma-NotificationArea") == "true") {
            applets << info;
        }
    }

    QStringList ownApplets;

    QMap<QString, KPluginInfo> sortedApplets;
    foreach (const KPluginInfo &info, applets) {
        const QString dbusactivation = info.property("X-Plasma-DBusActivationService").toString();
        if (!dbusactivation.isEmpty()) {
            qCDebug(SYSTEMTRAY) << "ST Found DBus-able Applet: " << info.pluginName() << dbusactivation;
            m_dbusActivatableTasks[info.pluginName()] = dbusactivation;
            continue;
        }

        if (m_allowedPlugins.contains(info.pluginName()) &&
            !m_tasks.contains(info.pluginName()) &&
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

QStringList PlasmoidProtocol::allowedPlugins() const
{
    return m_allowedPlugins;
}

void PlasmoidProtocol::setAllowedPlugins(const QStringList &allowed)
{
    m_allowedPlugins = allowed;

    if (m_containment) {
        restorePlasmoids();
    }
}

void PlasmoidProtocol::newTask(const QString &service)
{
    qCDebug(SYSTEMTRAY) << "ST new task " << service;
    //don't allow duplicates
    if (m_tasks.contains(service)) {
        return;
    }

    PlasmoidTask *task = new PlasmoidTask(service, m_knownPlugins.value(service), m_containment, this);

    if (task->pluginInfo().isValid()) {
        m_tasks[service] = task;
        emit taskCreated(task);
    } else {
        qWarning() << "Failed to load Plasmoid: " << service;
    }
}

void PlasmoidProtocol::cleanupTask(const QString &service)
{
    PlasmoidTask *task = m_tasks.value(service);

    if (task) {
        m_tasks.remove(service);
        if (task->isValid()) {
            emit task->destroyed(task);
        }
        task->deleteLater();
    }
}

void PlasmoidProtocol::initDBusActivatables()
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
    QDBusPendingCall async = QDBusConnection::sessionBus().interface()->asyncCall("ListNames");
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished,
            [=](QDBusPendingCallWatcher *callWatcher){
                PlasmoidProtocol::serviceNameFetchFinished(callWatcher, QDBusConnection::sessionBus());
            });

    QDBusPendingCall systemAsync = QDBusConnection::systemBus().interface()->asyncCall("ListNames");
    QDBusPendingCallWatcher *systemCallWatcher = new QDBusPendingCallWatcher(systemAsync, this);
    connect(systemCallWatcher, &QDBusPendingCallWatcher::finished,
            [=](QDBusPendingCallWatcher *callWatcher){
                PlasmoidProtocol::serviceNameFetchFinished(callWatcher, QDBusConnection::systemBus());
            });
}

void PlasmoidProtocol::serviceNameFetchFinished(QDBusPendingCallWatcher* watcher, const QDBusConnection &connection)
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
    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &PlasmoidProtocol::serviceRegistered);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &PlasmoidProtocol::serviceUnregistered);
}



void PlasmoidProtocol::serviceRegistered(const QString &service)
{
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

void PlasmoidProtocol::serviceUnregistered(const QString &service)
{
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


}


