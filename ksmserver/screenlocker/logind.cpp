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

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

const static QString s_login1Service = QStringLiteral("org.freedesktop.login1");
const static QString s_login1Path = QStringLiteral("/org/freedesktop/login1");
const static QString s_login1ManagerInterface = QStringLiteral("org.freedesktop.login1.Manager");
const static QString s_login1SessionInterface = QStringLiteral("org.freedesktop.login1.Session");

const static QString s_consolekitService = QStringLiteral("org.freedesktop.ConsoleKit");
const static QString s_consolekitPath = QStringLiteral("/org/freedesktop/ConsoleKit/Manager");
const static QString s_consolekitManagerInterface = QStringLiteral("org.freedesktop.ConsoleKit.Manager");
const static QString s_consolekitSessionInterface = QStringLiteral("org.freedesktop.ConsoleKit.Session");

LogindIntegration::LogindIntegration(const QDBusConnection &connection, QObject *parent)
    : QObject(parent)
    , m_bus(connection)
    , m_logindServiceWatcher(new QDBusServiceWatcher(s_login1Service,
                                                     m_bus,
                                                     QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration,
                                                     this))
    , m_connected(false)
    , m_inhibitFileDescriptor()
    , m_service(nullptr)
    , m_path(nullptr)
    , m_managerInterface(nullptr)
    , m_sessionInterface(nullptr)
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
                // Don't register ck if we have logind
                return;
            }
            if (reply.value().contains(s_consolekitService)) {
                consolekitServiceRegistered();
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

    m_service = &s_login1Service;
    m_path = &s_login1Path;
    m_managerInterface = &s_login1ManagerInterface;
    m_sessionInterface = &s_login1SessionInterface;

    commonServiceRegistered(watcher);
}

void LogindIntegration::consolekitServiceRegistered()
{
    // Don't try to register with ck if we have logind
    if (m_connected) {
        return;
    }

    // get the current session
    QDBusMessage message = QDBusMessage::createMethodCall(s_consolekitService,
                                                          s_consolekitPath,
                                                          s_consolekitManagerInterface,
                                                          QStringLiteral("GetCurrentSession"));
    QDBusPendingReply<QDBusObjectPath> session = m_bus.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(session, this);

    m_service = &s_consolekitService;
    m_path = &s_consolekitPath;
    m_managerInterface = &s_consolekitManagerInterface;
    m_sessionInterface = &s_consolekitSessionInterface;

    commonServiceRegistered(watcher);
}

void LogindIntegration::commonServiceRegistered(QDBusPendingCallWatcher *watcher)
{
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QDBusObjectPath> reply = *self;
            self->deleteLater();
            if (m_connected) {
                return;
            }
            if (!reply.isValid()) {
                qDebug() << "The session is not registered: " << reply.error().message();
                return;
            }
            const QString sessionPath = reply.value().path();
            qDebug() << "Session path:" << sessionPath;

            // connections need to be done this way as the object exposes both method and signal
            // with name "Lock"/"Unlock". Qt is not able to automatically handle this.
            m_bus.connect(*m_service,
                          sessionPath,
                          *m_sessionInterface,
                          QStringLiteral("Lock"),
                          this,
                          SIGNAL(requestLock()));
            m_bus.connect(*m_service,
                          sessionPath,
                          *m_sessionInterface,
                          QStringLiteral("Unlock"),
                          this,
                          SIGNAL(requestUnlock()));
            m_connected = true;
            emit connectedChanged();
        }
    );

    // connect to manager object's signals we need
    m_bus.connect(*m_service,
                  *m_path,
                  *m_managerInterface,
                  QStringLiteral("PrepareForSleep"),
                  this,
                  SIGNAL(prepareForSleep(bool)));
}

void LogindIntegration::inhibit()
{
    if (m_inhibitFileDescriptor.isValid()) {
        return;
    }

    if (!m_connected) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(*m_service,
                                                          *m_path,
                                                          *m_managerInterface,
                                                          QStringLiteral("Inhibit"));
    message.setArguments(QVariantList({QStringLiteral("sleep"),
                                       i18n("Screen Locker"),
                                       i18n("Ensuring that the screen gets locked before going to sleep"),
                                       QStringLiteral("delay")}));
    QDBusPendingReply<QDBusUnixFileDescriptor> reply = m_bus.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QDBusUnixFileDescriptor> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
                return;
            }
            reply.value().swap(m_inhibitFileDescriptor);
            emit inhibited();
        }
    );
}

void LogindIntegration::uninhibit()
{
    if (!m_inhibitFileDescriptor.isValid()) {
        return;
    }
    m_inhibitFileDescriptor = QDBusUnixFileDescriptor();
}

bool LogindIntegration::isInhibited() const
{
    return m_inhibitFileDescriptor.isValid();
}
