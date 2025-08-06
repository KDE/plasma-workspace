/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "abstractnotificationsmodel.h"
#include "abstractnotificationsmodel_p.h"
#include "debug.h"

#include "utils_p.h"

#include "notification_p.h"

#include <QDBusConnection>
#include <QDebug>
#include <QProcess>
#include <QTextDocumentFragment>

#include <KLocalizedString>
#include <KShell>

#include <algorithm>
#include <chrono>
#include <functional>

using namespace std::chrono_literals;

static constexpr int s_notificationsLimit = 1000;

using namespace NotificationManager;

AbstractNotificationsModel::Private::Private(AbstractNotificationsModel *q)
    : q(q)
    , lastRead(QDateTime::currentDateTimeUtc())
{
    pendingRemovalTimer.setSingleShot(true);
    pendingRemovalTimer.setInterval(50ms);
    connect(&pendingRemovalTimer, &QTimer::timeout, q, [this, q] {
        QList<int> rowsToBeRemoved;
        rowsToBeRemoved.reserve(pendingRemovals.count());
        for (uint id : std::as_const(pendingRemovals)) {
            Notification::Private::s_imageCache.remove(id);
            int row = q->rowOfNotification(id); // oh the complexity...
            if (row == -1) {
                continue;
            }
            rowsToBeRemoved.append(row);
        }

        removeRows(rowsToBeRemoved);
    });

    notificationWatcher.setConnection(QDBusConnection::sessionBus());
    notificationWatcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    // Forcibly expire the notification once the owning application exits, in order to
    // remove the interactive buttons from the notification in the history and/or make the
    // popup disappear.
    connect(&notificationWatcher, &QDBusServiceWatcher::serviceUnregistered, q, [this, q](const QString &serviceName) {
        for (const Notification &notification : std::as_const(notifications)) {
            if (notification.dBusService() == serviceName) {
                q->expire(notification.id());
            }
        }

        notificationWatcher.removeWatchedService(serviceName);
    });
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
        qCDebug(NOTIFICATIONMANAGER) << "Reached the notification limit of" << s_notificationsLimit << ", discarding the oldest" << cleanupCount
                                     << "notifications";
        q->beginRemoveRows(QModelIndex(), 0, cleanupCount - 1);
        for (int i = 0; i < cleanupCount; ++i) {
            Notification::Private::s_imageCache.remove(notifications.at(0).id());
            q->stopTimeout(notifications.first().id());
            notifications.removeAt(0);
            // TODO close gracefully?
        }
        q->endRemoveRows();
    }

    setupNotificationTimeout(notification);
    // Only set up watchers for notifications with actions, since some apps (e.g. `notify-send`) may just
    // dispatch a notification and then immediately exit
    if (notification.hasDefaultAction() || notification.hasReplyAction() || !notification.actionNames().empty()) {
        notificationWatcher.addWatchedService(notification.dBusService());
    }

    q->beginInsertRows(QModelIndex(), notifications.count(), notifications.count());
    notifications.append(std::move(notification));
    q->endInsertRows();
}

void AbstractNotificationsModel::Private::onNotificationReplaced(uint replacedId, const Notification &notification)
{
    const int row = q->rowOfNotification(replacedId);

    if (row == -1) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to replace notification with id" << replacedId
                                       << "which doesn't exist, creating a new one. This is a bug in the application" << notification.applicationName();
        onNotificationAdded(notification);
        return;
    }

    setupNotificationTimeout(notification);

    Notification newNotification(notification);

    const Notification &oldNotification = notifications.at(row);
    // As per spec a notification must be replaced atomically with no visual cues.
    // Transfer over properties that might cause this, such as unread showing the bell again,
    // or created() which should indicate the original date, whereas updated() is when it was last updated
    newNotification.setCreated(oldNotification.created());
    newNotification.setExpired(oldNotification.expired());
    newNotification.setDismissed(oldNotification.dismissed());
    newNotification.setRead(oldNotification.read());
    newNotification.setWasAddedDuringInhibition(Server::self().inhibited());

    notifications[row] = newNotification;
    const QModelIndex idx = q->index(row, 0);
    Q_EMIT q->dataChanged(idx, idx);
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

        // clang-format off
        Q_EMIT q->dataChanged(idx, idx, {
            Notifications::ExpiredRole,
            // TODO only Q_EMIT those if actually changed?
            Notifications::ActionNamesRole,
            Notifications::ActionLabelsRole,
            Notifications::HasDefaultActionRole,
            Notifications::DefaultActionLabelRole,
            Notifications::ConfigurableRole
        });
        // clang-format on

        return;
    }

    // Otherwise if explicitly closed by either user or app, mark it for removal
    // some apps are notorious for closing a bunch of notifications at once
    // causing newer notifications to move up and have a dialogs created for them
    // just to then be discarded causing excess CPU usage
    if (!pendingRemovals.contains(removedId)) {
        pendingRemovals.append(removedId);
    }

    if (!pendingRemovalTimer.isActive()) {
        pendingRemovalTimer.start();
    }
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

