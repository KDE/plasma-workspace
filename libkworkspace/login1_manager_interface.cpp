/*
 *   Copyright (C) 2019 David Edmundson <daviedmundson@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// This file overrides the auto-generated login1_manager_interface.cpp in order
// change the interface name and act as a completely transparent Logind1 / CK2 wrapper

#include "login1_manager_interface.h"
#include "sessionmanagementbackend.h"

enum LogindProvider {
    Unknown,
    None,
    ConsoleKit2,
    Logind1,
};
static LogindProvider s_mode = Unknown;

static LogindProvider getLogindMode()
{
    if (s_mode != Unknown) {
        return s_mode;
    }
    if (QDBusConnection::systemBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.login1"))) {
        s_mode = Logind1;
    } else if (QDBusConnection::systemBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.ConsoleKit"))) {
        s_mode = ConsoleKit2;
    } else {
        s_mode = None;
    }
    return s_mode;
}

bool LogindSessionBackend::exists()
{
    return getLogindMode() != None;
}

OrgFreedesktopLogin1ManagerInterface::OrgFreedesktopLogin1ManagerInterface(const QString &, const QString &, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(getLogindMode() == Logind1 ? QStringLiteral("org.freedesktop.login1") : QStringLiteral("org.freedesktop.ConsoleKit"),
                             getLogindMode() == Logind1 ? QStringLiteral("/org/freedesktop/login1") : QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
                             getLogindMode() == Logind1 ? "org.freedesktop.login1.Manager" : "org.freedesktop.ConsoleKit.Manager",
                             connection,
                             parent)
{
}

OrgFreedesktopLogin1ManagerInterface::~OrgFreedesktopLogin1ManagerInterface()
{
}

#include "login1_manager_interface.moc"
