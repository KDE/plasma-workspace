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
#ifndef LOGIND_H
#define LOGIND_H

#include <QDBusConnection>
#include <QDBusUnixFileDescriptor>
#include <QObject>

class QDBusServiceWatcher;

class LogindIntegration : public QObject
{
    Q_OBJECT
public:
    explicit LogindIntegration(QObject *parent = nullptr);
    ~LogindIntegration();

    bool isConnected() const {
        return m_connected;
    }

    void inhibit();
    void uninhibit();

Q_SIGNALS:
    void requestLock();
    void requestUnlock();
    void connectedChanged();
    void prepareForSleep(bool);
    void inhibited();

private:
    friend class LogindTest;
    /**
     * The DBusConnection argument is needed for the unit test. Logind uses the system bus
     * on which the unit test's fake logind cannot register to. Thus the unit test need to
     * be able to do everything over the session bus. This ctor allows the LogindTest to
     * create a LogindIntegration which listens on the session bus.
     **/
    explicit LogindIntegration(const QDBusConnection &connection, QObject *parent = nullptr);
    void logindServiceRegistered();
    QDBusConnection m_bus;
    QDBusServiceWatcher *m_logindServiceWatcher;
    bool m_connected;
    QDBusUnixFileDescriptor m_inhibitFileDescriptor;
};

#endif
