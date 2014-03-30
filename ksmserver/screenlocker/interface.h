/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright 1999 Martin R. Jones <mjones@kde.org>
Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

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
#ifndef SCREENLOCKER_INTERFACE_H
#define SCREENLOCKER_INTERFACE_H

#include <QtCore/QObject>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusMessage>

class QDBusServiceWatcher;

namespace ScreenLocker
{

class InhibitRequest
{
public:
    QString dbusid;
    uint cookie;
    uint powerdevilcookie;
};

class KSldApp;
class Interface : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ScreenSaver")
public:
    explicit Interface(KSldApp *parent = 0);
    virtual ~Interface();

public Q_SLOTS:
    /**
     * Lock the screen.
     */
    void Lock();

    /**
     * Simulate user activity
     */
    void SimulateUserActivity();
    /**
     * Request a change in the state of the screensaver.
     * Set to TRUE to request that the screensaver activate.
     * Active means that the screensaver has blanked the
     * screen and may run a graphical theme.  This does
     * not necessary mean that the screen is locked.
     */
    bool SetActive(bool state);

    /// Returns the value of the current state of activity (See setActive)
    bool GetActive();

    /**
     * Returns the number of seconds that the screensaver has
     * been active.  Returns zero if the screensaver is not active.
     */
    uint GetActiveTime();

    /**
     * Returns the number of seconds that the session has
     * been idle.  Returns zero if the session is not idle.
     */
    uint GetSessionIdleTime();

    /**
     * Request that saving the screen due to system idleness
     * be blocked until UnInhibit is called or the
     * calling process exits.
     * The cookie is a random number used to identify the request
     */
    uint Inhibit(const QString &application_name, const QString &reason_for_inhibit);
    /// Cancel a previous call to Inhibit() identified by the cookie.
    void UnInhibit(uint cookie);

    /**
     * Request that running themes while the screensaver is active
     * be blocked until UnThrottle is called or the
     * calling process exits.
     * The cookie is a random number used to identify the request
     */
    uint Throttle(const QString &application_name, const QString &reason_for_inhibit);
    /// Cancel a previous call to Throttle() identified by the cookie.
    void UnThrottle(uint cookie);

    // org.kde.screensvar
    void setupPlasma();
    void configure();
    void saverLockReady();

Q_SIGNALS:
    // DBus signals
    void ActiveChanged(bool state);

private Q_SLOTS:
    void slotLocked();
    void slotUnlocked();
    void serviceUnregistered(const QString &name);

private:
    void sendLockReplies();

    KSldApp *m_daemon;
    QDBusServiceWatcher *m_serviceWatcher;
    QList<InhibitRequest> m_requests;
    uint m_next_cookie;
    QList<QDBusMessage> m_lockReplies;
};
}

#endif // SCREENLOCKER_INTERFACE_H
