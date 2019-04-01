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

namespace NotificationManager
{

class NotificationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ~NotificationModel() override;

    using Ptr = QSharedPointer<NotificationModel>;
    static Ptr createNotificationModel();

    QDateTime lastRead() const;
    void setLastRead(const QDateTime &lastRead);

    // FIXME currently easier to debug if we crash when accessing invalid role
    // instead of just "randomly" returning display role when
    QVariant data(const QModelIndex &index, int role/* = Qt::DisplayRole*/) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // TODO should we have an API taking int row, or QModelIndex?
    Q_INVOKABLE void expire(uint notificationId);
    Q_INVOKABLE void close(uint notificationId);
    Q_INVOKABLE void configure(uint notificationId);
    Q_INVOKABLE void invokeDefaultAction(uint notificationId);
    // FIXME rename invokeAction
    Q_INVOKABLE void invoke(uint notificationId, const QString &actionName);

signals:
    void lastReadChanged();

private:
    class Private;
    QScopedPointer<Private> d;

    NotificationModel();
    Q_DISABLE_COPY(NotificationModel)

};

} // namespace NotificationManager
