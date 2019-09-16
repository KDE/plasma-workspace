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

#pragma once

#include <QAbstractListModel>
#include <QScopedPointer>
#include <QSharedPointer>

#include "notifications.h"

namespace NotificationManager
{

class NotificationsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ~NotificationsModel() override;

    using Ptr = QSharedPointer<NotificationsModel>;
    static Ptr createNotificationsModel();

    QDateTime lastRead() const;
    void setLastRead(const QDateTime &lastRead);

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    void expire(uint notificationId);
    void close(uint notificationId);
    void configure(uint notificationId);
    void configure(const QString &desktopEntry, const QString &notifyRcName, const QString &eventId);
    void invokeDefaultAction(uint notificationId);
    void invokeAction(uint notificationId, const QString &actionName);

    void startTimeout(uint notificationId);
    void stopTimeout(uint notificationId);

    void clear(Notifications::ClearFlags flags);

signals:
    void lastReadChanged();

private:
    class Private;
    QScopedPointer<Private> d;

    NotificationsModel();
    Q_DISABLE_COPY(NotificationsModel)

};

} // namespace NotificationManager
