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

#include "notificationserver.h"
#include "notificationserver_p.h"

#include "notification.h"

#include <QDBusConnection>

using namespace NotificationManager;

NotificationServer::NotificationServer(QObject *parent)
    : QObject(parent)
    , d(new NotificationServerPrivate(this))
{

}

NotificationServer::~NotificationServer() = default;

NotificationServer &NotificationServer::self()
{
    static NotificationServer s_self;
    return s_self;
}

bool NotificationServer::isValid() const
{
    return d->m_valid;
}

void NotificationServer::closeNotification(uint notificationId, CloseReason reason)
{
    emit notificationRemoved(notificationId, reason);

    emit d->NotificationClosed(notificationId, static_cast<int>(reason)); // tell on DBus
}

void NotificationServer::invokeAction(uint notificationId, const QString &actionName)
{
    emit d->ActionInvoked(notificationId, actionName);
}
