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

#include "notificationsmodel.h"

#include "debug.h"

#include "server.h"

#include "notifications.h"

#include "notification.h"
#include "notification_p.h"

#include <QDebug>
#include <QProcess>
#include <QTimer>

#include <KShell>

#include <algorithm>
#include <functional>

using namespace NotificationManager;

class Q_DECL_HIDDEN NotificationsModel::Private
{
public:
    explicit Private(NotificationsModel *q);
    ~Private();

    void onNotificationAdded(const Notification &notification);
    void onNotificationReplaced(uint replacedId, const Notification &notification);
    void onNotificationRemoved(uint notificationId, Server::CloseReason reason);

    void setupNotificationTimeout(const Notification &notification);

    int rowOfNotification(uint id) const;

    NotificationsModel *q;

    QVector<Notification> notifications;
    // Fallback timeout to ensure all notifications expire eventually
    // otherwise when it isn't shown to the user and doesn't expire
    // an app might wait indefinitely for the notification to do so
    QHash<uint /*notificationId*/, QTimer*> notificationTimeouts;

    QDateTime lastRead;

};

NotificationsModel::Private::Private(NotificationsModel *q)
    : q(q)
    , lastRead(QDateTime::currentDateTimeUtc())
{

}

NotificationsModel::Private::~Private()
{
    qDeleteAll(notificationTimeouts);
    notificationTimeouts.clear();
}

void NotificationsModel::Private::onNotificationAdded(const Notification &notification)
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

    setupNotificationTimeout(notification);

    q->beginInsertRows(QModelIndex(), notifications.count(), notifications.count());
    notifications.append(notification);
    q->endInsertRows();
}

void NotificationsModel::Private::onNotificationReplaced(uint replacedId, const Notification &notification)
{
    const int row = rowOfNotification(replacedId);

    if (row == -1) {
        return;
    }

    Q_ASSERT(notifications[row].id() == notification.id());
    setupNotificationTimeout(notification);

    notifications[row] = notification;
    const QModelIndex idx = q->index(row, 0);
    emit q->dataChanged(idx, idx);
}

void NotificationsModel::Private::onNotificationRemoved(uint removedId, Server::CloseReason reason)
{
    const int row = rowOfNotification(removedId);
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

void NotificationsModel::Private::setupNotificationTimeout(const Notification &notification)
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

int NotificationsModel::Private::rowOfNotification(uint id) const
{
    auto it = std::find_if(notifications.constBegin(), notifications.constEnd(), [id](const Notification &item) {
        return item.id() == id;
    });

    if (it == notifications.constEnd()) {
        return -1;
    }

    return std::distance(notifications.constBegin(), it);
}

NotificationsModel::NotificationsModel()
    : QAbstractListModel(nullptr)
    , d(new Private(this))
{
    connect(&Server::self(), &Server::notificationAdded, this, [this](const Notification &notification) {
        d->onNotificationAdded(notification);
    });
    connect(&Server::self(), &Server::notificationReplaced, this, [this](uint replacedId, const Notification &notification) {
        d->onNotificationReplaced(replacedId, notification);
    });
    connect(&Server::self(), &Server::notificationRemoved, this, [this](uint removedId, Server::CloseReason reason) {
        d->onNotificationRemoved(removedId, reason);
    });

    Server::self().init();
}

NotificationsModel::~NotificationsModel() = default;

NotificationsModel::Ptr NotificationsModel::createNotificationsModel()
{
    static QWeakPointer<NotificationsModel> s_instance;
    if (!s_instance) {
        QSharedPointer<NotificationsModel> ptr(new NotificationsModel());
        s_instance = ptr.toWeakRef();
        return ptr;
    }
    return s_instance.toStrongRef();
}

QDateTime NotificationsModel::lastRead() const
{
    return d->lastRead;
}

void NotificationsModel::setLastRead(const QDateTime &lastRead)
{
    if (d->lastRead != lastRead) {
        d->lastRead = lastRead;
        emit lastReadChanged();
    }
}

QVariant NotificationsModel::data(const QModelIndex &index, int role) const
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

    case Notifications::ClosableRole: return true;
    case Notifications::ConfigurableRole: return notification.configurable();
    case Notifications::ConfigureActionLabelRole: return notification.configureActionLabel();

    case Notifications::ExpiredRole: return notification.expired();
    }

    return QVariant();
}

int NotificationsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return d->notifications.count();
}

void NotificationsModel::expire(uint notificationId)
{
    if (d->rowOfNotification(notificationId) > -1) {
        Server::self().closeNotification(notificationId, Server::CloseReason::Expired);
    }
}

void NotificationsModel::close(uint notificationId)
{
    if (d->rowOfNotification(notificationId) > -1) {
        Server::self().closeNotification(notificationId, Server::CloseReason::DismissedByUser);
    }
}

void NotificationsModel::configure(uint notificationId)
{
    const int row = d->rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(row);

    if (notification.d->hasConfigureAction) {
        Server::self().invokeAction(notificationId, QStringLiteral("settings")); // FIXME make a static Notification::configureActionName() or something
        return;
    }

    if (!notification.desktopEntry().isEmpty() || !notification.notifyRcName().isEmpty()) {
        configure(notification.desktopEntry(), notification.notifyRcName(), notification.eventId());
        return;
    }

    qCWarning(NOTIFICATIONMANAGER) << "Trying to configure notification" << notificationId << "which isn't configurable";
}

void NotificationsModel::configure(const QString &desktopEntry, const QString &notifyRcName, const QString &eventId)
{
    // TODO would be nice to just have a signal but since NotificationsModel is shared,
    // if we connect to this from Notifications you would get a signal in every instance
    // and potentialy open the config dialog multiple times.

    QStringList args;
    if (!desktopEntry.isEmpty()) {
        args.append(QStringLiteral("--desktop-entry"));
        args.append(desktopEntry);
    }
    if (!notifyRcName.isEmpty()) {
        args.append(QStringLiteral("--notifyrc"));
        args.append(notifyRcName);
    }
    if (!eventId.isEmpty()) {
        args.append(QStringLiteral("--event-id"));
        args.append(eventId);
    }

    QProcess::startDetached(QStringLiteral("kcmshell5"), {
        QStringLiteral("notifications"),
        QStringLiteral("--args"),
        KShell::joinArgs(args)
    });
}

void NotificationsModel::invokeDefaultAction(uint notificationId)
{
    const int row = d->rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(row);
    if (!notification.hasDefaultAction()) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to invoke default action on notification" << notificationId << "which doesn't have one";
        return;
    }

    Server::self().invokeAction(notificationId, QStringLiteral("default")); // FIXME make a static Notification::defaultActionName() or something
}

void NotificationsModel::invokeAction(uint notificationId, const QString &actionName)
{
    const int row = d->rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(row);
    if (!notification.actionNames().contains(actionName)) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to invoke action" << actionName << "on notification" << notificationId << "which it doesn't have";
        return;
    }

    Server::self().invokeAction(notificationId, actionName);
}

void NotificationsModel::startTimeout(uint notificationId)
{
    const int row = d->rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = d->notifications.at(row);

    if (!notification.timeout() || notification.expired()) {
        return;
    }

    d->setupNotificationTimeout(notification);
}

void NotificationsModel::stopTimeout(uint notificationId)
{
    delete d->notificationTimeouts.take(notificationId);
}

void NotificationsModel::clear(Notifications::ClearFlags flags)
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
