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

//#include "notificationmanager_export.h"

namespace NotificationManager
{

class Notification;

/**
 * @short Registers a notification server on the DBus
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NotificationServer : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    ~NotificationServer() override;

    enum class CloseReason {
        Expired = 1,
        DismissedByUser = 2,
        Revoked = 3
    };
    Q_ENUM(CloseReason)

    static NotificationServer &self();

    void closeNotification(uint id, CloseReason reason);
    void invokeAction(uint id, const QString &actionName);

    // DBus
    uint Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int timeout);
    void CloseNotification(uint id);
    QStringList GetCapabilities() const;
    QString GetServerInformation(QString &vendor, QString &version, QString &specVersion) const;

Q_SIGNALS:
    // FIXME added vs closed, should it be shown? opened? created?
    void notificationAdded(const Notification &notification);
    void notificationReplaced(uint replacedId, const Notification &notification);
    void notificationRemoved(uint id, CloseReason reason);

    // DBus
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);

private:
    explicit NotificationServer(QObject *parent = nullptr);
    Q_DISABLE_COPY(NotificationServer)
    // FIXME we also need to disable move and other stuff?

    //QVector<uint> m_knownNotificationIds;
    uint m_highestNotificationId = 0;
};

} // namespace NotificationManager
