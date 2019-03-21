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

#include "notificationserver.h"

#include "notificationsadaptor.h"

#include "notification.h"

#include <QDBusConnection>

using namespace NotificationManager;

NotificationServer::NotificationServer(QObject *parent)
    : QObject(parent)
{
    new NotificationsAdaptor(this);

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/freedesktop/Notifications"), this);
    if (!QDBusConnection::sessionBus().registerService(QStringLiteral("org.freedesktop.Notifications"))) {
        qWarning() << "Failed to register Notifications service";
    }
}

NotificationServer::~NotificationServer() = default;

NotificationServer &NotificationServer::self()
{
    static NotificationServer s_self;
    return s_self;
}

void NotificationServer::closeNotification(uint notificationId, CloseReason reason)
{
    emit notificationRemoved(notificationId, reason);

    emit NotificationClosed(notificationId, static_cast<int>(reason)); // tell on DBus
}

void NotificationServer::invokeAction(uint notificationId, const QString &actionName)
{
    emit ActionInvoked(notificationId, actionName);
}

uint NotificationServer::Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                                const QString &summary, const QString &body, const QStringList &actions,
                                const QVariantMap &hints, int timeout)
{
    Q_ASSERT(calledFromDBus());

    const bool wasReplaced = replaces_id > 0;
    int notificationId = 0;
    if (wasReplaced) {
        notificationId = replaces_id;
    } else {
        ++m_highestNotificationId;
        notificationId = m_highestNotificationId;
    }

    Notification notification(notificationId);
    notification.setSummary(summary);
    notification.setBody(body);
    // we actually use that as notification icon (unless an image pixmap is provided in hints)
    notification.setIconName(app_icon);
    notification.setApplicationName(app_name);

    /*if (app_name.isEmpty()) {
        notification.setApplicationName(message().service());
    }*/

    notification.setActions(actions);

    notification.setTimeout(timeout);

    // might override some of the things we set above (like application name)
    notification.processHints(hints);

    if (wasReplaced) {
        notification.setUpdated();
        emit notificationReplaced(replaces_id, notification);
    } else {
        emit notificationAdded(notification);
    }

    return notificationId;
}

void NotificationServer::CloseNotification(uint id)
{
    // TODO raise dbus error if failed?
    closeNotification(id, CloseReason::Revoked);
}

QStringList NotificationServer::GetCapabilities() const
{
    // should this be configurable somehow so the UI can tell what it implements?
    return QStringList{
        QStringLiteral("body"),
        QStringLiteral("body-hyperlinks"),
        QStringLiteral("body-markup"),
        QStringLiteral("body-images"),
        QStringLiteral("icon-static"),
        QStringLiteral("actions"),
        // should we support "persistence" where notification stays present with "resident"
        // but that is basically an SNI isn't it?

        QStringLiteral("x-kde-urls")
    };
}

QString NotificationServer::GetServerInformation(QString &vendor, QString &version, QString &specVersion) const
{
    vendor = QStringLiteral("KDE");
    version = QLatin1String(PROJECT_VERSION);
    specVersion = QStringLiteral("1.2");
    return QStringLiteral("Plasma");
}
