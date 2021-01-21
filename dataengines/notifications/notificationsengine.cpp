/*
 *   Copyright (C) 2008 Dmitry Suzdalev <dimsuz@gmail.com>
 *
 * This program is free software you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "notificationsengine.h"
#include "notificationservice.h"

#include "notification.h"
#include "server.h"

#include <KConfig>
#include <KConfigGroup>
#include <KNotifyConfigWidget>
#include <KSharedConfig>
#include <QGuiApplication>
#include <klocalizedstring.h>

#include <Plasma/DataContainer>
#include <Plasma/Service>

#include <QImage>

#include "debug.h"

using namespace NotificationManager;

NotificationsEngine::NotificationsEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
    init();
}

NotificationsEngine::~NotificationsEngine()
{
}

void NotificationsEngine::init()
{
    connect(&Server::self(), &Server::notificationAdded, this, [this](const Notification &notification) {
        notificationAdded(notification);
    });

    connect(&Server::self(), &Server::notificationReplaced, this, [this](uint replacedId, const Notification &notification) {
        // Notification will already have the correct identical ID
        Q_UNUSED(replacedId);
        notificationAdded(notification);
    });

    connect(&Server::self(), &Server::notificationRemoved, this, [this](uint id, Server::CloseReason reason) {
        Q_UNUSED(reason);
        const QString source = QStringLiteral("notification %1").arg(id);
        // if we don't have that notification in our local list,
        // it has already been closed so don't notify a second time
        if (m_activeNotifications.remove(source) > 0) {
            removeSource(source);
        }
    });

    Server::self().init();
}

void NotificationsEngine::notificationAdded(const Notification &notification)
{
    const QString app_name = notification.applicationName();
    const QString appRealName = notification.notifyRcName();
    const QString eventId = notification.eventId(); // FIXME = hints[QStringLiteral("x-kde-eventId")].toString();
    const QStringList urls = QUrl::toStringList(notification.urls());
    const QString desktopEntry = notification.desktopEntry();
    const QString summary = notification.summary();

    QString bodyFinal = notification.body(); // is already sanitized by NotificationManager
    QString summaryFinal = notification.summary();
    int timeout = notification.timeout();

    if (bodyFinal.isEmpty()) {
        // some ridiculous apps will send just a title (#372112), in that case, treat it as though there's only a body
        bodyFinal = summary;
        summaryFinal = app_name;
    }

    uint id = notification.id(); // replaces_id ? replaces_id : m_nextId++;

    QString appname_str = app_name;
    if (appname_str.isEmpty()) {
        appname_str = i18n("Unknown Application");
    }

    bool isPersistent = (timeout == 0);

    const int AVERAGE_WORD_LENGTH = 6;
    const int WORD_PER_MINUTE = 250;
    int count = notification.summary().length() + notification.body().length() - strlen("<?xml version=\"1.0\"><html></html>");

    // -1 is "server default", 0 is persistent with "server default" display time,
    // anything more should honor the setting
    if (timeout <= 0) {
        timeout = 60000 * count / AVERAGE_WORD_LENGTH / WORD_PER_MINUTE;

        // Add two seconds for the user to notice the notification, and ensure
        // it last at least five seconds, otherwise all the user see is a
        // flash
        timeout = 2000 + qMax(timeout, 3000);
    }

    const QString source = QStringLiteral("notification %1").arg(id);

    Plasma::DataEngine::Data notificationData;
    notificationData.insert(QStringLiteral("id"), QString::number(id));
    notificationData.insert(QStringLiteral("eventId"), eventId);
    notificationData.insert(QStringLiteral("appName"), notification.applicationName());
    // TODO should be proper passed in icon?
    notificationData.insert(QStringLiteral("appIcon"), notification.applicationIconName());
    notificationData.insert(QStringLiteral("summary"), summaryFinal);
    notificationData.insert(QStringLiteral("body"), bodyFinal);

    QStringList actions;
    for (int i = 0; i < notification.actionNames().count(); ++i) {
        actions << notification.actionNames().at(i) << notification.actionLabels().at(i);
    }
    // NotificationManager hides the configure and default stuff from us but we need to re-add them
    // to the actions list for compatibility
    if (!notification.configureActionLabel().isEmpty()) {
        actions << QStringLiteral("settings") << notification.configureActionLabel();
    }
    if (notification.hasDefaultAction()) {
        actions << QStringLiteral("default") << QString();
    }

    notificationData.insert(QStringLiteral("actions"), actions);
    notificationData.insert(QStringLiteral("isPersistent"), isPersistent);
    notificationData.insert(QStringLiteral("expireTimeout"), timeout);

    notificationData.insert(QStringLiteral("desktopEntry"), desktopEntry);

    KService::Ptr service = KService::serviceByStorageId(desktopEntry);
    if (service) {
        notificationData.insert(QStringLiteral("appServiceName"), service->name());
        notificationData.insert(QStringLiteral("appServiceIcon"), service->icon());
    }

    notificationData.insert(QStringLiteral("appRealName"), appRealName);
    // NotificationManager configurable is anything that has a notifyrc or desktop entry
    // but the old stuff assumes only stuff with notifyrc to be configurable
    notificationData.insert(QStringLiteral("configurable"), !notification.notifyRcName().isEmpty());

    QImage image = notification.image();
    notificationData.insert(QStringLiteral("image"), image.isNull() ? QVariant() : image);

    int urgency = -1;
    switch (notification.urgency()) {
    case Notifications::LowUrgency:
        urgency = 0;
        break;
    case Notifications::NormalUrgency:
        urgency = 1;
        break;
    case Notifications::CriticalUrgency:
        urgency = 2;
        break;
    }

    if (urgency > -1) {
        notificationData.insert(QStringLiteral("urgency"), urgency);
    }

    notificationData.insert(QStringLiteral("urls"), urls);

    setData(source, notificationData);

    m_activeNotifications.insert(source, notification.applicationName() + notification.summary());
}

void NotificationsEngine::removeNotification(uint id, uint closeReason)
{
    const QString source = QStringLiteral("notification %1").arg(id);
    // if we don't have that notification in our local list,
    // it has already been closed so don't notify a second time
    if (m_activeNotifications.remove(source) > 0) {
        removeSource(source);
        Server::self().closeNotification(id, static_cast<Server::CloseReason>(closeReason));
    }
}

Plasma::Service *NotificationsEngine::serviceForSource(const QString &source)
{
    return new NotificationService(this, source);
}

int NotificationsEngine::createNotification(const QString &appName,
                                            const QString &appIcon,
                                            const QString &summary,
                                            const QString &body,
                                            int timeout,
                                            const QStringList &actions,
                                            const QVariantMap &hints)
{
    Notification notification;
    notification.setApplicationName(appName);
    notification.setApplicationIconName(appIcon);
    notification.setSummary(summary);
    notification.setBody(body); // sanitizes
    notification.setActions(actions);
    notification.setTimeout(timeout);
    notification.processHints(hints);
    Server::self().add(notification);
    return 0;
}

void NotificationsEngine::configureNotification(const QString &appName, const QString &eventId)
{
    KNotifyConfigWidget *widget = KNotifyConfigWidget::configure(nullptr, appName);
    if (!eventId.isEmpty()) {
        widget->selectEvent(eventId);
    }
}

QSharedPointer<NotificationInhibiton> NotificationsEngine::createInhibition(const QString &hint, const QString &value)
{
    auto ni = new NotificationInhibiton;
    ni->hint = hint;
    ni->value = value;

    QPointer<NotificationsEngine> guard(this);
    QSharedPointer<NotificationInhibiton> rc(ni, [this, guard](NotificationInhibiton *ni) {
        if (guard) {
            m_inhibitions.removeOne(ni);
        }
        delete ni;
    });
    m_inhibitions.append(ni);
    return rc;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(notifications, NotificationsEngine, "plasma-dataengine-notifications.json")

#include "notificationsengine.moc"
