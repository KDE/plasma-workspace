/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "notification.h"
#include "server.h"

#include <QDBusServiceWatcher>
#include <QDateTime>
#include <QTimer>

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

    void removeRows(const QList<int> &rows);

    AbstractNotificationsModel *q;

    QList<Notification> notifications;
    // Fallback timeout to ensure all notifications expire eventually
    // otherwise when it isn't shown to the user and doesn't expire
    // an app might wait indefinitely for the notification to do so
    QHash<uint /*notificationId*/, QTimer *> notificationTimeouts;

    // Some apps clean up their own notifications on exit, but for
    // those that don't, we need to manually expire the notifications.
    // Otherwise, we will be left with notifications in the history with
    // buttons, making it seem as though they are interactive, even though
    // they are not.
    QDBusServiceWatcher notificationWatcher;

    QList<uint /*notificationId*/> pendingRemovals;
    QTimer pendingRemovalTimer;

    QDateTime lastRead;
    QWindow *window = nullptr;
};

}
