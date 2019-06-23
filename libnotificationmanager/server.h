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

#include "notificationmanager_export.h"

namespace NotificationManager
{

class Notification;

class ServerPrivate;

/**
 * @short A notification DBus server
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NOTIFICATIONMANAGER_EXPORT Server : public QObject
{
    Q_OBJECT

public:
    ~Server() override;

    /**
     * The reason a notification was closed
     */
    enum class CloseReason {
        Expired = 1, ///< The notification timed out
        DismissedByUser = 2, ///< The user explicitly closed or acknowledged the notification
        Revoked = 3 ///< The notification was revoked by the issuing app because it is no longer relevant
    };
    Q_ENUM(CloseReason)

    static Server &self();

    /**
     * Registers the Notification Service on DBus.
     *
     * @return true if it succeeded, false otherwise.
     */
    bool init();

    /**
     * Whether the notification service could be registered
     */
    bool isValid() const;

    /**
     * Whether an application requested to inhibit notifications.
     */
    bool inhibited() const;

    // should we return a struct or pair or something?
    QStringList inhibitionApplications() const;
    QStringList inhibitionReasons() const;

    /**
     * Remove all inhibitions.
     *
     * @note The applications are not explicitly informed about this.
     */
    void clearInhibitions();

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

    /**
     * Adds a notification
     *
     * @note The notification isn't actually broadcast
     * but just emitted locally.
     *
     * @return the ID of the notification
     */
    uint add(const Notification &notification);

Q_SIGNALS:
    /**
     * Emitted when a notification was added.
     * This is emitted regardless of any filtering rules or user settings.
     * @param notification The notification
     */
    void notificationAdded(const Notification &notification);
    /**
     * Emitted when a notification is supposed to be updated
     * This is emitted regardless of any filtering rules or user settings.
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

    /**
     * Emitted when inhibitions have been changed. Becomes true
     * as soon as there is one inhibition and becomes false again
     * when all inhibitions have been lifted.
     */
    void inhibitedChanged(bool inhibited);

    /**
     * Emitted when the list of applications holding a notification
     * inhibition changes.
     * Normally you would only want to listen do @c inhibitedChanged
     */
    void inhibitionApplicationsChanged();

    /**
     * Emitted when the ownership of the Notification DBus Service is lost.
     */
    void serviceOwnershipLost();

private:
    explicit Server(QObject *parent = nullptr);
    Q_DISABLE_COPY(Server)
    // FIXME we also need to disable move and other stuff?

    QScopedPointer<ServerPrivate> d;
};

} // namespace NotificationManager
