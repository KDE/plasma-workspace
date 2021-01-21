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
#include "sessionmanagementbackend.h"

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>

#include <KConfigGroup>

#include "kdisplaymanager.h"

#include "consolekit_manager_interface.h"
#include "login1_manager_interface.h"
#include "upower_interface.h"

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
    } else if (ConsoleKitSessionBackend::exists()) {
        s_backend = new ConsoleKitSessionBackend();
    } else {
        s_backend = new DummySessionBackend();
    }

    return s_backend;
}

SessionBackend::SessionBackend()
{
    m_kserverConfig = KConfigWatcher::create(KSharedConfig::openConfig("ksmserverrc"));
}

bool SessionBackend::confirmLogout() const
{
    return m_kserverConfig->config()->group("General").readEntry("confirmLogout", true);
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
            emit stateChanged();
            emit canShutdownChanged();
            emit canRebootChanged();
            emit canSuspendChanged();
            emit canHibernateChanged();
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

    connect(m_login1, &OrgFreedesktopLogin1ManagerInterface::PrepareForSleep, this, [this](bool sleeping) {
        if (sleeping) {
            emit aboutToSuspend();
        } else {
            emit resumingFromSuspend();
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
    // its not really needed for suspend tasks where the calling app won't be closing
    m_login1->PowerOff(true).waitForFinished();
}

void LogindSessionBackend::reboot()
{
    m_login1->Reboot(true).waitForFinished();
}

void LogindSessionBackend::suspend()
{
    m_login1->Suspend(true);
}

void LogindSessionBackend::hybridSuspend()
{
    m_login1->HybridSleep(true);
}

void LogindSessionBackend::hibernate()
{
    m_login1->Hibernate(true);
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

/*********************************************************************************/

bool ConsoleKitSessionBackend::exists()
{
    return QDBusConnection::systemBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.login1"));
}

ConsoleKitSessionBackend::ConsoleKitSessionBackend()
{
    m_ck = new OrgFreedesktopConsoleKitManagerInterface(QStringLiteral("org.freedesktop.ConsoleKit"),
                                                        QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
                                                        QDBusConnection::systemBus(),
                                                        this);
    m_upower = new OrgFreedesktopUPowerInterface(QStringLiteral("org.freedesktop.UPower"),
                                                 QStringLiteral("/org/freedesktop/UPower"),
                                                 QDBusConnection::systemBus(),
                                                 this);

    auto canStop = m_ck->CanStop();
    canStop.waitForFinished();
    m_canShutdown = canStop.value();

    auto canRestart = m_ck->CanRestart();
    canRestart.waitForFinished();
    m_canReboot = canRestart.value();

    m_canSuspend = m_upower->canSuspend();
    m_canHibernate = m_upower->canHibernate();

    connect(m_upower, &OrgFreedesktopUPowerInterface::AboutToSleep, this, &SessionBackend::aboutToSuspend);
    connect(m_upower, &OrgFreedesktopUPowerInterface::Resuming, this, &SessionBackend::resumingFromSuspend);

    m_state = SessionManagement::State::Ready;
}

SessionManagement::State ConsoleKitSessionBackend::state() const
{
    return m_state;
}

void ConsoleKitSessionBackend::shutdown()
{
    m_ck->Stop();
}

void ConsoleKitSessionBackend::reboot()
{
    m_ck->Restart();
}

void ConsoleKitSessionBackend::suspend()
{
    m_upower->Suspend();
}

void ConsoleKitSessionBackend::hibernate()
{
    m_upower->Hibernate();
}

bool ConsoleKitSessionBackend::canShutdown() const
{
    return m_canShutdown;
}

bool ConsoleKitSessionBackend::canReboot() const
{
    return m_canReboot;
}

bool ConsoleKitSessionBackend::canSuspend() const
{
    return m_canSuspend;
}

bool ConsoleKitSessionBackend::canHibernate() const
{
    return m_canHibernate;
}
