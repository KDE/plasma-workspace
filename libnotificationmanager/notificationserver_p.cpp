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

#include "notificationserver_p.h"

#include "debug.h"

#include "notificationsadaptor.h"

#include "notification.h"
#include "notification_p.h"

#include "notificationserver.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>

#include <KConfigGroup>
#include <KService>
#include <KSharedConfig>
#include <KUser>

#include <processcore/processes.h>
#include <processcore/process.h>

using namespace NotificationManager;

NotificationServerPrivate::NotificationServerPrivate(QObject *parent)
    : QObject(parent)
{

}

NotificationServerPrivate::~NotificationServerPrivate() = default;

bool NotificationServerPrivate::init()
{
    if (m_valid) {
        return true;
    }

    new NotificationsAdaptor(this);

    if (!QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/freedesktop/Notifications"), this)) {
        qCWarning(NOTIFICATIONMANAGER) << "Failed to register Notification DBus object";
        return false;
    }

    if (!QDBusConnection::sessionBus().registerService(QStringLiteral("org.freedesktop.Notifications"))) {
        qCWarning(NOTIFICATIONMANAGER) << "Failed to register Notification service on DBus";
        return false;
    }

    m_inhibitionWatcher = new QDBusServiceWatcher(this);
    m_inhibitionWatcher->setConnection(QDBusConnection::sessionBus());
    m_inhibitionWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_inhibitionWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &NotificationServerPrivate::onServiceUnregistered);

    connect(this, &NotificationServerPrivate::inhibitedChanged, this, [this] {
        // emit DBus change signal...
        QDBusMessage signal = QDBusMessage::createSignal(
            QStringLiteral("/org/freedesktop/Notifications"),
            QStringLiteral("org.freedesktop.DBus.Properties"),
            QStringLiteral("PropertiesChanged")
        );

        signal.setArguments({
            QStringLiteral("org.freedesktop.Notifications"),
            QVariantMap{ // updated
                {QStringLiteral("Inhibited"), inhibited()},
            },
            QStringList() // invalidated
        });

        QDBusConnection::sessionBus().send(signal);
    });

    qCDebug(NOTIFICATIONMANAGER) << "Registered Notification service on DBus";

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Notifications"));
    const bool broadcastsEnabled = config.readEntry("ListenForBroadcasts", false);

    if (broadcastsEnabled) {
        qCDebug(NOTIFICATIONMANAGER) << "Notification server is configured to listen for broadcasts";
        QDBusConnection::systemBus().connect({}, {}, QStringLiteral("org.kde.BroadcastNotifications"),
                                             QStringLiteral("Notify"), this, SLOT(onBroadcastNotification(QMap<QString,QVariant>)));
    }

    m_valid = true;
    return true;
}

uint NotificationServerPrivate::Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                                       const QString &summary, const QString &body, const QStringList &actions,
                                       const QVariantMap &hints, int timeout)
{
    const bool wasReplaced = replaces_id > 0;
    int notificationId = 0;
    if (wasReplaced) {
        notificationId = replaces_id;
    } else {
        // TODO according to spec should wrap around once INT_MAX is exceeded
        ++m_highestNotificationId;
        notificationId = m_highestNotificationId;
    }

    Notification notification(notificationId);
    notification.setSummary(summary);
    notification.setBody(body);
    // we actually use that as notification icon (unless an image pixmap is provided in hints)
    notification.setIconName(app_icon);
    notification.setApplicationName(app_name);

    notification.setActions(actions);

    notification.setTimeout(timeout);

    // might override some of the things we set above (like application name)
    notification.d->processHints(hints);

    // No application name? Try to figure out the process name using the sender's PID
    if (notification.applicationName().isEmpty()) {
        qCInfo(NOTIFICATIONMANAGER) << "Notification from service" << message().service() << "didn't contain any identification information, this is an application bug!";

        QDBusReply<uint> pidReply = connection().interface()->servicePid(message().service());
        if (pidReply.isValid()) {
            const auto pid = pidReply.value();

            KSysGuard::Processes procs;
            procs.updateOrAddProcess(pid);

            if (KSysGuard::Process *proc = procs.getProcess(pid)) {
                qCDebug(NOTIFICATIONMANAGER) << "Resolved notification to be from PID" << pid << "which is" << proc->name();
                notification.setApplicationName(proc->name());
            }
        }
    }

    if (wasReplaced) {
        notification.resetUpdated();
        emit static_cast<NotificationServer*>(parent())->notificationReplaced(replaces_id, notification);
    } else {
        emit static_cast<NotificationServer*>(parent())->notificationAdded(notification);
    }

    return notificationId;
}

void NotificationServerPrivate::CloseNotification(uint id)
{
    // spec says "If the notification no longer exists, an empty D-BUS Error message is sent back."
    static_cast<NotificationServer*>(parent())->closeNotification(id, NotificationServer::CloseReason::Revoked);
}

QStringList NotificationServerPrivate::GetCapabilities() const
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

        QStringLiteral("x-kde-urls"),

        QStringLiteral("inhibitions")
    };
}

