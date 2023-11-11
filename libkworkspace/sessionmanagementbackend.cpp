/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sessionmanagementbackend.h"

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>

#include <KConfigGroup>

#include "kdisplaymanager.h"

#include "login1_manager_interface.h"

static SessionBackend *s_backend = nullptr;

SessionBackend *SessionBackend::self()
{
    static QMutex mutex;
    QMutexLocker lock(&mutex);

    if (s_backend) {
        return s_backend;
    }

    if (LogindSessionBackend::exists()) {
        s_backend = new LogindSessionBackend();
    } else {
        s_backend = new DummySessionBackend();
    }

    return s_backend;
}

SessionBackend::SessionBackend()
    : m_kserverConfig(KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("ksmserverrc"))))
{
}

bool SessionBackend::confirmLogout() const
{
    return m_kserverConfig->config()->group(QStringLiteral("General")).readEntry("confirmLogout", true);
}

bool SessionBackend::canSwitchUser() const
{
    return KDisplayManager().isSwitchable();
}

DummySessionBackend::DummySessionBackend()
{
    qCritical() << "Could not load a session backend. Session management operations such as shutdown will not be operational. This is a setup issue.";
}

/*********************************************************************************/

LogindSessionBackend::LogindSessionBackend()
{
    m_login1 = new OrgFreedesktopLogin1ManagerInterface(QStringLiteral("org.freedesktop.login1"),
                                                        QStringLiteral("/org/freedesktop/login1"),
                                                        QDBusConnection::systemBus(),
                                                        this);

    auto propLoaded = [this](QDBusPendingCallWatcher *watcher, bool *argToUpdate) {
        watcher->deleteLater();
        m_pendingJobs--;
        QDBusPendingReply<QString> reply = *watcher;
        if (reply.isError()) {
            *argToUpdate = false;
        } else {
            // both "yes" and "challenge" will show up in the UI
            const QString value = reply.value();
            *argToUpdate = false;
            if (value == QLatin1String("yes") || value == QLatin1String("challenge")) {
                *argToUpdate = true;
            }
        }

        if (m_pendingJobs == 0) {
            m_state = SessionManagement::State::Ready;
            Q_EMIT stateChanged();
            Q_EMIT canShutdownChanged();
            Q_EMIT canRebootChanged();
            Q_EMIT canSuspendChanged();
            Q_EMIT canHibernateChanged();
            Q_EMIT canSuspendThenHibernateChanged();
        }
    };

    m_pendingJobs = 5;
    {
        auto watcher = new QDBusPendingCallWatcher(m_login1->CanPowerOff(), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, std::bind(propLoaded, std::placeholders::_1, &m_canShutdown));
    }
    {
        auto watcher = new QDBusPendingCallWatcher(m_login1->CanReboot(), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, std::bind(propLoaded, std::placeholders::_1, &m_canReboot));
    }
    {
        auto watcher = new QDBusPendingCallWatcher(m_login1->CanSuspend(), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, std::bind(propLoaded, std::placeholders::_1, &m_canSuspend));
    }
    {
        auto watcher = new QDBusPendingCallWatcher(m_login1->CanHybridSleep(), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, std::bind(propLoaded, std::placeholders::_1, &m_canHybridSuspend));
    }
    {
        auto watcher = new QDBusPendingCallWatcher(m_login1->CanHibernate(), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, std::bind(propLoaded, std::placeholders::_1, &m_canHibernate));
    }
    {
        auto watcher = new QDBusPendingCallWatcher(m_login1->CanSuspendThenHibernate(), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, std::bind(propLoaded, std::placeholders::_1, &m_canSuspendThenHibernate));
    }

    connect(m_login1, &OrgFreedesktopLogin1ManagerInterface::PrepareForSleep, this, [this](bool sleeping) {
        if (sleeping) {
            Q_EMIT aboutToSuspend();
        } else {
            Q_EMIT resumingFromSuspend();
        }
    });
}

SessionManagement::State LogindSessionBackend::state() const
{
    return m_state;
}

void LogindSessionBackend::shutdown()
{
    // logind will confirm credentials with the caller, if the app quits after sending this
    // this may fail
    m_login1->PowerOff(true).waitForFinished();
}

void LogindSessionBackend::reboot()
{
    m_login1->Reboot(true).waitForFinished();
}

void LogindSessionBackend::suspend()
{
    // these need to be synchronous as well - ksmserver-logout-greeter specifically calls these
    // and will quit immediately after
    m_login1->Suspend(true).waitForFinished();
}

void LogindSessionBackend::hybridSuspend()
{
    m_login1->HybridSleep(true).waitForFinished();
}

void LogindSessionBackend::hibernate()
{
    m_login1->Hibernate(true).waitForFinished();
}

void LogindSessionBackend::suspendThenHibernate()
{
    m_login1->SuspendThenHibernate(true).waitForFinished();
}

bool LogindSessionBackend::canShutdown() const
{
    return m_canShutdown;
}

bool LogindSessionBackend::canReboot() const
{
    return m_canReboot;
}

bool LogindSessionBackend::canSuspend() const
{
    return m_canSuspend;
}

bool LogindSessionBackend::canHybridSuspend() const
{
    return m_canHybridSuspend;
}

bool LogindSessionBackend::canHibernate() const
{
    return m_canHibernate;
}

bool LogindSessionBackend::canSuspendThenHibernate() const
{
    return m_canSuspendThenHibernate;
}
