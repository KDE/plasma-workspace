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

#include "abstractnotificationsmodel.h"
#include "abstractnotificationsmodel_p.h"
#include "debug.h"

#include "server.h"
#include "utils_p.h"

#include "notifications.h"

#include "notification.h"
#include "notification_p.h"

#include <QDebug>
#include <QProcess>
#include <QTimer>

#include <KShell>

#include <algorithm>
#include <functional>

static const int s_notificationsLimit = 1000;

using namespace NotificationManager;

AbstractNotificationsModel::Private::Private(AbstractNotificationsModel *q)
    : q(q)
    , lastRead(QDateTime::currentDateTimeUtc())
{

}

AbstractNotificationsModel::Private::~Private()
{
    qDeleteAll(notificationTimeouts);
    notificationTimeouts.clear();
}

void AbstractNotificationsModel::Private::onNotificationAdded(const Notification &notification)
{
    // Once we reach a certain insane number of notifications discard some old ones
    // as we keep pixmaps around etc
    if (notifications.count() >= s_notificationsLimit) {
        const int cleanupCount = s_notificationsLimit / 2;
        qCDebug(NOTIFICATIONMANAGER) << "Reached the notification limit of" << s_notificationsLimit << ", discarding the oldest" << cleanupCount << "notifications";
        q->beginRemoveRows(QModelIndex(), 0, cleanupCount - 1);
        for (int i = 0 ; i < cleanupCount; ++i) {
            notifications.removeAt(0);
            // TODO close gracefully?
        }
        q->endRemoveRows();
    }

    setupNotificationTimeout(notification);

    q->beginInsertRows(QModelIndex(), notifications.count(), notifications.count());
    notifications.append(std::move(notification));
    q->endInsertRows();
}

void AbstractNotificationsModel::Private::onNotificationReplaced(uint replacedId, const Notification &notification)
{
    const int row = q->rowOfNotification(replacedId);

    if (row == -1) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to replace notification with id" << replacedId << "which doesn't exist, creating a new one. This is an application bug!";
        onNotificationAdded(notification);
        return;
    }

    setupNotificationTimeout(notification);

    notifications[row] = notification;
    const QModelIndex idx = q->index(row, 0);
    emit q->dataChanged(idx, idx);
}

void AbstractNotificationsModel::Private::onNotificationRemoved(uint removedId, Server::CloseReason reason)
{
    const int row = q->rowOfNotification(removedId);
    if (row == -1) {
        return;
    }

    q->stopTimeout(removedId);

    // When a notification expired, keep it around in the history and mark it as such
    if (reason == Server::CloseReason::Expired) {
        const QModelIndex idx = q->index(row, 0);

        Notification &notification = notifications[row];
        notification.setExpired(true);

        // Since the notification is "closed" it cannot have any actions
        // unless it is "resident" which we don't support
        notification.setActions(QStringList());

        emit q->dataChanged(idx, idx, {
            Notifications::ExpiredRole,
            // TODO only emit those if actually changed?
            Notifications::ActionNamesRole,
            Notifications::ActionLabelsRole,
            Notifications::HasDefaultActionRole,
            Notifications::DefaultActionLabelRole,
            Notifications::ConfigurableRole
        });

        return;
    }

    // Otherwise if explicitly closed by either user or app, remove it

    q->beginRemoveRows(QModelIndex(), row, row);
    notifications.removeAt(row);
    q->endRemoveRows();
}

void AbstractNotificationsModel::Private::setupNotificationTimeout(const Notification &notification)
{
    if (notification.timeout() == 0) {
        // In case it got replaced by a persistent notification
        q->stopTimeout(notification.id());
        return;
    }

    QTimer *timer = notificationTimeouts.value(notification.id());
    if (!timer) {
        timer = new QTimer();
        timer->setSingleShot(true);

        connect(timer, &QTimer::timeout, q, [this, timer] {
            const uint id = timer->property("notificationId").toUInt();
            q->expire(id);
        });
        notificationTimeouts.insert(notification.id(), timer);
    }

    timer->stop();
    timer->setProperty("notificationId", notification.id());
    timer->setInterval(60000 /*1min*/ + (notification.timeout() == -1 ? 120000 /*2min, max configurable default timeout*/ : notification.timeout()));
    timer->start();
}

int AbstractNotificationsModel::rowOfNotification(uint id) const
{
    auto it = std::find_if(d->notifications.constBegin(), d->notifications.constEnd(), [id](const Notification &item) {
        return item.id() == id;
    });

    if (it == d->notifications.constEnd()) {
        return -1;
    }

    return std::distance(d->notifications.constBegin(), it);
}

AbstractNotificationsModel::AbstractNotificationsModel()
    : QAbstractListModel(nullptr)
    , d(new Private(this))
{
}

AbstractNotificationsModel::~AbstractNotificationsModel() = default;

QDateTime AbstractNotificationsModel::lastRead() const
{
    return d->lastRead;
}

void AbstractNotificationsModel::setLastRead(const QDateTime &lastRead)
{
    if (d->lastRead != lastRead) {
        d->lastRead = lastRead;
        emit lastReadChanged();
    }
}

