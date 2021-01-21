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

#include "statusnotifieritem_engine.h"
#include "statusnotifieritemsource.h"
#include <QStringList>

#include "dbusproperties.h"

#include "debug.h"
#include <iostream>

static const QString s_watcherServiceName(QStringLiteral("org.kde.StatusNotifierWatcher"));

StatusNotifierItemEngine::StatusNotifierItemEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
    , m_statusNotifierWatcher(nullptr)
{
    Q_UNUSED(args);
    init();
}

StatusNotifierItemEngine::~StatusNotifierItemEngine()
{
    QDBusConnection::sessionBus().unregisterService(m_serviceName);
}

Plasma::Service *StatusNotifierItemEngine::serviceForSource(const QString &name)
{
    StatusNotifierItemSource *source = dynamic_cast<StatusNotifierItemSource *>(containerForSource(name));
    // if source does not exist, return null service
    if (!source) {
        return Plasma::DataEngine::serviceForSource(name);
    }

    Plasma::Service *service = source->createService();
    service->setParent(this);
    return service;
}

void StatusNotifierItemEngine::init()
{
    if (QDBusConnection::sessionBus().isConnected()) {
        m_serviceName = "org.kde.StatusNotifierHost-" + QString::number(QCoreApplication::applicationPid());
        QDBusConnection::sessionBus().registerService(m_serviceName);

        QDBusServiceWatcher *watcher =
            new QDBusServiceWatcher(s_watcherServiceName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
        connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &StatusNotifierItemEngine::serviceChange);

        registerWatcher(s_watcherServiceName);
    }
}

void StatusNotifierItemEngine::serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    qCDebug(DATAENGINE_SNI) << "Service" << name << "status change, old owner:" << oldOwner << "new:" << newOwner;

    if (newOwner.isEmpty()) {
        // unregistered
        unregisterWatcher(name);
    } else if (oldOwner.isEmpty()) {
        // registered
        registerWatcher(name);
    }
}

void StatusNotifierItemEngine::registerWatcher(const QString &service)
{
    // qCDebug(DATAENGINE_SNI)<<"service appeared"<<service;
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
                    newItem(service);
                }
            });

            connect(m_statusNotifierWatcher,
                    &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemRegistered,
                    this,
                    &StatusNotifierItemEngine::serviceRegistered);
            connect(m_statusNotifierWatcher,
                    &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemUnregistered,
                    this,
                    &StatusNotifierItemEngine::serviceUnregistered);

        } else {
            delete m_statusNotifierWatcher;
            m_statusNotifierWatcher = nullptr;
            qCDebug(DATAENGINE_SNI) << "System tray daemon not reachable";
        }
    }
}

void StatusNotifierItemEngine::unregisterWatcher(const QString &service)
{
    if (service == s_watcherServiceName) {
        qCDebug(DATAENGINE_SNI) << s_watcherServiceName << "disappeared";

        disconnect(m_statusNotifierWatcher,
                   &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemRegistered,
                   this,
                   &StatusNotifierItemEngine::serviceRegistered);
        disconnect(m_statusNotifierWatcher,
                   &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemUnregistered,
                   this,
                   &StatusNotifierItemEngine::serviceUnregistered);

        removeAllSources();

        delete m_statusNotifierWatcher;
        m_statusNotifierWatcher = nullptr;
    }
}

void StatusNotifierItemEngine::serviceRegistered(const QString &service)
{
    qCDebug(DATAENGINE_SNI) << "Registering" << service;
    newItem(service);
}

void StatusNotifierItemEngine::serviceUnregistered(const QString &service)
{
    removeSource(service);
}

void StatusNotifierItemEngine::newItem(const QString &service)
{
    StatusNotifierItemSource *itemSource = new StatusNotifierItemSource(service, this);
    addSource(itemSource);
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(statusnotifieritem, StatusNotifierItemEngine, "plasma-dataengine-statusnotifieritem.json")

#include "statusnotifieritem_engine.moc"
