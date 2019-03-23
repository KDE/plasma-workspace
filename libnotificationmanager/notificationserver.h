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

#pragma once

#include <QObject>
#include <QDBusContext>
#include <QVector>

#include "notificationmanager_export.h"

namespace NotificationManager
{

class Notification;

class NotificationServerPrivate;

/**
 * @short Registers a notification server on the DBus
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NOTIFICATIONMANAGER_EXPORT NotificationServer : public QObject
{
    Q_OBJECT

public:
    ~NotificationServer() override;

    /**
     * The reason a notification was closed
     */
    enum class CloseReason {
        Expired = 1, ///< The notification timed out
        DismissedByUser = 2, ///< The user explicitly closed or acknowledged the notification
        Revoked = 3 ///< The notification was revoked by the issuing app because it is no longer relevant
    };
    Q_ENUM(CloseReason)

    static NotificationServer &self();

    /**
     * Whether the notification service could be registered
     */
    // TODO add a retry or reconnect thing
    bool isValid() const;

    /**
     * Sends a notification closed event
     *
     * @param id The notification ID
     * @reason The reason why it was closed
     */
    void closeNotification(uint id, CloseReason reason);
    /**
     * Sends an action invocation request
     *
     * @param id The notification ID
     * @param actionName The name of the action, e.g. "Action 1", or "default"
     */
    void invokeAction(uint id, const QString &actionName);

Q_SIGNALS:
    /**
     * Emitted when a notification was added
     * @param notification The notification
     */
    void notificationAdded(const Notification &notification);
    /**
     * Emitted when a notification is supposed to be updated
     * @param replacedId The ID of the notification it replaces
     * @param notification The new notification to use instead
     */
    void notificationReplaced(uint replacedId, const Notification &notification);
    /**
     * Emitted when a notification got removed (closed)
     * @param id The notification ID
     * @param reason The reason why it was closed
     */
    void notificationRemoved(uint id, CloseReason reason);

private:
    explicit NotificationServer(QObject *parent = nullptr);
    Q_DISABLE_COPY(NotificationServer)
    // FIXME we also need to disable move and other stuff?

    class Private;
    QScopedPointer<NotificationServerPrivate> d;
};

} // namespace NotificationManager
