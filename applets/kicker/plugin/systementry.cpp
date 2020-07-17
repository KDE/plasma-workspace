/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "systementry.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCall>

#include <KAuthorized>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <kworkspace.h>
#include <sessionmanagement.h>

int SystemEntry::s_instanceCount = 0;
SessionManagement* SystemEntry::s_sessionManagement = nullptr;

SystemEntry::SystemEntry(AbstractModel *owner, Action action) : AbstractEntry(owner)
, m_action(action)
, m_valid(false)
, m_initialized(false)
{
    refresh();
    ++s_instanceCount;
    m_initialized = true;
}

SystemEntry::SystemEntry(AbstractModel *owner, const QString &id) : AbstractEntry(owner)
, m_action(NoAction)
, m_valid(false)
, m_initialized(false)
{
    if (id == QLatin1String("lock-screen")) {
        m_action = LockSession;
    } else if (id == QLatin1String("logout")) {
        m_action = LogoutSession;
    } else if (id == QLatin1String("save-session")) {
        m_action = SaveSession;
    } else if (id == QLatin1String("switch-user")) {
        m_action = SwitchUser;
    } else if (id == QLatin1String("suspend")) {
        m_action = Suspend;
    } else if (id == QLatin1String("hibernate")) {
        m_action = Hibernate;
    } else if (id == QLatin1String("reboot")) {
        m_action = Reboot;
    } else if (id == QLatin1String("shutdown")) {
        m_action = Shutdown;
    }

    refresh();
    ++s_instanceCount;
    m_initialized = true;
}

SystemEntry::~SystemEntry()
{
    --s_instanceCount;

    if (!s_instanceCount) {
        delete s_sessionManagement;
        s_sessionManagement = nullptr;
    }
}

SystemEntry::Action SystemEntry::action() const
{
    return m_action;
}

void SystemEntry::refresh()
{
    if (!s_sessionManagement) {
        s_sessionManagement = new SessionManagement();
    }

    bool valid = false;

    switch (m_action) {
        case LockSession: {
            valid = s_sessionManagement->canLock();
            QObject::connect(s_sessionManagement, &SessionManagement::canLockChanged,
                this, &SystemEntry::refresh);
            break;
        }
        case LogoutSession: {
            valid = s_sessionManagement->canLogout();
            QObject::connect(s_sessionManagement, &SessionManagement::canLogoutChanged,
                this, &SystemEntry::refresh);
            break;
        }
        case SaveSession: {
            valid = s_sessionManagement->canSaveSession();
            QObject::connect(s_sessionManagement, &SessionManagement::canSaveSessionChanged,
                this, &SystemEntry::refresh);
            break;
        }
        case SwitchUser: {
            valid = s_sessionManagement->canSwitchUser();
            QObject::connect(s_sessionManagement, &SessionManagement::canSwitchUserChanged,
                this, &SystemEntry::refresh);
            break;
        }
        case Suspend: {
            valid = s_sessionManagement->canSuspend();
            QObject::connect(s_sessionManagement, &SessionManagement::canSuspendChanged,
                this, &SystemEntry::refresh);
            break;
        }
        case Hibernate: {
            valid = s_sessionManagement->canHibernate();
            QObject::connect(s_sessionManagement, &SessionManagement::canHibernateChanged,
                this, &SystemEntry::refresh);
            break;
        }
        case Reboot: {
            valid = s_sessionManagement->canReboot();
            QObject::connect(s_sessionManagement, &SessionManagement::canRebootChanged,
                this, &SystemEntry::refresh);
            break;
        }
        case Shutdown: {
            valid = s_sessionManagement->canShutdown();
            QObject::connect(s_sessionManagement, &SessionManagement::canShutdownChanged,
                this, &SystemEntry::refresh);
            break;
        }
        default:
            break;
    }


    if (m_valid != valid) {
        m_valid = valid;

        if (m_initialized) {
            emit isValidChanged();
        }
    }
}

bool SystemEntry::isValid() const
{
    return m_valid;
}

QIcon SystemEntry::icon() const
{
    const QString &name = iconName();

    if (!name.isEmpty()) {
        return QIcon::fromTheme(name, QIcon::fromTheme(QStringLiteral("unknown")));
    }

    return QIcon::fromTheme(QStringLiteral("unknown"));
}

