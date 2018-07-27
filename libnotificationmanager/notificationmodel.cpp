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

#include "notificationmodel.h"

#include "notificationserver.h"

#include "notification.h"

#include <QDebug>

#include <algorithm>
#include <functional>

using namespace NotificationManager;

class Q_DECL_HIDDEN NotificationModel::Private
{
public:
    explicit Private(NotificationModel *q);
    ~Private();

    void onNotificationAdded(const Notification &notification);
    void onNotificationReplaced(uint replacedId, const Notification &notification);
    void onNotificationRemoved(uint notificationId, NotificationServer::CloseReason reason);

    QVector<Notification> notifications;

    int indexOfNotification(uint id) const;

private:
    NotificationModel *q;

};

NotificationModel::Private::Private(NotificationModel *q)
    : q(q)
{

}

NotificationModel::Private::~Private() = default;

void NotificationModel::Private::onNotificationAdded(const Notification &notification)
{
    q->beginInsertRows(QModelIndex(), notifications.count(), notifications.count());
    notifications.append(notification);
    q->endInsertRows();
}

void NotificationModel::Private::onNotificationReplaced(uint replacedId, const Notification &notification)
{
    // Completely remove the old notification and add the new one

    const int row = indexOfNotification(replacedId);

    if (row > -1) {
        q->beginRemoveRows(QModelIndex(), row, row);
        notifications.removeAt(row); // erase or something but we need the index for beginRemoveRows anyway
        q->endRemoveRows();
    }

    // now insert the new notification
    onNotificationAdded(notification);
}

void NotificationModel::Private::onNotificationRemoved(uint removedId, NotificationServer::CloseReason reason)
{
    qDebug() << "Notification" << removedId << "closed because" << reason;

    const int row = indexOfNotification(removedId);
    if (row == -1) {
        return;
    }

    // When a notification expired, keep it around in the history and mark it as such
    if (reason == NotificationServer::CloseReason::Expired) {
        const QModelIndex idx = q->index(row, 0);

        Notification &notification = notifications[row];
        notification.setExpired(true);
        emit q->dataChanged(idx, idx, {NotificationModel::ExpiredRole});

        return;
    }

    // Otherwise if explicitly closed by either user or app, remove it

    q->beginRemoveRows(QModelIndex(), row, row);
    notifications.removeAt(row);
    q->endRemoveRows();
}

int NotificationModel::Private::indexOfNotification(uint id) const
{
    auto it = std::find_if(notifications.constBegin(), notifications.constEnd(), [id](const Notification &item) {
        return item.id() == id;
    });

    if (it == notifications.constEnd()) {
        return -1;
    }

    return std::distance(notifications.constBegin(), it);
}

NotificationModel::NotificationModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new Private(this))
{
    connect(&NotificationServer::self(), &NotificationServer::notificationAdded, this, [this](const Notification &notification) {
        // why doesn't std::bind(&NotificationModel::Private::onNotificationAdded, d.data())); work?!
        d->onNotificationAdded(notification);
    });
    connect(&NotificationServer::self(), &NotificationServer::notificationReplaced, this, [this](uint replacedId, const Notification &notification) {
        d->onNotificationReplaced(replacedId, notification);
    });
    connect(&NotificationServer::self(), &NotificationServer::notificationRemoved, this, [this](uint removedId, NotificationServer::CloseReason reason) {
        d->onNotificationRemoved(removedId, reason);
    });
}

NotificationModel::~NotificationModel() = default;

/*NotificationServer &NotificationServer::self()
{
    static NotificationServer s_self;
    return s_self;
}*/

QVariant NotificationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= d->notifications.count()) {
        return QVariant();
    }

    const Notification &notification = d->notifications.at(index.row());

    switch (role) {
    case IdRole: return notification.id();
    case CreatedRole: return notification.created();
    case SummaryRole: return notification.summary();
    case BodyRole: return notification.body();
    case IconNameRole: return notification.iconName();
    case ImageRole: return notification.image();
    case ApplicationNameRole: return notification.applicationName();
    case ApplicationIconNameRole: return notification.applicationIconName();

    case ActionNamesRole: return notification.actionNames();
    case ActionLabelsRole: return notification.actionLabels();
    case HasDefaultActionRole: return notification.hasDefaultAction();

    case UrlsRole: return QVariant::fromValue(notification.urls());
    case UrgencyRole: return static_cast<int>(notification.urgency()); // FIXME?
    //case UrgencyRole: return QVariant::fromValue(notification.urgency());

    case TimeoutRole: return notification.timeout();
    case PersistentRole: return notification.isPersistent(); // WHY is

        // FIXME why "is"
    case IsConfigurableRole: return notification.isConfigurable();
    case ConfigureActionLabelRole: return notification.configureActionLabel();

    case ExpiredRole: return notification.expired();
    case SeenRole: return notification.seen();

    default: Q_UNREACHABLE(); // FIXME
    }

    return QVariant();
}

