/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
 *   Copyright (C) 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>       *
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

#include "statusnotifieritemhost.h"
#include "statusnotifieritemsource.h"
#include <QStringList>

#include "dbusproperties.h"

#include "debug.h"
#include <iostream>

class StatusNotifierItemHostSingleton
{
public:
    StatusNotifierItemHost self;
};

Q_GLOBAL_STATIC(StatusNotifierItemHostSingleton, privateStatusNotifierItemHostSelf)

static const QString s_watcherServiceName(QStringLiteral("org.kde.StatusNotifierWatcher"));

StatusNotifierItemHost::StatusNotifierItemHost()
    : QObject()
    , m_statusNotifierWatcher(nullptr)
{
    init();
}

StatusNotifierItemHost::~StatusNotifierItemHost()
{
    QDBusConnection::sessionBus().unregisterService(m_serviceName);
}

StatusNotifierItemHost *StatusNotifierItemHost::self()
{
    return &privateStatusNotifierItemHostSelf()->self;
}

const QList<QString> StatusNotifierItemHost::services() const
{
    return m_sniServices.keys();
}

StatusNotifierItemSource *StatusNotifierItemHost::itemForService(const QString service)
{
    return m_sniServices.value(service);
}

void StatusNotifierItemHost::init()
{
    if (QDBusConnection::sessionBus().isConnected()) {
        m_serviceName = "org.kde.StatusNotifierHost-" + QString::number(QCoreApplication::applicationPid());
        QDBusConnection::sessionBus().registerService(m_serviceName);

        QDBusServiceWatcher *watcher =
            new QDBusServiceWatcher(s_watcherServiceName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
        connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &StatusNotifierItemHost::serviceChange);

        registerWatcher(s_watcherServiceName);
    }
}

void StatusNotifierItemHost::serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    qCDebug(SYSTEM_TRAY) << "Service" << name << "status change, old owner:" << oldOwner << "new:" << newOwner;

    if (newOwner.isEmpty()) {
        // unregistered
        unregisterWatcher(name);
    } else if (oldOwner.isEmpty()) {
        // registered
        registerWatcher(name);
    }
}

void StatusNotifierItemHost::registerWatcher(const QString &service)
{
    if (service == s_watcherServiceName) {
        delete m_statusNotifierWatcher;

        m_statusNotifierWatcher =
            new org::kde::StatusNotifierWatcher(s_watcherServiceName, QStringLiteral("/StatusNotifierWatcher"), QDBusConnection::sessionBus());
        if (m_statusNotifierWatcher->isValid()) {
            m_statusNotifierWatcher->call(QDBus::NoBlock, QStringLiteral("RegisterStatusNotifierHost"), m_serviceName);

            OrgFreedesktopDBusPropertiesInterface propetriesIface(m_statusNotifierWatcher->service(),
                                                                  m_statusNotifierWatcher->path(),
                                                                  m_statusNotifierWatcher->connection());

            QDBusPendingReply<QDBusVariant> pendingItems = propetriesIface.Get(m_statusNotifierWatcher->interface(), "RegisteredStatusNotifierItems");

            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingItems, this);
            connect(watcher, &QDBusPendingCallWatcher::finished, this, [=]() {
                watcher->deleteLater();
                QDBusReply<QDBusVariant> reply = *watcher;
                QStringList registeredItems = reply.value().variant().toStringList();
                foreach (const QString &service, registeredItems) {
                    addSNIService(service);
                }
            });

            connect(m_statusNotifierWatcher,
                    &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemRegistered,
                    this,
                    &StatusNotifierItemHost::serviceRegistered);
            connect(m_statusNotifierWatcher,
                    &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemUnregistered,
                    this,
                    &StatusNotifierItemHost::serviceUnregistered);

        } else {
            delete m_statusNotifierWatcher;
            m_statusNotifierWatcher = nullptr;
            qCDebug(SYSTEM_TRAY) << "System tray daemon not reachable";
        }
    }
}

void StatusNotifierItemHost::unregisterWatcher(const QString &service)
{
    if (service == s_watcherServiceName) {
        qCDebug(SYSTEM_TRAY) << s_watcherServiceName << "disappeared";

        disconnect(m_statusNotifierWatcher,
                   &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemRegistered,
                   this,
                   &StatusNotifierItemHost::serviceRegistered);
        disconnect(m_statusNotifierWatcher,
                   &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemUnregistered,
                   this,
                   &StatusNotifierItemHost::serviceUnregistered);

        removeAllSNIServices();

        delete m_statusNotifierWatcher;
        m_statusNotifierWatcher = nullptr;
    }
}

void StatusNotifierItemHost::serviceRegistered(const QString &service)
{
    qCDebug(SYSTEM_TRAY) << "Registering" << service;
    addSNIService(service);
}

void StatusNotifierItemHost::serviceUnregistered(const QString &service)
{
    removeSNIService(service);
}

void StatusNotifierItemHost::removeAllSNIServices()
{
    QHashIterator<QString, StatusNotifierItemSource *> it(m_sniServices);
    while (it.hasNext()) {
        it.next();

        StatusNotifierItemSource *item = it.value();
        item->disconnect();
        item->deleteLater();
        Q_EMIT itemRemoved(it.key());
    }
    m_sniServices.clear();
}

void StatusNotifierItemHost::addSNIService(const QString &service)
{
    StatusNotifierItemSource *item = new StatusNotifierItemSource(service, this);
    m_sniServices.insert(service, item);
    Q_EMIT itemAdded(service);
}

void StatusNotifierItemHost::removeSNIService(const QString &service)
{
    if (m_sniServices.contains(service)) {
        auto item = m_sniServices.value(service);
        item->disconnect();
        item->deleteLater();
        m_sniServices.remove(service);
        Q_EMIT itemRemoved(service);
    }
}

