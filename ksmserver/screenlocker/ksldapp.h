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

#include <ksld_export.h>

// forward declarations
class GlobalAccel;
class LogindIntegration;
class KActionCollection;
class QTimer;
class KSldTest;

namespace ScreenLocker
{

enum class EstablishLock {
    Immediate,
    Delayed
};

class AbstractLocker;
class WaylandServer;

class KSLD_EXPORT KSldApp : public QObject
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

    void lock(EstablishLock establishLock);

Q_SIGNALS:
    void locked();
    void unlocked();

private Q_SLOTS:
    void cleanUp();
    void endGraceTime();
    void solidSuspend();

private:
    void initialize();
    void initializeX11();
    bool establishGrab();
    void startLockProcess(EstablishLock establishLock);
    void showLockWindow();
    void hideLockWindow();
    void doUnlock();
    bool isFdoPowerInhibited() const;

    KActionCollection *m_actionCollection;
    LockState m_lockState;
    QProcess *m_lockProcess;
    AbstractLocker *m_lockWindow;
    WaylandServer *m_waylandServer;
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
    LogindIntegration *m_logind;
    GlobalAccel *m_globalAccel = nullptr;
    bool m_hasXInput2 = false;

    bool m_isX11;
    bool m_isWayland;
    int m_greeterCrashedCounter = 0;

    // for auto tests
    friend KSldTest;
};
} // namespace

#endif // SCREENLOCKER_KSLDAPP_H
