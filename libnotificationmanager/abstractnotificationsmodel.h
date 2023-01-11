/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QWindow>

#include "notification.h"
#include "notifications.h"
#include "server.h"

namespace NotificationManager
{
class Q_DECL_EXPORT AbstractNotificationsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)

public:
    ~AbstractNotificationsModel() override;

    QDateTime lastRead() const;
    void setLastRead(const QDateTime &lastRead);

    QWindow *window() const;
    void setWindow(QWindow *window);

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    virtual void expire(uint notificationId) = 0;
    virtual void close(uint notificationId) = 0;

    // Currently configure actions are not exposed in AbstractNotificationsModel to keep it very minimal
    // if usecase for this comes up in future, we can revisit it.

    virtual void invokeDefaultAction(uint notificationId, Notifications::InvokeBehavior behavior) = 0;
    virtual void invokeAction(uint notificationId, const QString &actionName, Notifications::InvokeBehavior behavior) = 0;
    virtual void reply(uint notificationId, const QString &text, Notifications::InvokeBehavior behavior) = 0;

    void startTimeout(uint notificationId);
    void stopTimeout(uint notificationId);

    void clear(Notifications::ClearFlags flags);

Q_SIGNALS:
    void lastReadChanged();
    void windowChanged(QWindow *window);

protected:
    AbstractNotificationsModel();
    void onNotificationAdded(const Notification &notification);
    void onNotificationReplaced(uint replacedId, const Notification &notification);
    void onNotificationRemoved(uint notificationId, Server::CloseReason reason);

    void setupNotificationTimeout(const Notification &notification);
    const QVector<Notification> &notifications();
    int rowOfNotification(uint id) const;

private:
    friend class NotificationTest;

    class Private;
    QScopedPointer<Private> d;

    Q_DISABLE_COPY(AbstractNotificationsModel)
};

} // namespace NotificationManager
