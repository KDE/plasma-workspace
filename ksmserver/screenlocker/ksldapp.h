/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

 Copyright 1999 Martin R. Jones <mjones@kde.org>
 Copyright 2003 Oswald Buddenhagen <ossi@kde.org>
 Copyright 2008 Chani Armitage <chanika@gmail.com>
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
#ifndef SCREENLOCKER_KSLDAPP_H
#define SCREENLOCKER_KSLDAPP_H

#include <QtCore/QElapsedTimer>
#include <QtCore/QProcess>

// forward declarations
class KActionCollection;
class QTimer;

namespace ScreenLocker
{

class LockWindow;

class KSldApp : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.ksld.App")
    Q_ENUMS(LockState)

public:
    enum LockState {
        Unlocked,
        AcquiringLock,
        Locked
    };

    static KSldApp* self();

    KSldApp(QObject * parent = 0);
    virtual ~KSldApp();

    // The action collection of the active widget
    KActionCollection* actionCollection();

    LockState lockState() const {
        return m_lockState;
    }

    /**
     * @returns the number of milliseconds passed since the screen has been locked.
     **/
    uint activeTime() const;

    void configure();

    bool isGraceTime() const;

    /**
     * Can be used by the lock window to remove the lock during grace time.
     **/
    void unlock();
    void inhibit();
    void uninhibit();

    uint autoLogoutTimeout() const {
        return m_autoLogoutTimeout;
    }

public Q_SLOTS:
    Q_SCRIPTABLE void lock();
    void lock(bool immediateLock);
    void lockProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void lockProcessReady();

Q_SIGNALS:
    void locked();
    void unlocked();

private Q_SLOTS:
    void cleanUp();
    void idleTimeout(int identifier);
    void endGraceTime();

private:
    void initialize();
    bool establishGrab();
    bool grabKeyboard();
    bool grabMouse();
    bool startLockProcess(bool immediateLock);
    void showLockWindow();
    void hideLockWindow();
    void doUnlock();

    KActionCollection *m_actionCollection;
    LockState m_lockState;
    QProcess *m_lockProcess;
    LockWindow *m_lockWindow;
    /**
     * Timer to measure how long the screen is locked.
     * This information is required by DBus Interface.
     **/
    QElapsedTimer m_lockedTimer;
    int m_idleId;
    /**
     * Number of milliseconds after locking in which user activity will result in screen being
     * unlocked without requiring a password.
     **/
    int m_lockGrace;
    /**
     * Controls whether user activity may remove the lock. Only enabled after idle timeout.
     **/
    bool m_inGraceTime;
    /**
     * Grace time ends when timer expires.
     **/
    QTimer *m_graceTimer;
    int m_inhibitCounter;
    uint m_autoLogoutTimeout;
    bool m_plasmaEnabled;
};
} // namespace

#endif // SCREENLOCKER_KSLDAPP_H
