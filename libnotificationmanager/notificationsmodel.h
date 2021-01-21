/*
 * Copyright 2020 Shah Bhushan <bshah@kde.org>
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

    void invokeDefaultAction(uint notificationId) override;
    void invokeAction(uint notificationId, const QString &actionName) override;
    void reply(uint notificationId, const QString &text) override;

    void configure(uint notificationId);
    void configure(const QString &desktopEntry, const QString &notifyRcName, const QString &eventId);

private:
    NotificationsModel();
};

}
