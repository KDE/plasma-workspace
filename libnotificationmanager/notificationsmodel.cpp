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

#include "notificationsmodel.h"
#include "server.h"
#include "abstractnotificationsmodel_p.h"
#include "notification_p.h"

#include "debug.h"

#include <QProcess>

#include <KShell>

using namespace NotificationManager;

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

NotificationsModel::NotificationsModel()
{
    connect(&Server::self(), &Server::notificationAdded, this, [this](const Notification &notification) {
        onNotificationAdded(notification);
    });
    connect(&Server::self(), &Server::notificationReplaced, this, [this](uint replacedId, const Notification &notification) {
        onNotificationReplaced(replacedId, notification);
    });
    connect(&Server::self(), &Server::notificationRemoved, this, [this](uint removedId, Server::CloseReason reason) {
        onNotificationRemoved(removedId, reason);
    });
    connect(&Server::self(), &Server::serviceOwnershipLost, this, [this] {
        // Expire all notifications as we're defunct now
        const auto notificationList = notifications();
        for (const Notification &notification : notificationList) {
            if (!notification.expired()) {
                onNotificationRemoved(notification.id(), Server::CloseReason::Expired);
            }
        }
    });
    Server::self().init();
}

void NotificationsModel::expire(uint notificationId)
{
    if (rowOfNotification(notificationId) > -1) {
        Server::self().closeNotification(notificationId, Server::CloseReason::Expired);
    }
}

void NotificationsModel::close(uint notificationId)
{
    if (rowOfNotification(notificationId) > -1) {
        Server::self().closeNotification(notificationId, Server::CloseReason::DismissedByUser);
    }
}


void NotificationsModel::invokeDefaultAction(uint notificationId)
{
    const int row = rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = notifications().at(row);
    if (!notification.hasDefaultAction()) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to invoke default action on notification" << notificationId << "which doesn't have one";
        return;
    }

    Server::self().invokeAction(notificationId, QStringLiteral("default")); // FIXME make a static Notification::defaultActionName() or something
}

void NotificationsModel::invokeAction(uint notificationId, const QString &actionName)
{
    const int row = rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = notifications().at(row);
    if (!notification.actionNames().contains(actionName)) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to invoke action" << actionName << "on notification" << notificationId << "which it doesn't have";
        return;
    }

    Server::self().invokeAction(notificationId, actionName);
}

void NotificationsModel::reply(uint notificationId, const QString &text)
{
    const int row = rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = notifications().at(row);
    if (!notification.hasReplyAction()) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to reply to a notification which doesn't have a reply action";
        return;
    }

    Server::self().reply(notification.dBusService(), notificationId, text);
}


void NotificationsModel::configure(uint notificationId)
{
    const int row = rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = notifications().at(row);

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
    // and potentially open the config dialog multiple times.

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
