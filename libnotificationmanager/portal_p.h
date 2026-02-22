/*
 * SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <QDBusContext>
#include <QHash>
#include <QObject>

#include "notification.h"

namespace NotificationManager
{

class Q_DECL_HIDDEN Portal : public QObject, protected QDBusContext
{
    Q_OBJECT

    // DBus
    Q_PROPERTY(uint version READ version)

public:
    static Portal &self();
    ~Portal() override;

    // libnotificationmanager is built around notification IDs being uint,
    // so use the highest bit to tell portal from fdo notifications.
    static uint portalIdMask();
    static bool isPortalNotificationId(uint notificationId);

    bool init();

    void removeNotification(uint notificationId);
    void invokeAction(uint notificationId, const QString &action, const QVariant &target, const QString &response, QWindow *window);

    // DBus
    void AddNotification(const QString &appId, const QString &notificationId, const QVariantMap &notification);
    void RemoveNotification(const QString &appId, const QString &notificationId);
    uint version() const;

Q_SIGNALS:
    void notificationAdded(const Notification &notification);
    void notificationReplaced(uint replacedId, const Notification &notification);
    void notificationRemoved(uint id);

    // DBus
    void ActionInvoked(const QString &appId, const QString &NotificationId, const QString &action, const QVariantList &parameter);

private:
    Portal(QObject *parent = nullptr);

    // TODO maybe use a struct insted of QPair?
    QHash<uint, QPair<QString, QString>> m_idMap;
    uint m_highestNotificationId = 0;

    bool m_valid = false;
    bool m_dbusObjectValid = false;
};

} // namespace NotificationManager
