/*
 * SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "portalnotificationsmodel.h"
#include "notification_p.h"
#include "portal_p.h"
#include "server.h"

#include "debug.h"

#include <KIO/CommandLauncherJob>
#include <KShell>

using namespace Qt::StringLiterals;

namespace NotificationManager
{

PortalNotificationsModel::Ptr PortalNotificationsModel::create()
{
    static std::weak_ptr<PortalNotificationsModel> s_instance;
    if (s_instance.expired()) {
        std::shared_ptr<PortalNotificationsModel> ptr(new PortalNotificationsModel());
        s_instance = ptr;
        return ptr;
    }
    return s_instance.lock();
}

PortalNotificationsModel::PortalNotificationsModel()
{
    connect(&PortalPrivate::self(), &PortalPrivate::notificationAdded, this, [this](const Notification &notification) {
        onNotificationAdded(notification);
    });
    connect(&PortalPrivate::self(), &PortalPrivate::notificationReplaced, this, [this](uint replacedId, const Notification &notification) {
        onNotificationReplaced(replacedId, notification);
    });
    connect(&PortalPrivate::self(), &PortalPrivate::notificationRemoved, this, [this](uint removedId) {
        onNotificationRemoved(removedId, Server::CloseReason::Revoked);
    });
    PortalPrivate::self().init();
}

void PortalNotificationsModel::expire(uint notificationId)
{
    if (rowOfNotification(notificationId) > -1) {
        onNotificationRemoved(notificationId, Server::CloseReason::Expired);
    }
}

void PortalNotificationsModel::close(uint notificationId)
{
    if (rowOfNotification(notificationId) > -1) {
        PortalPrivate::self().removeNotification(notificationId);
        onNotificationRemoved(notificationId, Server::CloseReason::DismissedByUser);
    }
}

void PortalNotificationsModel::invokeDefaultAction(uint notificationId, Notifications::InvokeBehavior behavior)
{
    Q_UNUSED(behavior);

    const int row = rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = notifications().at(row);
    if (!notification.hasDefaultAction()) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to invoke default action on notification" << notificationId << "which doesn't have one";
        return;
    }

    // TODO maybe make defaultactionid public?
    PortalPrivate::self().invokeAction(notificationId, notification.d->defaultActionId, notification.d->defaultActionTarget, {}, window());
}

void PortalNotificationsModel::invokeAction(uint notificationId, const QString &actionName, Notifications::InvokeBehavior behavior)
{
    Q_UNUSED(behavior);

    const int row = rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = notifications().at(row);
    const int actionIndex = notification.actionNames().indexOf(actionName);
    if (actionIndex == -1) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to invoke action" << actionName << "on notification" << notificationId << "which it doesn't have";
        return;
    }

    const QVariant actionTarget = notification.d->actionTargets.at(actionIndex);
    PortalPrivate::self().invokeAction(notificationId, actionName, actionTarget, {}, window());
}

void PortalNotificationsModel::reply(uint notificationId, const QString &text, Notifications::InvokeBehavior behavior)
{
    Q_UNUSED(behavior);

    const int row = rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = notifications().at(row);
    if (!notification.hasReplyAction()) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to reply to a notification which doesn't have a reply action";
        return;
    }

    PortalPrivate::self().invokeAction(notificationId, notification.d->replyActionId, {}, text, window());
}

void PortalNotificationsModel::configure(uint notificationId)
{
    const int row = rowOfNotification(notificationId);
    if (row == -1) {
        return;
    }

    const Notification &notification = notifications().at(row);

    /*if (notification.d->hasConfigureAction) {
        Server::self().invokeAction(notificationId,
                                    QStringLiteral("settings"),
                                    notification.d->xdgTokenAppId,
                                    Notifications::None, // FIXME make a static Notification::configureActionName() or something
                                    window());
        return;
    }*/

    if (!notification.desktopEntry().isEmpty() || !notification.notifyRcName().isEmpty()) {
        configure(notification.desktopEntry(), notification.notifyRcName(), notification.eventId());
        return;
    }

    qCWarning(NOTIFICATIONMANAGER) << "Trying to configure notification" << notificationId << "which isn't configurable";
}

void PortalNotificationsModel::configure(const QString &desktopEntry, const QString &notifyRcName, const QString &eventId)
{
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

    const QString systemSettings = QStringLiteral("systemsettings");
    auto job = new KIO::CommandLauncherJob(systemSettings, {QStringLiteral("kcm_notifications"), QStringLiteral("--args"), KShell::joinArgs(args)});
    job->setDesktopName(systemSettings);
    job->start();
}

} // namespace NotificationManager
