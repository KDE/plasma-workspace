/*
   Copyright (C) 2019 David Edmundson <davidedmundson@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#pragma once

#include <KConfigWatcher>
#include <QObject>

#include "kworkspace_export.h"
#include "sessionmanagement.h"

class OrgFreedesktopLogin1ManagerInterface;
class OrgFreedesktopUPowerInterface;
class OrgFreedesktopConsoleKitManagerInterface;

/**
 * Performs direct system actions that could kill the session
 *
 * Semi-internal. Symbols exported, but not the header
 * To be used only by the daemon that performs logout (currently ksmserver)
 *
 * All other users should go via the public SessionManagement that prompts and logs out properly.
 */
class KWORKSPACE_EXPORT SessionBackend : public QObject
{
    Q_OBJECT
public:
    static SessionBackend *self();
    virtual SessionManagement::State state() const = 0;

    virtual void shutdown() = 0;
    virtual void reboot() = 0;
    virtual void suspend() = 0;
    virtual void hybridSuspend() = 0;
    virtual void hibernate() = 0;

    virtual bool canShutdown() const = 0;
    virtual bool canReboot() const = 0;
    virtual bool canSuspend() const = 0;
    virtual bool canHybridSuspend() const = 0;
    virtual bool canHibernate() const = 0;

    virtual bool canSwitchUser() const;

    bool confirmLogout() const;

Q_SIGNALS:
    void stateChanged();
    void canShutdownChanged();
    void canRebootChanged();
    void canSuspendChanged();
    void canHybridSuspendChanged();
    void canHibernateChanged();

    void aboutToSuspend();
    void resumingFromSuspend();

protected:
    SessionBackend();
    ~SessionBackend() override = default;

private:
    KConfigWatcher::Ptr m_kserverConfig;
};

/*
 * This class wraps both Logind and CK2
 * Abstraction for that is handled in OrgFreedesktopLogin1ManagerInterface
 */
class LogindSessionBackend : public SessionBackend
{
    Q_OBJECT
public:
    static bool exists();
    LogindSessionBackend();

    SessionManagement::State state() const override;
    void shutdown() override;
    void reboot() override;
    void suspend() override;
    void hybridSuspend() override;
    void hibernate() override;
    bool canShutdown() const override;
    bool canReboot() const override;
    bool canSuspend() const override;
    bool canHybridSuspend() const override;
    bool canHibernate() const override;

private:
    OrgFreedesktopLogin1ManagerInterface *m_login1;
    SessionManagement::State m_state = SessionManagement::State::Loading;
    bool m_canShutdown = false;
    bool m_canReboot = false;
    bool m_canSuspend = false;
    bool m_canHybridSuspend = false;
    bool m_canHibernate = false;
    uint m_pendingJobs = 0;
};

/* Maybe misleadingly named, consolekit doesn't support suspend directly so it's
 * suplemented with upower where available
 */
class ConsoleKitSessionBackend : public SessionBackend
{
    Q_OBJECT
public:
    static bool exists();
    ConsoleKitSessionBackend();

    SessionManagement::State state() const override;
    void shutdown() override;
    void reboot() override;
    void suspend() override;
    void hybridSuspend() override
    {
    }
    void hibernate() override;
    bool canShutdown() const override;
    bool canReboot() const override;
    bool canSuspend() const override;
    bool canHybridSuspend() const override
    {
        return false;
    }
    bool canHibernate() const override;

private:
    OrgFreedesktopUPowerInterface *m_upower;
    OrgFreedesktopConsoleKitManagerInterface *m_ck;

    SessionManagement::State m_state = SessionManagement::State::Loading;
    bool m_canShutdown = false;
    bool m_canReboot = false;
    bool m_canSuspend = false;
    bool m_canHibernate = false;
    uint m_pendingJobs = 0;
};

class DummySessionBackend : public SessionBackend
{
    Q_OBJECT
public:
    DummySessionBackend();

    SessionManagement::State state() const override
    {
        return SessionManagement::State::Error;
    }
    void shutdown() override
    {
    }
    void reboot() override
    {
    }
    void suspend() override
    {
    }
    void hybridSuspend() override
    {
    }
    void hibernate() override
    {
    }
    bool canShutdown() const override
    {
        return false;
    }
    bool canReboot() const override
    {
        return false;
    }
    bool canSuspend() const override
    {
        return false;
    }
    bool canHybridSuspend() const override
    {
        return false;
    }
    bool canHibernate() const override
    {
        return false;
    }
};