bool NotificationModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= d->notifications.count()) {
        return false;
    }

    Notification &notification = d->notifications[index.row()];

    bool changed = false;

    switch (role) {
    /*case ExpiredRole: {
        const bool expired = value.toBool();
        if (notification.expired() != expired) {
            notification.setExpired(expired);
            changed = true;
        }
        break;
    }*/
    case SeenRole: {
        const bool seen = value.toBool();
        if (notification.seen() == seen) {
            notification.setSeen(seen);
            changed = true;
        }
    }
    }

    if (changed) {
        emit dataChanged(index, index, {role});
    }

    return changed;
}

int NotificationModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return d->notifications.count();
}

QHash<int, QByteArray> NotificationModel::roleNames() const
{
    return QHash<int, QByteArray> {
        {IdRole, QByteArrayLiteral("notificationId")}, // id is QML-reserved
        {CreatedRole, QByteArrayLiteral("created")},
        {SummaryRole, QByteArrayLiteral("summary")},
        {BodyRole, QByteArrayLiteral("body")},
        {IconNameRole, QByteArrayLiteral("iconName")},
        {ImageRole, QByteArrayLiteral("image")},
        {ApplicationNameRole, QByteArrayLiteral("applicationName")},
        {ApplicationIconNameRole, QByteArrayLiteral("applicationIconName")},

        {ActionNamesRole, QByteArrayLiteral("actionNames")},
        {ActionLabelsRole, QByteArrayLiteral("actionLabels")},
        {HasDefaultActionRole, QByteArrayLiteral("hasDefaultAction")},

        {UrlsRole, QByteArrayLiteral("urls")},
        {UrgencyRole, QByteArrayLiteral("urgency")},
        {TimeoutRole, QByteArrayLiteral("timeout")},
        {PersistentRole, QByteArrayLiteral("persistent")},

        // FIXME why "is"
        {IsConfigurableRole, QByteArrayLiteral("isConfigurable")},
        {ConfigureActionLabelRole, QByteArrayLiteral("configureActionLabel")},

        {ExpiredRole, QByteArrayLiteral("expired")},
        {SeenRole, QByteArrayLiteral("seen")}
    };
}

void NotificationModel::expire(uint notificationId)
{
    if (d->indexOfNotification(notificationId) > -1) {
        NotificationServer::self().closeNotification(notificationId, NotificationServer::CloseReason::Expired);
    }
}

void NotificationModel::dismiss(uint notificationId)
{
    if (d->indexOfNotification(notificationId) > -1) {
        NotificationServer::self().closeNotification(notificationId, NotificationServer::CloseReason::DismissedByUser);
    }
}

void NotificationModel::configure(uint notificationId)
{
    const int idx = d->indexOfNotification(notificationId);
    if (idx == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(idx);

    if (notification.m_hasConfigureAction) {
        NotificationServer::self().invokeAction(notificationId, QStringLiteral("settings")); // FIXME make a static Notification::configureActionName() or something
        return;
    }

    if (!notification.m_notifyRcName.isEmpty()) {
        // TODO show knotifyconfigwidget thingie or emit a signal so we don't have any widget deps in this lib
        qDebug() << "IMPLEMENT ME configure" << notificationId << "of" << notification.m_notifyRcName;
        return;
    }

    qWarning() << "Trying to configure notification" << notificationId << "which isn't configurable";
}

void NotificationModel::invokeDefaultAction(uint notificationId)
{
    const int idx = d->indexOfNotification(notificationId);
    if (idx == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(idx);
    if (!notification.hasDefaultAction()) {
        qWarning() << "Trying to invoke default action on notification" << notificationId << "which doesn't have one";
        return;
    }

    NotificationServer::self().invokeAction(notificationId, QStringLiteral("default")); // FIXME make a static Notification::defaultActionName() or something
}

void NotificationModel::invoke(uint notificationId, const QString &actionName)
{
    const int idx = d->indexOfNotification(notificationId);
    if (idx == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(idx);
    if (!notification.actionNames().contains(actionName)) {
        qWarning() << "Trying to invoke action" << actionName << "on notification" << notificationId << "which it doesn't have";
        return;
    }

    NotificationServer::self().invokeAction(notificationId, actionName);
}
