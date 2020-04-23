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

#ifndef ABSTRACTNOTIFICATIONSMODEL_H
#define ABSTRACTNOTIFICATIONSMODEL_H

#include <QAbstractListModel>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QDateTime>

#include "notifications.h"
#include "notification.h"
#include "server.h"

namespace NotificationManager
{

class Q_DECL_EXPORT AbstractNotificationsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ~AbstractNotificationsModel() override;

    QDateTime lastRead() const;
    void setLastRead(const QDateTime &lastRead);

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    virtual void expire(uint notificationId) = 0;
    virtual void close(uint notificationId) = 0;

    // Currently configure actions are not exposed in AbstractNotificationsModel to keep it very minimal
    // if usecase for this comes up in future, we can revisit it.
    
    virtual void invokeDefaultAction(uint notificationId) = 0;
    virtual void invokeAction(uint notificationId, const QString &actionName) = 0;
    virtual void reply(uint notificationId, const QString &text) = 0;

    void startTimeout(uint notificationId);
    void stopTimeout(uint notificationId);

    void clear(Notifications::ClearFlags flags);

signals:
    void lastReadChanged();

protected:
    AbstractNotificationsModel();
    void onNotificationAdded(const Notification &notification);
    void onNotificationReplaced(uint replacedId, const Notification &notification);
    void onNotificationRemoved(uint notificationId, Server::CloseReason reason);

    void setupNotificationTimeout(const Notification &notification);
    const QVector<Notification>& notifications();
    int rowOfNotification(uint id) const;


private:
    class Private;
    QScopedPointer<Private> d;

    Q_DISABLE_COPY(AbstractNotificationsModel)

};

} // namespace NotificationManager

#endif //ABSTRACTNOTIFICATIONSMODEL_H
