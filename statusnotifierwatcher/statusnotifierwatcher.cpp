/***************************************************************************
 *   Copyright 2009 by Marco Martin <notmart@gmail.com>                    *
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

#include "statusnotifierwatcher.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>

#include <kglobal.h>
#include <kaboutdata.h>
#include <kdebug.h>

#include <kpluginfactory.h>
#include <kpluginloader.h>
#include "statusnotifierwatcheradaptor.h"
#include "statusnotifieritem_interface.h"

K_PLUGIN_FACTORY(StatusNotifierWatcherFactory,
                 registerPlugin<StatusNotifierWatcher>();
    )

StatusNotifierWatcher::StatusNotifierWatcher(QObject *parent, const QList<QVariant>&)
      : KDEDModule(parent)
{
    setModuleName("StatusNotifierWatcher");
    new StatusNotifierWatcherAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerService("org.kde.StatusNotifierWatcher");
    dbus.registerObject("/StatusNotifierWatcher", this);

    m_serviceWatcher = new QDBusServiceWatcher(this);
    m_serviceWatcher->setConnection(dbus);
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    connect(m_serviceWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(serviceUnregistered(QString)));
}

StatusNotifierWatcher::~StatusNotifierWatcher()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService("org.kde.StatusNotifierWatcher");
}


void StatusNotifierWatcher::RegisterStatusNotifierItem(const QString &serviceOrPath)
{
    QString service;
    QString path;
    if (serviceOrPath.startsWith('/')) {
        service = message().service();
        path = serviceOrPath;
    } else {
        service = serviceOrPath;
        path = "/StatusNotifierItem";
    }
    QString notifierItemId = service + path;
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(service).value() &&
        !m_registeredServices.contains(notifierItemId)) {
        kDebug()<<"Registering" << notifierItemId << "to system tray";

        //check if the service has registered a SystemTray object
        org::kde::StatusNotifierItem trayclient(service, path,
                                                QDBusConnection::sessionBus());
        if (trayclient.isValid()) {
            m_registeredServices.append(notifierItemId);
            m_serviceWatcher->addWatchedService(service);
            emit StatusNotifierItemRegistered(notifierItemId);
        }
    }
}

QStringList StatusNotifierWatcher::RegisteredStatusNotifierItems() const
{
    return m_registeredServices;
}


void StatusNotifierWatcher::serviceUnregistered(const QString& name)
{
    kDebug()<<"Service "<< name << "unregistered";
    m_serviceWatcher->removeWatchedService(name);

    QString match = name + '/';
    QStringList::Iterator it = m_registeredServices.begin();
    while (it != m_registeredServices.end()) {
        if (it->startsWith(match)) {
            QString name = *it;
            it = m_registeredServices.erase(it);
            emit StatusNotifierItemUnregistered(name);
        } else {
            ++it;
        }
    }

    if (m_statusNotifierHostServices.contains(name)) {
        m_statusNotifierHostServices.remove(name);
        emit StatusNotifierHostUnregistered();
    }
}

void StatusNotifierWatcher::RegisterStatusNotifierHost(const QString &service)
{
    if (service.contains("org.kde.StatusNotifierHost-") &&
        QDBusConnection::sessionBus().interface()->isServiceRegistered(service).value() &&
        !m_statusNotifierHostServices.contains(service)) {
        kDebug()<<"Registering"<<service<<"as system tray";

        m_statusNotifierHostServices.insert(service);
        m_serviceWatcher->addWatchedService(service);
        emit StatusNotifierHostRegistered();
    }
}

bool StatusNotifierWatcher::IsStatusNotifierHostRegistered() const
{
    return !m_statusNotifierHostServices.isEmpty();
}

int StatusNotifierWatcher::ProtocolVersion() const
{
    return 0;
}

#include "statusnotifierwatcher.moc"
