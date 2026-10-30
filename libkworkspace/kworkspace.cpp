/*
    SPDX-FileCopyrightText: 2024 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kworkspace.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QVariant>

bool KWorkSpace::isUsingSystemdBoot()
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                              QStringLiteral("/org/freedesktop/systemd1"),
                                              QStringLiteral("org.freedesktop.systemd1.Manager"),
                                              QStringLiteral("GetUnit"));
    msg << QStringLiteral("plasma-workspace.target");

    // "GetUnit" also succeeds for units that exist but are not running, so
    // the state of the unit has to be checked as well.
    const QDBusReply<QDBusObjectPath> reply = QDBusConnection::sessionBus().call(msg);
    if (!reply.isValid()) {
        return false;
    }

    msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                         reply.value().path(),
                                         QStringLiteral("org.freedesktop.DBus.Properties"),
                                         QStringLiteral("Get"));
    msg << QStringLiteral("org.freedesktop.systemd1.Unit") << QStringLiteral("ActiveState");

    const QDBusReply<QVariant> stateReply = QDBusConnection::sessionBus().call(msg);
    return stateReply.isValid() && stateReply.value().toString() == QLatin1String("active");
}
