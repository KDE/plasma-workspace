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

#include "notificationmodel.h"

#include "debug.h"

#include "notificationserver.h"

#include "notifications.h"

#include "notification.h"
#include "notification_p.h"

#include <QDebug>
#include <QProcess>

#include <KShell>

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

    int indexOfNotification(uint id) const;

    NotificationModel *q;

    QVector<Notification> notifications;

    QDateTime lastRead;

};

NotificationModel::Private::Private(NotificationModel *q)
    : q(q)
    , lastRead(QDateTime::currentDateTimeUtc())
{

}

NotificationModel::Private::~Private() = default;

void NotificationModel::Private::onNotificationAdded(const Notification &notification)
{
    // If we get the same notification in succession, just compress them into one
    if (!notifications.isEmpty()) {
        const Notification &lastNotification = notifications.constLast();
        if (lastNotification.applicationName() == notification.applicationName()
                && lastNotification.summary() == notification.summary()
                && lastNotification.body() == notification.body()
                && lastNotification.desktopEntry() == notification.desktopEntry()
                && lastNotification.applicationName() == notification.applicationName()) {
            onNotificationReplaced(lastNotification.id(), notification);
            return;
        }
    }

    q->beginInsertRows(QModelIndex(), notifications.count(), notifications.count());
    notifications.append(notification);
    q->endInsertRows();
}

void NotificationModel::Private::onNotificationReplaced(uint replacedId, const Notification &notification)
{
    const int row = indexOfNotification(replacedId);

    if (row == -1) {
        return;
    }

    notifications[row] = notification;
    const QModelIndex idx = q->index(row, 0);
    emit q->dataChanged(idx, idx);
}

void NotificationModel::Private::onNotificationRemoved(uint removedId, NotificationServer::CloseReason reason)
{
    const int row = indexOfNotification(removedId);
    if (row == -1) {
        return;
    }

    // When a notification expired, keep it around in the history and mark it as such
    if (reason == NotificationServer::CloseReason::Expired) {
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

NotificationModel::NotificationModel()
    : QAbstractListModel(nullptr)
    , d(new Private(this))
{
    connect(&NotificationServer::self(), &NotificationServer::notificationAdded, this, [this](const Notification &notification) {
        d->onNotificationAdded(notification);
    });
    connect(&NotificationServer::self(), &NotificationServer::notificationReplaced, this, [this](uint replacedId, const Notification &notification) {
        d->onNotificationReplaced(replacedId, notification);
    });
    connect(&NotificationServer::self(), &NotificationServer::notificationRemoved, this, [this](uint removedId, NotificationServer::CloseReason reason) {
        d->onNotificationRemoved(removedId, reason);
    });

    NotificationServer::self().init();
}

NotificationModel::~NotificationModel() = default;

NotificationModel::Ptr NotificationModel::createNotificationModel()
{
    static QWeakPointer<NotificationModel> s_instance;
    if (!s_instance) {
        QSharedPointer<NotificationModel> ptr(new NotificationModel());
        s_instance = ptr.toWeakRef();
        return ptr;
    }
    return s_instance.toStrongRef();
}

QDateTime NotificationModel::lastRead() const
{
    return d->lastRead;
}

void NotificationModel::setLastRead(const QDateTime &lastRead)
{
    if (d->lastRead != lastRead) {
        d->lastRead = lastRead;
        emit lastReadChanged();
    }
}

QVariant NotificationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= d->notifications.count()) {
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
    case Notifications::IconNameRole: return notification.iconName();
    case Notifications::ImageRole:
        if (!notification.image().isNull()) {
            return notification.image();
        }
        break;
    case Notifications::DesktopEntryRole: return notification.desktopEntry();
    case Notifications::NotifyRcNameRole: return notification.notifyRcName();

    case Notifications::ApplicationNameRole: return notification.applicationName();
    case Notifications::ApplicationIconNameRole: return notification.applicationIconName();
    case Notifications::DeviceNameRole: return notification.deviceName();

    case Notifications::ActionNamesRole: return notification.actionNames();
    case Notifications::ActionLabelsRole: return notification.actionLabels();
    case Notifications::HasDefaultActionRole: return notification.hasDefaultAction();
    case Notifications::DefaultActionLabelRole: return notification.defaultActionLabel();

    case Notifications::UrlsRole: return QVariant::fromValue(notification.urls());

    case Notifications::UrgencyRole: return static_cast<int>(notification.urgency());

    case Notifications::TimeoutRole: return notification.timeout();

    case Notifications::ConfigurableRole: return notification.configurable();
    case Notifications::ConfigureActionLabelRole: return notification.configureActionLabel();

    case Notifications::ExpiredRole: return notification.expired();
    }

    return QVariant();
}

int NotificationModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return d->notifications.count();
}

void NotificationModel::expire(uint notificationId)
{
    if (d->indexOfNotification(notificationId) > -1) {
        NotificationServer::self().closeNotification(notificationId, NotificationServer::CloseReason::Expired);
    }
}

void NotificationModel::close(uint notificationId)
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

    if (notification.d->hasConfigureAction) {
        NotificationServer::self().invokeAction(notificationId, QStringLiteral("settings")); // FIXME make a static Notification::configureActionName() or something
        return;
    }

    if (!notification.desktopEntry().isEmpty() || !notification.notifyRcName().isEmpty()) {
        // TODO would be nice to just have a signal but since NotificationModel is shared,
        // if we connect to this from Notifications you would get a signal in every instance
        // and potentialy open the config dialog multiple times.

        QStringList args;
        if (!notification.desktopEntry().isEmpty()) {
            args.append(QStringLiteral("--desktop-entry"));
            args.append(notification.desktopEntry());
        }
        if (!notification.notifyRcName().isEmpty()) {
            args.append(QStringLiteral("--notifyrc"));
            args.append(notification.notifyRcName());
        }
        if (!notification.d->eventId.isEmpty()) {
            args.append(QStringLiteral("--event-id"));
            args.append(notification.d->eventId);
        }

        QProcess::startDetached(QStringLiteral("kcmshell5"), {
            QStringLiteral("notifications"),
            QStringLiteral("--args"),
            KShell::joinArgs(args)
        });
        return;
    }

    qCWarning(NOTIFICATIONMANAGER) << "Trying to configure notification" << notificationId << "which isn't configurable";
}

void NotificationModel::invokeDefaultAction(uint notificationId)
{
    const int idx = d->indexOfNotification(notificationId);
    if (idx == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(idx);
    if (!notification.hasDefaultAction()) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to invoke default action on notification" << notificationId << "which doesn't have one";
        return;
    }

    NotificationServer::self().invokeAction(notificationId, QStringLiteral("default")); // FIXME make a static Notification::defaultActionName() or something
}

void NotificationModel::invokeAction(uint notificationId, const QString &actionName)
{
    const int idx = d->indexOfNotification(notificationId);
    if (idx == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(idx);
    if (!notification.actionNames().contains(actionName)) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to invoke action" << actionName << "on notification" << notificationId << "which it doesn't have";
        return;
    }

    NotificationServer::self().invokeAction(notificationId, actionName);
}

void NotificationModel::clear(Notifications::ClearFlags flags)
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

        bool clear = (flags.testFlag(Notifications::ClearExpired) && notification.expired())
                || (flags.testFlag(Notifications::ClearDismissed) && notification.dismissed());

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

    qDebug() << "clearing the following ranges" << clearQueue;
}
