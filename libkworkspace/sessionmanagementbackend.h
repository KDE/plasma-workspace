/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KConfigWatcher>
#include <QObject>

#include "kworkspace_export.h"
#include "sessionmanagement.h"

class OrgFreedesktopLogin1ManagerInterface;

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
    virtual void sleep() = 0;
    virtual void suspend() = 0;
    virtual void hybridSuspend() = 0;
    virtual void hibernate() = 0;
    virtual void suspendThenHibernate() = 0;

    virtual bool canShutdown() const = 0;
    virtual bool canReboot() const = 0;
    virtual bool canSleep() const = 0;
    virtual bool canSuspend() const = 0;
    virtual bool canHybridSuspend() const = 0;
    virtual bool canHibernate() const = 0;
    virtual bool canSuspendThenHibernate() const = 0;

    virtual bool canSwitchUser() const;

    bool confirmLogout() const;

Q_SIGNALS:
    void stateChanged();
    void canShutdownChanged();
    void canRebootChanged();
    void canSleepChanged();
    void canSuspendChanged();
    void canHybridSuspendChanged();
    void canHibernateChanged();
    void canSuspendThenHibernateChanged();

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
    void sleep() override;
    void suspend() override;
    void hybridSuspend() override;
    void hibernate() override;
    void suspendThenHibernate() override;
    bool canShutdown() const override;
    bool canReboot() const override;
    bool canSleep() const override;
    bool canSuspend() const override;
    bool canHybridSuspend() const override;
    bool canHibernate() const override;
    bool canSuspendThenHibernate() const override;
    bool canSwitchUser() const override;

private:
    OrgFreedesktopLogin1ManagerInterface *m_login1;
    SessionManagement::State m_state = SessionManagement::State::Loading;
    bool m_canShutdown = false;
    bool m_canReboot = false;
    bool m_canSleep = false;
    bool m_canSuspend = false;
    bool m_canHybridSuspend = false;
    bool m_canHibernate = false;
    bool m_canSuspendThenHibernate = false;
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
    void sleep() override
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
    void suspendThenHibernate() override
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
    bool canSleep() const override
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
    bool canSuspendThenHibernate() const override
    {
        return false;
    }
};

class TestSessionBackend : public SessionBackend
{
    Q_OBJECT
public:
    TestSessionBackend();

    SessionManagement::State state() const override
    {
        return SessionManagement::State::Ready;
    }
    void shutdown() override;
    void reboot() override;
    void sleep() override;
    void suspend() override;
    void hybridSuspend() override;
    void hibernate() override;
    void suspendThenHibernate() override;

    bool canShutdown() const override
    {
        return true;
    }
    bool canReboot() const override
    {
        return true;
    }
    bool canSleep() const override
    {
        return true;
    }
    bool canSuspend() const override
    {
        return true;
    }
    bool canHybridSuspend() const override
    {
        return true;
    }
    bool canHibernate() const override
    {
        return true;
    }
    bool canSuspendThenHibernate() const override
    {
        return true;
    }
};
