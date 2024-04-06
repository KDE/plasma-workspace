/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "dbusservicewatcher.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>

DBusServiceWatcher::DBusServiceWatcher(QObject *parent)
    : QObject(parent)
{
    m_watcher.setConnection(QDBusConnection::sessionBus());
    m_watcher.setWatchMode(QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
}

DBusServiceWatcher::~DBusServiceWatcher()
{
}

BusType::Type DBusServiceWatcher::busType() const
{
    return m_busType;
}

void DBusServiceWatcher::setBusType(BusType::Type conn)
{
    if (m_busType == conn) {
        return;
    }

    m_busType = conn;
    switch (m_busType) {
    case BusType::System:
        m_watcher.setConnection(QDBusConnection::systemBus());
        break;
    case BusType::Session:
    default:
        m_watcher.setConnection(QDBusConnection::sessionBus());
    }

    if (m_ready) {
        checkServiceRegistered();
    }
}

QBindable<bool> DBusServiceWatcher::isRegistered() const
{
    return &m_registered;
}

QString DBusServiceWatcher::watchedService() const
{
    const QStringList serviceList = m_watcher.watchedServices();
    return serviceList.empty() ? QString() : serviceList[0];
}

void DBusServiceWatcher::setWatchedService(const QString &service)
{
    QBindable<QStringList> bindable = m_watcher.bindableWatchedServices();
    if (service.isEmpty()) {
        bindable.setValue({});
        return;
    }

    const QStringList oldList = bindable.value();
    if (!oldList.empty() && service == oldList[0]) {
        Q_ASSERT(oldList.size() == 1);
        return;
    }

    bindable.setValue({service});
}

void DBusServiceWatcher::classBegin()
{
}

void DBusServiceWatcher::componentComplete()
{
    m_ready = true;
    m_watchedServiceNotifier = m_watcher.bindableWatchedServices().addNotifier([this] {
        Q_EMIT watchedServiceChanged();
        checkServiceRegistered();
    });
    connect(&m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &DBusServiceWatcher::onServiceRegistered);
    connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &DBusServiceWatcher::onServiceUnregistered);
    checkServiceRegistered();
}

void DBusServiceWatcher::onServiceRegistered(const QString & /*serviceName*/)
{
    m_registered = true;
}

void DBusServiceWatcher::onServiceUnregistered(const QString & /*serviceName*/)
{
    m_registered = false;
}

void DBusServiceWatcher::checkServiceRegistered()
{
    Q_ASSERT(m_ready);
    const QString serviceName = watchedService();
    if (serviceName.isEmpty()) {
        m_registered = false;
        return;
    }
    m_registered = m_watcher.connection().interface()->isServiceRegistered(serviceName);
}

#include "moc_dbusservicewatcher.cpp"
