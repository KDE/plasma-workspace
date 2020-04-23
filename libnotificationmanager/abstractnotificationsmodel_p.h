/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

#ifndef ABSTRACTNOTIFICATIONSMODEL_P_H
#define ABSTRACTNOTIFICATIONSMODEL_P_H

#include "notification.h"
#include "server.h"

#include <QDateTime>

namespace NotificationManager
{

class Q_DECL_HIDDEN AbstractNotificationsModel::Private
{
public:
    explicit Private(AbstractNotificationsModel *q);
    ~Private();

    void onNotificationAdded(const Notification &notification);
    void onNotificationReplaced(uint replacedId, const Notification &notification);
    void onNotificationRemoved(uint notificationId, Server::CloseReason reason);

    void setupNotificationTimeout(const Notification &notification);

    AbstractNotificationsModel *q;

    QVector<Notification> notifications;
    // Fallback timeout to ensure all notifications expire eventually
    // otherwise when it isn't shown to the user and doesn't expire
    // an app might wait indefinitely for the notification to do so
    QHash<uint /*notificationId*/, QTimer*> notificationTimeouts;

    QDateTime lastRead;

};

}

#endif // ABSTRACTNOTIFICATIONSMODEL_P_H
