/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "inhibitionserver.h"

#include "kdenotificationsadaptor.h"

#include "notification.h"

#include <QDBusConnection>

using namespace NotificationManager;

InhibitionServer::InhibitionServer(QObject *parent)
    : QObject(parent)
{
    new InhibitionAdaptor(this);

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/kde/Notifications"), this);
    if (!QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Notifications.Inhibit"))) {
        qWarning() << "Failed to register Notification Inhibit service";
    }
}

InhibitionServer::~InhibitionServer() = default;

InhibitionServer &InhibitionServer::self()
{
    static InhibitionServer s_self;
    return s_self;
}

QDBusUnixFileDescriptor InhibitionServer::Inhibit(const QString &app_name, const QString &reason, const QVariantMap &hints)
{
    qDebug() << "INHIBIT" << app_name << reason << hints;
    return QDBusUnixFileDescriptor();
}
