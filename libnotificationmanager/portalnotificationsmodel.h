/*
 * SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include "abstractnotificationsmodel.h"

namespace NotificationManager
{
class PortalNotificationsModel : public AbstractNotificationsModel
{
public:
    using Ptr = std::shared_ptr<PortalNotificationsModel>;
    static Ptr create();

    void expire(uint notificationId) override;
    void close(uint notificationId) override;

    void invokeDefaultAction(uint notificationId, Notifications::InvokeBehavior behavior) override;
    void invokeAction(uint notificationId, const QString &actionName, Notifications::InvokeBehavior behavior) override;
    void reply(uint notificationId, const QString &text, Notifications::InvokeBehavior behavior) override;

    void configure(uint notificationId);
    void configure(const QString &desktopEntry, const QString &notifyRcName, const QString &eventId);

private:
    PortalNotificationsModel();
};

}
