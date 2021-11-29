/*
    SPDX-FileCopyrightText: 2009 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "statusnotifierwatcher.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDebug>

#include <kpluginfactory.h>

#include "statusnotifieritem_interface.h"
#include "statusnotifierwatcheradaptor.h"

K_PLUGIN_CLASS_WITH_JSON(StatusNotifierWatcher, "statusnotifierwatcher.json")

StatusNotifierWatcher::StatusNotifierWatcher(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    setModuleName(QStringLiteral("StatusNotifierWatcher"));
    new StatusNotifierWatcherAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/StatusNotifierWatcher"), this);
    dbus.registerService(QStringLiteral("org.kde.StatusNotifierWatcher"));

    m_serviceWatcher = new QDBusServiceWatcher(this);
    m_serviceWatcher->setConnection(dbus);
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &StatusNotifierWatcher::serviceUnregistered);
}

StatusNotifierWatcher::~StatusNotifierWatcher()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService(QStringLiteral("org.kde.StatusNotifierWatcher"));
}

void StatusNotifierWatcher::RegisterStatusNotifierItem(const QString &serviceOrPath)
{
    QString service;
    QString path;
    if (serviceOrPath.startsWith(QLatin1Char('/'))) {
        service = message().service();
        path = serviceOrPath;
    } else {
        service = serviceOrPath;
        path = QStringLiteral("/StatusNotifierItem");
    }
    QString notifierItemId = service + path;
    if (m_registeredServices.contains(notifierItemId)) {
        return;
    }
    m_serviceWatcher->addWatchedService(service);
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(service).value()) {
        // check if the service has registered a SystemTray object
        org::kde::StatusNotifierItem trayclient(service, path, QDBusConnection::sessionBus());
        if (trayclient.isValid()) {
            qDebug() << "Registering" << notifierItemId << "to system tray";
            m_registeredServices.append(notifierItemId);
            Q_EMIT StatusNotifierItemRegistered(notifierItemId);
        } else {
            m_serviceWatcher->removeWatchedService(service);
        }
    } else {
        m_serviceWatcher->removeWatchedService(service);
    }
}

QStringList StatusNotifierWatcher::RegisteredStatusNotifierItems() const
{
    return m_registeredServices;
}

void StatusNotifierWatcher::serviceUnregistered(const QString &name)
{
    qDebug() << "Service " << name << "unregistered";
    m_serviceWatcher->removeWatchedService(name);

    QString match = name + QLatin1Char('/');
    QStringList::Iterator it = m_registeredServices.begin();
    while (it != m_registeredServices.end()) {
        if (it->startsWith(match)) {
            QString name = *it;
            it = m_registeredServices.erase(it);
            Q_EMIT StatusNotifierItemUnregistered(name);
        } else {
            ++it;
        }
    }

    if (m_statusNotifierHostServices.contains(name)) {
        m_statusNotifierHostServices.remove(name);
        Q_EMIT StatusNotifierHostUnregistered();
    }
}

void StatusNotifierWatcher::RegisterStatusNotifierHost(const QString &service)
{
    if (service.contains(QLatin1String("org.kde.StatusNotifierHost-")) && QDBusConnection::sessionBus().interface()->isServiceRegistered(service).value()
        && !m_statusNotifierHostServices.contains(service)) {
        qDebug() << "Registering" << service << "as system tray";

        m_statusNotifierHostServices.insert(service);
        m_serviceWatcher->addWatchedService(service);
        Q_EMIT StatusNotifierHostRegistered();
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
