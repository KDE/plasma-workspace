/*
 * Copyright 2020 Shah Bhushan <bshah@kde.org>
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

#include "watchednotificationsmodel.h"

#include <QDBusMetaType>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>

#include <QDebug>

#include "fdonotifications_interface.h"

using namespace NotificationManager;

class WatchedNotificationsModel::Private : public QObject
{
    Q_OBJECT
public:
    explicit Private(WatchedNotificationsModel* q, QObject* parent = nullptr);
    ~Private();
    bool valid = false;
    void registerWatcher(const QString &service);

public Q_SLOTS:
    Q_SCRIPTABLE void Notify(uint id, const QString &app_name, uint replaces_id, const QString &app_icon,
                        const QString &summary, const QString &body, const QStringList &actions,
                        const QVariantMap &hints, int timeout);
    Q_SCRIPTABLE void CloseNotification(uint id);
    void NotificationClosed(uint id, uint reason);

private:
    WatchedNotificationsModel* q;
    OrgFreedesktopNotificationsInterface *fdoNotificationsInterface;
};

WatchedNotificationsModel::Private::Private(WatchedNotificationsModel* q, QObject *parent)
    : QObject(parent)
    , q(q)
{
}

void WatchedNotificationsModel::Private::registerWatcher(const QString& service)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    fdoNotificationsInterface = new OrgFreedesktopNotificationsInterface(QStringLiteral("org.freedesktop.Notifications"),
                                                                         QStringLiteral("/org/freedesktop/Notifications"),
                                                                         dbus,
                                                                         this);
    connect(fdoNotificationsInterface, &OrgFreedesktopNotificationsInterface::NotificationClosed,
            this, &WatchedNotificationsModel::Private::NotificationClosed);

    dbus.registerObject("/NotificationWatcher", QStringLiteral("org.kde.NotificationWatcher"), this, QDBusConnection::ExportScriptableSlots);
    valid = dbus.registerService(service);
    Q_EMIT q->validChanged(valid);
}

WatchedNotificationsModel::Private::~Private()
{
}

void WatchedNotificationsModel::Private::Notify(uint id, const QString &app_name, uint replaces_id, const QString &app_icon,
                                                const QString &summary, const QString &body, const QStringList &actions,
                                                const QVariantMap &hints, int timeout)
{
    const bool wasReplaced = replaces_id > 0;

    qDebug() << summary;
    qDebug() << body;
    Notification notification(id);
    notification.setSummary(summary);
    notification.setBody(body);
    notification.setApplicationName(app_name);

    notification.setActions(actions);
    notification.setTimeout(timeout);
    notification.setHints(hints);
    notification.setIcon(app_icon);
    if(wasReplaced) {
        q->onNotificationReplaced(replaces_id, notification);
    } else {
        q->onNotificationAdded(notification);
    }
}

void WatchedNotificationsModel::Private::CloseNotification(uint id)
{
    q->onNotificationRemoved(id, Server::CloseReason::Expired);
}

void WatchedNotificationsModel::Private::NotificationClosed(uint id, uint reason)
{
    q->onNotificationRemoved(id, static_cast<Server::CloseReason>(reason));
}

WatchedNotificationsModel::WatchedNotificationsModel()
    : AbstractNotificationsModel(),
    d(new Private(this, nullptr))
{
}

void WatchedNotificationsModel::registerWatcher(const QString& service)
{
    d->registerWatcher(service);
}

WatchedNotificationsModel::~WatchedNotificationsModel()
{
}

void WatchedNotificationsModel::close(uint notificationId)
{
    onNotificationRemoved(notificationId, Server::CloseReason::DismissedByUser);
}

void WatchedNotificationsModel::expire(uint notificationId)
{
    onNotificationRemoved(notificationId, Server::CloseReason::Expired);
}

void WatchedNotificationsModel::invokeDefaultAction(uint notificationId)
{
    this->invokeAction(notificationId, QStringLiteral("default"));
}

void WatchedNotificationsModel::invokeAction(uint notificationId, const QString &actionName)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/NotificationWatcher", this, QDBusConnection::ExportScriptableSlots);
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.kde.NotificationManager"),
        QStringLiteral("InvokeAction")
    );
    msg.setArguments({
        notificationId,
        actionName
    });
    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);  
}

void WatchedNotificationsModel::reply(uint notificationId, const QString &text)
{
    // todo
    Q_UNUSED(notificationId)
    Q_UNUSED(text)
}

bool WatchedNotificationsModel::valid()
{
    return d->valid;
}

#include "watchednotificationsmodel.moc"