QString SystemEntry::iconName() const
{
    switch (m_action) {
        case LockSession:
            return QStringLiteral("system-lock-screen");
            break;
        case LogoutSession:
            return QStringLiteral("system-log-out");
            break;
        case SaveSession:
            return QStringLiteral("system-save-session");
            break;
        case SwitchUser:
            return QStringLiteral("system-switch-user");
            break;
        case Suspend:
            return QStringLiteral("system-suspend");
            break;
        case Hibernate:
            return QStringLiteral("system-suspend-hibernate");
            break;
        case Reboot:
            return QStringLiteral("system-reboot");
            break;
        case Shutdown:
            return QStringLiteral("system-shutdown");
            break;
        default:
            break;
    }

    return QString();
}

QString SystemEntry::name() const
{
    switch (m_action) {
        case LockSession:
            return i18n("Lock");
            break;
        case LogoutSession:
            return i18n("Log Out");
            break;
        case SaveSession:
            return i18n("Save Session");
            break;
        case SwitchUser:
            return i18n("Switch User");
            break;
        case Suspend:
            return i18nc("Suspend to RAM", "Sleep");
            break;
        case Hibernate:
            return i18n("Hibernate");
            break;
        case Reboot:
            return i18n("Restart");
            break;
        case Shutdown:
            return i18n("Shut Down");
            break;
        default:
            break;
    }

    return QString();
}

QString SystemEntry::group() const
{
    switch (m_action) {
        case LockSession:
            return i18n("Session");
            break;
        case LogoutSession:
            return i18n("Session");
            break;
        case SaveSession:
            return i18n("Session");
            break;
        case SwitchUser:
            return i18n("Session");
            break;
        case Suspend:
            return i18n("System");
            break;
        case Hibernate:
            return i18n("System");
            break;
        case Reboot:
            return i18n("System");
            break;
        case Shutdown:
            return i18n("System");
            break;
        default:
            break;
    }

    return QString();
}

QString SystemEntry::description() const
{
    switch (m_action) {
        case LockSession:
            return i18n("Lock screen");
            break;
        case LogoutSession:
            return i18n("End session");
            break;
        case SaveSession:
            return i18n("Save Session");
            break;
        case SwitchUser:
            return i18n("Start a parallel session as a different user");
            break;
        case Suspend:
            return i18n("Suspend to RAM");
            break;
        case Hibernate:
            return i18n("Suspend to disk");
            break;
        case Reboot:
            return i18n("Restart computer");
            break;
        case Shutdown:
            return i18n("Turn off computer");
            break;
        default:
            break;
    }

    return QString();
}

QString SystemEntry::id() const
{
    switch (m_action) {
        case LockSession:
            return QStringLiteral("lock-screen");
            break;
        case LogoutSession:
            return QStringLiteral("logout");
            break;
        case SaveSession:
            return QStringLiteral("save-session");
            break;
        case SwitchUser:
            return QStringLiteral("switch-user");
            break;
        case Suspend:
            return QStringLiteral("suspend");
            break;
        case Hibernate:
            return QStringLiteral("hibernate");
            break;
        case Reboot:
            return QStringLiteral("reboot");
            break;
        case Shutdown:
            return QStringLiteral("shutdown");
            break;

        default:
            break;
    }

    return QString();
}

bool SystemEntry::run(const QString& actionId, const QVariant &argument)
{
    Q_UNUSED(actionId)
    Q_UNUSED(argument)

    if (!m_valid) {
        return false;
    }

    switch (m_action) {
        case LockSession: s_sessionManagement->lock(); break;
        case LogoutSession: s_sessionManagement->requestLogout(); break;
        case SaveSession: s_sessionManagement->saveSession(); break;
        case SwitchUser: s_sessionManagement->switchUser(); break;
        case Suspend: s_sessionManagement->suspend(); break;
        case Hibernate: s_sessionManagement->hibernate(); break;
        case Reboot: s_sessionManagement->requestReboot(); break;
        case Shutdown: s_sessionManagement->requestShutdown(); break;
        default: break;
    }

    return true;
}