void AbstractNotificationsModel::Private::removeRows(const QList<int> &rows)
{
    if (rows.isEmpty()) {
        return;
    }

    QList<int> rowsToBeRemoved(rows);
    std::sort(rowsToBeRemoved.begin(), rowsToBeRemoved.end());

    QList<QPair<int, int>> clearQueue;

    QPair<int, int> clearRange{rowsToBeRemoved.first(), rowsToBeRemoved.first()};

    for (int row : rowsToBeRemoved) {
        if (row > clearRange.second + 1) {
            clearQueue.append(clearRange);
            clearRange.first = row;
        }

        clearRange.second = row;
    }

    if (clearQueue.isEmpty() || clearQueue.last() != clearRange) {
        clearQueue.append(clearRange);
    }

    int rowsRemoved = 0;

    for (int i = clearQueue.count() - 1; i >= 0; --i) {
        const auto &range = clearQueue.at(i);

        q->beginRemoveRows(QModelIndex(), range.first, range.second);
        for (int j = range.second; j >= range.first; --j) {
            notifications.removeAt(j);
            ++rowsRemoved;
        }
        q->endRemoveRows();
    }

    Q_ASSERT(rowsRemoved == rowsToBeRemoved.count());

    pendingRemovals.clear();
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
        Q_EMIT lastReadChanged();
    }
}

QWindow *AbstractNotificationsModel::window() const
{
    return d->window;
}

void AbstractNotificationsModel::setWindow(QWindow *window)
{
    if (d->window == window) {
        return;
    }
    if (d->window) {
        disconnect(d->window, &QObject::destroyed, this, nullptr);
    }
    d->window = window;
    if (d->window) {
        connect(d->window, &QObject::destroyed, this, [this] {
            setWindow(nullptr);
        });
    }
    Q_EMIT windowChanged(window);
}

QVariant AbstractNotificationsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    const Notification &notification = d->notifications.at(index.row());

    switch (role) {
    case Notifications::IdRole:
        return notification.id();
    case Notifications::TypeRole:
        return Notifications::NotificationType;

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
    case Notifications::SummaryRole:
        return notification.summary();
    case Notifications::BodyRole:
        return notification.body();
    case Qt::AccessibleDescriptionRole:
        return i18nc("@info %1 notification body %2 application name",
                     "%1 from %2",
                     QTextDocumentFragment::fromHtml(notification.body()).toPlainText(),
                     notification.applicationName());
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
    case Notifications::DesktopEntryRole:
        return notification.desktopEntry();
    case Notifications::NotifyRcNameRole:
        return notification.notifyRcName();

    case Notifications::ApplicationNameRole:
        return notification.applicationName();
    case Notifications::ApplicationIconNameRole:
        return notification.applicationIconName();
    case Notifications::OriginNameRole:
        return notification.originName();

    case Notifications::ActionNamesRole:
        return notification.actionNames();
    case Notifications::ActionLabelsRole:
        return notification.actionLabels();
    case Notifications::HasDefaultActionRole:
        return notification.hasDefaultAction();
    case Notifications::DefaultActionLabelRole:
        return notification.defaultActionLabel();

    case Notifications::UrlsRole:
        return QVariant::fromValue(notification.urls());

    case Notifications::UrgencyRole:
        return static_cast<int>(notification.urgency());
    case Notifications::UserActionFeedbackRole:
        return notification.userActionFeedback();

    case Notifications::TimeoutRole:
        return notification.timeout();

    case Notifications::ClosableRole:
        return true;
    case Notifications::ConfigurableRole:
        return notification.configurable();
    case Notifications::ConfigureActionLabelRole:
        return notification.configureActionLabel();

    case Notifications::CategoryRole:
        return notification.category();

    case Notifications::ExpiredRole:
        return notification.expired();
    case Notifications::ReadRole:
        return notification.read();
    case Notifications::ResidentRole:
        return notification.resident();
    case Notifications::TransientRole:
        return notification.transient();

    case Notifications::WasAddedDuringInhibitionRole:
        return notification.wasAddedDuringInhibition();

    case Notifications::HasReplyActionRole:
        return notification.hasReplyAction();
    case Notifications::ReplyActionLabelRole:
        return notification.replyActionLabel();
    case Notifications::ReplyPlaceholderTextRole:
        return notification.replyPlaceholderText();
    case Notifications::ReplySubmitButtonTextRole:
        return notification.replySubmitButtonText();
    case Notifications::ReplySubmitButtonIconNameRole:
        return notification.replySubmitButtonIconName();
    case Notifications::HintsRole:
        return notification.hints();

    case Notifications::DismissedRole:
        return notification.dismissed();
    case Notifications::DismissableRole:
        return notification.resident() && notification.timeout() == 0;
    }

    return QVariant();
}

bool AbstractNotificationsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        return false;
    }

    Notification &notification = d->notifications[index.row()];
    bool dirty = false;

    switch (role) {
    case Notifications::ReadRole:
        if (value.toBool() != notification.read()) {
            notification.setRead(value.toBool());
            dirty = true;
        }
        break;
    // Allows to mark a notification as expired without actually sending that out through expire() for persistency
    case Notifications::ExpiredRole:
        if (value.toBool() != notification.expired()) {
            notification.setExpired(value.toBool());
            dirty = true;
        }
        break;
    case Notifications::WasAddedDuringInhibitionRole:
        if (bool v = value.toBool(); v != notification.wasAddedDuringInhibition()) {
            notification.setWasAddedDuringInhibition(v);
            dirty = true;
        }
        break;
    case Notifications::DismissedRole:
        if (const bool v = value.toBool(); v != notification.dismissed()) {
            notification.setDismissed(v);
            dirty = true;
        }
    }

    if (dirty) {
        Q_EMIT dataChanged(index, index, {role});
    }

    return dirty;
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

    QList<int> rowsToRemove;

    for (int i = 0; i < d->notifications.count(); ++i) {
        const Notification &notification = d->notifications.at(i);

        if (flags.testFlag(Notifications::ClearExpired) && (notification.expired() || notification.wasAddedDuringInhibition())) {
            close(notification.id());
        }
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

const QList<Notification> &AbstractNotificationsModel::notifications()
{
    return d->notifications;
}

#include "moc_abstractnotificationsmodel.cpp"