QVariant AbstractNotificationsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    const Notification &notification = d->notifications.at(index.row());

    switch (role) {
    case Notifications::IdRole: return notification.id();
    case Notifications::TypeRole: return Notifications::NotificationType;

    case Notifications::CreatedRole:
        if (notification.created().isValid()) {
            return notification.created();
        }
        break;
    case Notifications::UpdatedRole:
        if (notification.updated().isValid()) {
            return notification.updated();
        }
        break;
    case Notifications::SummaryRole: return notification.summary();
    case Notifications::BodyRole: return notification.body();
    case Notifications::IconNameRole:
        if (notification.image().isNull()) {
            return notification.icon();
        }
        break;
    case Notifications::ImageRole:
        if (!notification.image().isNull()) {
            return notification.image();
        }
        break;
    case Notifications::DesktopEntryRole: return notification.desktopEntry();
    case Notifications::NotifyRcNameRole: return notification.notifyRcName();

    case Notifications::ApplicationNameRole: return notification.applicationName();
    case Notifications::ApplicationIconNameRole: return notification.applicationIconName();
    case Notifications::OriginNameRole: return notification.originName();

    case Notifications::ActionNamesRole: return notification.actionNames();
    case Notifications::ActionLabelsRole: return notification.actionLabels();
    case Notifications::HasDefaultActionRole: return notification.hasDefaultAction();
    case Notifications::DefaultActionLabelRole: return notification.defaultActionLabel();

    case Notifications::UrlsRole: return QVariant::fromValue(notification.urls());

    case Notifications::UrgencyRole: return static_cast<int>(notification.urgency());
    case Notifications::UserActionFeedbackRole: return notification.userActionFeedback();

    case Notifications::TimeoutRole: return notification.timeout();

    case Notifications::ClosableRole: return true;
    case Notifications::ConfigurableRole: return notification.configurable();
    case Notifications::ConfigureActionLabelRole: return notification.configureActionLabel();

    case Notifications::ExpiredRole: return notification.expired();
    case Notifications::ReadRole: return notification.read();

    case Notifications::HasReplyActionRole: return notification.hasReplyAction();
    case Notifications::ReplyActionLabelRole: return notification.replyActionLabel();
    case Notifications::ReplyPlaceholderTextRole: return notification.replyPlaceholderText();
    case Notifications::ReplySubmitButtonTextRole: return notification.replySubmitButtonText();
    case Notifications::ReplySubmitButtonIconNameRole: return notification.replySubmitButtonIconName();
    }

    return QVariant();
}

bool AbstractNotificationsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        return false;
    }

    Notification &notification = d->notifications[index.row()];

    switch (role) {
    case Notifications::ReadRole:
        if (value.toBool() != notification.read()) {
            notification.setRead(value.toBool());
            return true;
        }
        break;
    }

    return false;
}

int AbstractNotificationsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return d->notifications.count();
}

QHash<int, QByteArray> AbstractNotificationsModel::roleNames() const
{
    return Utils::roleNames();
}

void AbstractNotificationsModel::startTimeout(uint notificationId)
{
    const int row = rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(row);

    if (!notification.timeout() || notification.expired()) {
        return;
    }

    d->setupNotificationTimeout(notification);
}

void AbstractNotificationsModel::stopTimeout(uint notificationId)
{
    delete d->notificationTimeouts.take(notificationId);
}

void AbstractNotificationsModel::clear(Notifications::ClearFlags flags)
{
    if (d->notifications.isEmpty()) {
        return;
    }

    // Tries to remove a contiguous group if possible as the likely case is
    // you have n unread notifications at the end of the list, we don't want to
    // remove and signal each item individually
    QVector<QPair<int, int>> clearQueue;

    QPair<int, int> clearRange{-1, -1};

    for (int i = d->notifications.count() - 1; i >= 0; --i) {
        const Notification &notification = d->notifications.at(i);

        bool clear = (flags.testFlag(Notifications::ClearExpired) && notification.expired());

        if (clear) {
            if (clearRange.second == -1) {
                clearRange.second = i;
            }
            clearRange.first = i;
        } else {
            if (clearRange.first != -1) {
                clearQueue.append(clearRange);
                clearRange.first = -1;
                clearRange.second = -1;
            }
        }
    }

    if (clearRange.first != -1) {
        clearQueue.append(clearRange);
        clearRange.first = -1;
        clearRange.second = -1;
    }

    for (const auto &range : clearQueue) {
        beginRemoveRows(QModelIndex(), range.first, range.second);
        for (int i = range.second; i >= range.first; --i) {
            d->notifications.removeAt(i);
        }
        endRemoveRows();
    }
}

void AbstractNotificationsModel::onNotificationAdded(const Notification &notification)
{
    d->onNotificationAdded(notification);
}

void AbstractNotificationsModel::onNotificationReplaced(uint replacedId, const Notification &notification)
{
    d->onNotificationReplaced(replacedId, notification);
}

void AbstractNotificationsModel::onNotificationRemoved(uint notificationId, Server::CloseReason reason)
{
    d->onNotificationRemoved(notificationId, reason);
}

void AbstractNotificationsModel::setupNotificationTimeout(const Notification &notification)
{
    d->setupNotificationTimeout(notification);
}

const QVector<Notification>& AbstractNotificationsModel::notifications()
{
    return d->notifications;
}
