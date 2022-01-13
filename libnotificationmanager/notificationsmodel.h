/*
    SPDX-FileCopyrightText: 2020 Shah Bhushan <bshah@kde.org>
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "abstractnotificationsmodel.h"

namespace NotificationManager
{
class Q_DECL_EXPORT NotificationsModel : public AbstractNotificationsModel
{
public:
    using Ptr = QSharedPointer<NotificationsModel>;
    static Ptr createNotificationsModel();
    void expire(uint notificationId) override;
    void close(uint notificationId) override;

    void invokeDefaultAction(uint notificationId, Notifications::InvokeBehavior behavior) override;
    void invokeAction(uint notificationId, const QString &actionName, Notifications::InvokeBehavior behavior) override;
    void reply(uint notificationId, const QString &text, Notifications::InvokeBehavior behavior) override;

    void configure(uint notificationId);
    void configure(const QString &desktopEntry, const QString &notifyRcName, const QString &eventId);

private:
    NotificationsModel();
};

}