QString NotificationServerPrivate::GetServerInformation(QString &vendor, QString &version, QString &specVersion) const
{
    vendor = QStringLiteral("KDE");
    version = QLatin1String(PROJECT_VERSION);
    specVersion = QStringLiteral("1.2");
    return QStringLiteral("Plasma");
}

void NotificationServerPrivate::onBroadcastNotification(const QMap<QString, QVariant> &properties)
{
    qCDebug(NOTIFICATIONMANAGER) << "Received broadcast notification";

    const auto currentUserId = KUserId::currentEffectiveUserId().nativeId();

    // a QVariantList with ints arrives as QDBusArgument here, using a QStringList for simplicity
    const QStringList &userIds = properties.value(QStringLiteral("uids")).toStringList();
    if (!userIds.isEmpty()) {
        auto it = std::find_if(userIds.constBegin(), userIds.constEnd(), [currentUserId](const QVariant &id) {
            bool ok;
            auto uid = id.toString().toLongLong(&ok);
            return ok && uid == currentUserId;
        });

        if (it == userIds.constEnd()) {
            qCDebug(NOTIFICATIONMANAGER) << "It is not meant for us, ignoring";
            return;
        }
    }

    bool ok;
    int timeout = properties.value(QStringLiteral("timeout")).toInt(&ok);
    if (!ok) {
        timeout = -1; // -1 = server default, 0 would be "persistent"
    }

    Notify(
        properties.value(QStringLiteral("appName")).toString(),
        0, // replaces_id
        properties.value(QStringLiteral("appIcon")).toString(),
        properties.value(QStringLiteral("summary")).toString(),
        properties.value(QStringLiteral("body")).toString(),
        {}, // no actions
        properties.value(QStringLiteral("hints")).toMap(),
        timeout
    );
}

uint NotificationServerPrivate::add(const Notification &notification)
{
    // TODO check if notification with ID already exists and signal update instead
    if (notification.id() == 0) {
        ++m_highestNotificationId;
        notification.d->id = m_highestNotificationId;

        emit static_cast<NotificationServer*>(parent())->notificationAdded(notification);
    } else {
        emit static_cast<NotificationServer*>(parent())->notificationReplaced(notification.id(), notification);
    }

    return notification.id();
}

uint NotificationServerPrivate::Inhibit(const QString &desktop_entry, const QString &reason, const QVariantMap &hints)
{
    const QString dbusService = message().service();

    qCDebug(NOTIFICATIONMANAGER) << "Request inhibit from service" << dbusService << "which is" << desktop_entry << "with reason" << reason;

    if (desktop_entry.isEmpty()) {
        // TODO return error
        return 0;
    }

    KService::Ptr service = KService::serviceByDesktopName(desktop_entry);
    QString applicationName;
    if (service) { // should we check for this and error if it didn't find a service?
        applicationName = service->name();
    }

    m_inhibitionWatcher->addWatchedService(dbusService);

    ++m_highestInhibitionCookie;

    m_inhibitions.insert(m_highestInhibitionCookie, {
        desktop_entry,
        applicationName,
        reason,
        hints
    });

    m_inhibitionServices.insert(m_highestInhibitionCookie, dbusService);

    emit inhibitedChanged();
    emit inhibitionAdded();

    return m_highestInhibitionCookie;
}

void NotificationServerPrivate::onServiceUnregistered(const QString &serviceName)
{
    qCDebug(NOTIFICATIONMANAGER) << "Inhibition service unregistered" << serviceName;

    const uint cookie = m_inhibitionServices.key(serviceName);
    if (!cookie) {
        qCInfo(NOTIFICATIONMANAGER) << "Unknown inhibition service unregistered" << serviceName;
        return;
    }

    // We do lookups in there again...
    UnInhibit(cookie);
}

void NotificationServerPrivate::UnInhibit(uint cookie)
{
    qCDebug(NOTIFICATIONMANAGER) << "Request release inhibition for cookie" << cookie;

    const QString service = m_inhibitionServices.value(cookie);
    if (service.isEmpty()) {
        qCInfo(NOTIFICATIONMANAGER) << "Requested to release inhibition with cookie" << cookie << "that doesn't exist";
        // TODO if called from dbus raise error
        return;
    }

    m_inhibitionWatcher->removeWatchedService(service);
    m_inhibitions.remove(cookie);
    m_inhibitionServices.remove(cookie);

    if (m_inhibitions.isEmpty()) {
        emit inhibitedChanged();
        emit inhibitionRemoved();
    }
}

/*QList<Inhibition> NotificationServerPrivate::ListInhibitors() const
{
    return m_inhibitions.values();
}*/

QList<Inhibition> NotificationServerPrivate::inhibitions() const
{
    return m_inhibitions.values();
}

bool NotificationServerPrivate::inhibited() const
{
    return !m_inhibitions.isEmpty();
}

void NotificationServerPrivate::clearInhibitions()
{
    if (m_inhibitions.isEmpty()) {
        return;
    }

    m_inhibitionWatcher->setWatchedServices(QStringList()); // remove all watches
    m_inhibitionServices.clear();
    m_inhibitions.clear();
    emit inhibitedChanged();
    emit inhibitionRemoved();
}
