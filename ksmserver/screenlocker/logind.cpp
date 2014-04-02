/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "logind.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>

const static QString s_login1Service = QStringLiteral("org.freedesktop.login1");
const static QString s_login1Path = QStringLiteral("/org/freedesktop/login1");
const static QString s_login1ManagerInterface = QStringLiteral("org.freedesktop.login1.Manager");
const static QString s_login1SessionInterface = QStringLiteral("org.freedesktop.login1.Session");

LogindIntegration::LogindIntegration(const QDBusConnection &connection, QObject *parent)
    : QObject(parent)
    , m_bus(connection)
    , m_logindServiceWatcher(new QDBusServiceWatcher(s_login1Service,
                                                     m_bus,
                                                     QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration,
                                                     this))
    , m_connected(false)
{
    connect(m_logindServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &LogindIntegration::logindServiceRegistered);
    connect(m_logindServiceWatcher, &QDBusServiceWatcher::serviceUnregistered, this,
        [this]() {
            m_connected = false;
            emit connectedChanged();
        }
    );

    // check whether the logind service is registered
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"),
                                                          QStringLiteral("/"),
                                                          QStringLiteral("org.freedesktop.DBus"),
                                                          QStringLiteral("ListNames"));
    QDBusPendingReply<QStringList> async = m_bus.asyncCall(message);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QStringList> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
                return;
            }
            if (reply.value().contains(s_login1Service)) {
                logindServiceRegistered();
            }
        }
    );
}

LogindIntegration::LogindIntegration(QObject *parent)
    : LogindIntegration(QDBusConnection::systemBus(), parent)
{
}

LogindIntegration::~LogindIntegration() = default;

void LogindIntegration::logindServiceRegistered()
{
    // get the current session
    QDBusMessage message = QDBusMessage::createMethodCall(s_login1Service,
                                                          s_login1Path,
                                                          s_login1ManagerInterface,
                                                          QStringLiteral("GetSessionByPID"));
    message.setArguments(QVariantList() << (quint32) QCoreApplication::applicationPid());
    QDBusPendingReply<QDBusObjectPath> session = m_bus.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(session, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QDBusObjectPath> reply = *self;
            self->deleteLater();
            if (m_connected) {
                return;
            }
            if (!reply.isValid()) {
                qDebug() << "The session is not registered with logind" << reply.error().message();
                return;
            }
            const QString sessionPath = reply.value().path();
            qDebug() << "Session path:" << sessionPath;

            // connections need to be done this way as the object exposes both method and signal
            // with name "Lock"/"Unlock". Qt is not able to automatically handle this.
            m_bus.connect(s_login1Service,
                          sessionPath,
                          s_login1SessionInterface,
                          QStringLiteral("Lock"),
                          this,
                          SIGNAL(requestLock()));
            m_bus.connect(s_login1Service,
                          sessionPath,
                          s_login1SessionInterface,
                          QStringLiteral("Unlock"),
                          this,
                          SIGNAL(requestUnlock()));
            m_connected = true;
            emit connectedChanged();
        }
    );
}
