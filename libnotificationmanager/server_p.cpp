/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "server_p.h"

#include "debug.h"

#include "notificationmanageradaptor.h"
#include "notificationsadaptor.h"

#include "notification_p.h"

#include "server.h"
#include "serverinfo.h"

#include "utils_p.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>

#include <KConfigGroup>
#include <KService>
#include <KSharedConfig>
#include <KUser>

using namespace NotificationManager;

ServerPrivate::ServerPrivate(QObject *parent)
    : QObject(parent)
    , m_inhibitionWatcher(new QDBusServiceWatcher(this))
    , m_notificationWatchers(new QDBusServiceWatcher(this))
{
    m_inhibitionWatcher->setConnection(QDBusConnection::sessionBus());
    m_inhibitionWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_inhibitionWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &ServerPrivate::onInhibitionServiceUnregistered);

    m_notificationWatchers->setConnection(QDBusConnection::sessionBus());
    m_notificationWatchers->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_notificationWatchers, &QDBusServiceWatcher::serviceUnregistered, [this](const QString &service) {
        m_notificationWatchers->removeWatchedService(service);
    });
}

ServerPrivate::~ServerPrivate() = default;

QString ServerPrivate::notificationServiceName()
{
    return QStringLiteral("org.freedesktop.Notifications");
}

QString ServerPrivate::notificationServicePath()
{
    return QStringLiteral("/org/freedesktop/Notifications");
}

QString ServerPrivate::notificationServiceInterface()
{
    return notificationServiceName();
}

ServerInfo *ServerPrivate::currentOwner() const
{
    if (!m_currentOwner) {
        m_currentOwner.reset(new ServerInfo());
    }

    return m_currentOwner.get();
}

bool ServerPrivate::init()
{
    if (m_valid) {
        return true;
    }

    new NotificationsAdaptor(this);
    new NotificationManagerAdaptor(this);

    if (!m_dbusObjectValid) { // if already registered, don't fail here
        m_dbusObjectValid = QDBusConnection::sessionBus().registerObject(notificationServicePath(), this);
    }

    if (!m_dbusObjectValid) {
        qCWarning(NOTIFICATIONMANAGER) << "Failed to register Notification DBus object";
        return false;
    }

    // Only the "dbus master" (effectively plasmashell) should be the true owner of notifications
    const bool master = Utils::isDBusMaster();

    QDBusConnectionInterface *dbusIface = QDBusConnection::sessionBus().interface();

    if (!master) {
        // NOTE this connects to whether the application lost ownership of given service
        // This is not a wildcard listener for all unregistered services on the bus!
        connect(dbusIface, &QDBusConnectionInterface::serviceUnregistered, this, &ServerPrivate::onServiceOwnershipLost, Qt::UniqueConnection);
    }

    auto registration = dbusIface->registerService(notificationServiceName(),
                                                   master ? QDBusConnectionInterface::ReplaceExistingService : QDBusConnectionInterface::DontQueueService,
                                                   master ? QDBusConnectionInterface::DontAllowReplacement : QDBusConnectionInterface::AllowReplacement);
    if (registration.value() != QDBusConnectionInterface::ServiceRegistered) {
        qCWarning(NOTIFICATIONMANAGER) << "Failed to register Notification service on DBus";
        return false;
    }

    connect(this, &ServerPrivate::inhibitedChanged, this, &ServerPrivate::onInhibitedChanged, Qt::UniqueConnection);

    qCDebug(NOTIFICATIONMANAGER) << "Registered Notification service on DBus";

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Notifications"));
    const bool broadcastsEnabled = config.readEntry("ListenForBroadcasts", false);

    if (broadcastsEnabled) {
        qCDebug(NOTIFICATIONMANAGER) << "Notification server is configured to listen for broadcasts";
        // NOTE Keep disconnect() call in onServiceOwnershipLost in sync if you change this!
        QDBusConnection::systemBus().connect({},
                                             {},
                                             QStringLiteral("org.kde.BroadcastNotifications"),
                                             QStringLiteral("Notify"),
                                             this,
                                             SLOT(onBroadcastNotification(QMap<QString, QVariant>)));
    }

    Notification::Private::s_imageCache.setMaxCost(256 * 256 * 100);

    m_valid = true;
    Q_EMIT validChanged();

    return true;
}

uint ServerPrivate::Notify(const QString &app_name,
                           uint replaces_id,
                           const QString &app_icon,
                           const QString &summary,
                           const QString &body,
                           const QStringList &actions,
                           const QVariantMap &hints,
                           int timeout)
{
    const bool wasReplaced = replaces_id > 0;
    uint notificationId = 0;
    if (wasReplaced) {
        notificationId = replaces_id;
    } else {
        // Avoid wrapping around to 0 in case of overflow
        if (!m_highestNotificationId) {
            ++m_highestNotificationId;
        }
        notificationId = m_highestNotificationId;
        ++m_highestNotificationId;
    }

    Notification notification(notificationId);
    notification.setDBusService(message().service());
    notification.setSummary(summary);
    notification.setBody(body);
    notification.setApplicationName(app_name);

    notification.setActions(actions);

    notification.setTimeout(timeout);
    notification.setWasAddedDuringInhibition(m_inhibited);

    // might override some of the things we set above (like application name)
    notification.d->processHints(hints);

    // If we got a pixmap, use app_icon as application icon,
    // otherwise use it as the notification icon, except for default events
    // since the generic Plasma icon isn't any better than the application icon.
    if (!app_icon.isEmpty()) {
        if (notification.d->s_imageCache.contains(notificationId) && !notification.isDefaultEvent()) {
            notification.setApplicationIconName(app_icon);
        } else {
            notification.setIcon(app_icon);
        }
    }

    uint pid = 0;
    if (notification.desktopEntry().isEmpty() || notification.applicationName().isEmpty()) {
        if (notification.desktopEntry().isEmpty() && notification.applicationName().isEmpty()) {
            qCInfo(NOTIFICATIONMANAGER) << "Notification from service" << message().service()
                                        << "didn't contain any identification information. This is a bug in the application" << app_name;
        }
        QDBusReply<uint> pidReply = connection().interface()->servicePid(message().service());
        if (pidReply.isValid()) {
            pid = pidReply.value();
        }
    }

    // No desktop entry? Try to read the BAMF_DESKTOP_FILE_HINT in the environment of snaps
    if (notification.desktopEntry().isEmpty() && pid > 0) {
        const QString desktopEntry = Utils::desktopEntryFromPid(pid);
        if (!desktopEntry.isEmpty()) {
            qCDebug(NOTIFICATIONMANAGER) << "Resolved notification to be from desktop entry" << desktopEntry;
            notification.setDesktopEntry(desktopEntry);

            // No application name? Set it to the service name, which is nicer than the process name fallback below
            // Also if the title looks like it's just the desktop entry, use the nicer service name
            if (notification.applicationName().isEmpty() || notification.applicationName() == desktopEntry) {
                KService::Ptr service = KService::serviceByDesktopName(desktopEntry);
                if (service) {
                    notification.setApplicationName(service->name());
                }
            }
        }
    }

    // No application name? Try to figure out the process name using the sender's PID
    if (notification.applicationName().isEmpty() && pid > 0) {
        const QString processName = Utils::processNameFromPid(pid);
        if (!processName.isEmpty()) {
            qCDebug(NOTIFICATIONMANAGER) << "Resolved notification to be from process name" << processName;
            notification.setApplicationName(processName);
        }
    }

    // If multiple identical notifications are sent in quick succession, refuse the request
    if (m_lastNotification.applicationName() == notification.applicationName() && m_lastNotification.summary() == notification.summary()
        && m_lastNotification.body() == notification.body() && m_lastNotification.desktopEntry() == notification.desktopEntry()
        && m_lastNotification.eventId() == notification.eventId() && m_lastNotification.actionNames() == notification.actionNames()
        && m_lastNotification.icon() == notification.icon() && m_lastNotification.urls() == notification.urls()
        && m_lastNotification.created().msecsTo(notification.created()) < 1000) {
        qCDebug(NOTIFICATIONMANAGER) << "Discarding excess notification creation request";

        sendErrorReply(QStringLiteral("org.freedesktop.Notifications.Error.ExcessNotificationGeneration"),
                       QStringLiteral("Created too many similar notifications in quick succession"));
        return 0;
    }

    m_lastNotification = notification;

    if (wasReplaced) {
        notification.resetUpdated();
        Q_EMIT static_cast<Server *>(parent())->notificationReplaced(replaces_id, notification);
    } else {
        Q_EMIT static_cast<Server *>(parent())->notificationAdded(notification);
    }

    // currently we dispatch all notification, this is ugly
    // TODO: come up with proper authentication/user selection
    for (const QString &service : m_notificationWatchers->watchedServices()) {
        QDBusMessage msg = QDBusMessage::createMethodCall(service,
                                                          QStringLiteral("/NotificationWatcher"),
                                                          QStringLiteral("org.kde.NotificationWatcher"),
                                                          QStringLiteral("Notify"));
        msg.setArguments({notificationId,
                          notification.applicationName(),
                          replaces_id,
                          notification.applicationIconName(),
                          notification.summary(),
                          // we pass raw body data since this data goes through another sanitization
                          // in WatchedNotificationsModel when notification object is created.
                          notification.rawBody(),
                          actions,
                          hints,
                          notification.timeout()});
        QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
    }

    return notificationId;
}

void ServerPrivate::CloseNotification(uint id)
{
    for (const QString &service : m_notificationWatchers->watchedServices()) {
        QDBusMessage msg = QDBusMessage::createMethodCall(service,
                                                          QStringLiteral("/NotificationWatcher"),
                                                          QStringLiteral("org.kde.NotificationWatcher"),
                                                          QStringLiteral("CloseNotification"));
        msg.setArguments({id});
        QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
    }
    // spec says "If the notification no longer exists, an empty D-BUS Error message is sent back."
    static_cast<Server *>(parent())->closeNotification(id, Server::CloseReason::Revoked);
}

QStringList ServerPrivate::GetCapabilities() const
{
    // should this be configurable somehow so the UI can tell what it implements?
    return QStringList{QStringLiteral("body"),
                       QStringLiteral("body-hyperlinks"),
                       QStringLiteral("body-markup"),
                       QStringLiteral("body-images"),
                       QStringLiteral("icon-static"),
                       QStringLiteral("actions"),
                       QStringLiteral("persistence"),
                       QStringLiteral("inline-reply"),
                       QStringLiteral("sound"),
                       QStringLiteral("x-kde-urls"),
                       QStringLiteral("x-kde-origin-name"),
                       QStringLiteral("x-kde-display-appname"),

                       QStringLiteral("inhibitions")};
}

QString ServerPrivate::GetServerInformation(QString &vendor, QString &version, QString &specVersion) const
{
    vendor = QStringLiteral("KDE");
    version = QLatin1String(PROJECT_VERSION);
    specVersion = QStringLiteral("1.2");
    return QStringLiteral("Plasma");
}

void ServerPrivate::onBroadcastNotification(const QMap<QString, QVariant> &properties)
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

    Notify(properties.value(QStringLiteral("appName")).toString(),
           0, // replaces_id
           properties.value(QStringLiteral("appIcon")).toString(),
           properties.value(QStringLiteral("summary")).toString(),
           properties.value(QStringLiteral("body")).toString(),
           {}, // no actions
           properties.value(QStringLiteral("hints")).toMap(),
           timeout);
}

uint ServerPrivate::add(const Notification &notification)
{
    // TODO check if notification with ID already exists and signal update instead
    if (notification.id() == 0) {
        ++m_highestNotificationId;
        notification.d->id = m_highestNotificationId;

        Q_EMIT static_cast<Server *>(parent())->notificationAdded(notification);
    } else {
        Q_EMIT static_cast<Server *>(parent())->notificationReplaced(notification.id(), notification);
    }

    return notification.id();
}

void ServerPrivate::sendReplyText(const QString &dbusService, uint notificationId, const QString &text, Notifications::InvokeBehavior behavior)
{
    if (dbusService.isEmpty()) {
        qCWarning(NOTIFICATIONMANAGER) << "Sending notification reply text for notification" << notificationId << "untargeted";
    }

    QDBusMessage msg =
        QDBusMessage::createTargetedSignal(dbusService, notificationServicePath(), notificationServiceName(), QStringLiteral("NotificationReplied"));
    msg.setArguments({notificationId, text});
    QDBusConnection::sessionBus().send(msg);

    if (behavior & Notifications::Close) {
        Q_EMIT CloseNotification(notificationId);
    }
}

uint ServerPrivate::Inhibit(const QString &desktop_entry, const QString &reason, const QVariantMap &hints)
{
    const QString dbusService = message().service();

    QString applicationName = desktop_entry;

    qCDebug(NOTIFICATIONMANAGER) << "Request inhibit from service" << dbusService << "which is" << desktop_entry << "with reason" << reason;

    // xdg-desktop-portal forwards appId only for sandboxed apps it can trust
    // Resolve it to process name here to at least have something, even if that means showing "xdg-desktop-portal-kde is currently..."
    if (desktop_entry.isEmpty()) {
        QDBusReply<uint> pidReply = connection().interface()->servicePid(message().service());
        if (pidReply.isValid()) {
            const auto pid = pidReply.value();

            const QString processName = Utils::processNameFromPid(pid);
            if (!processName.isEmpty()) {
                qCDebug(NOTIFICATIONMANAGER) << "Resolved inhibition to be from process name" << processName;
                applicationName = processName;
            }
        }
    } else {
        KService::Ptr service = KService::serviceByDesktopName(desktop_entry);
        if (service) {
            applicationName = service->name();
        }
    }

    if (applicationName.isEmpty()) {
        sendErrorReply(QDBusError::InvalidArgs, QStringLiteral("No meaningful desktop_entry provided"));
        return 0;
    }

    m_inhibitionWatcher->addWatchedService(dbusService);

    ++m_highestInhibitionCookie;

    const bool oldExternalInhibited = externalInhibited();

    m_externalInhibitions.insert(m_highestInhibitionCookie, {desktop_entry, applicationName, reason, hints});

    m_inhibitionServices.insert(m_highestInhibitionCookie, dbusService);

    if (externalInhibited() != oldExternalInhibited) {
        Q_EMIT externalInhibitedChanged();
    }
    Q_EMIT externalInhibitionsChanged();

    return m_highestInhibitionCookie;
}

void ServerPrivate::onServiceOwnershipLost(const QString &serviceName)
{
    if (serviceName != notificationServiceName()) {
        return;
    }

    qCDebug(NOTIFICATIONMANAGER) << "Lost ownership of" << serviceName << "service";

    disconnect(QDBusConnection::sessionBus().interface(), &QDBusConnectionInterface::serviceUnregistered, this, &ServerPrivate::onServiceOwnershipLost);
    disconnect(this, &ServerPrivate::inhibitedChanged, this, &ServerPrivate::onInhibitedChanged);

    QDBusConnection::systemBus().disconnect({},
                                            {},
                                            QStringLiteral("org.kde.BroadcastNotifications"),
                                            QStringLiteral("Notify"),
                                            this,
                                            SLOT(onBroadcastNotification(QMap<QString, QVariant>)));

    m_valid = false;

    Q_EMIT validChanged();
    Q_EMIT serviceOwnershipLost();
}

void ServerPrivate::onInhibitionServiceUnregistered(const QString &serviceName)
{
    qCDebug(NOTIFICATIONMANAGER) << "Inhibition service unregistered" << serviceName;

    const QList<uint> cookies = m_inhibitionServices.keys(serviceName);
    if (cookies.isEmpty()) {
        qCInfo(NOTIFICATIONMANAGER) << "Unknown inhibition service unregistered" << serviceName;
        return;
    }

    // We do lookups in there again...
    for (uint cookie : cookies) {
        UnInhibit(cookie);
    }
}

void ServerPrivate::onInhibitedChanged()
{
    // Q_EMIT DBus change signal...
    QDBusMessage signal =
        QDBusMessage::createSignal(notificationServicePath(), QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("PropertiesChanged"));

    signal.setArguments({
        notificationServiceInterface(),
        QVariantMap{
            // updated
            {QStringLiteral("Inhibited"), inhibited()},
        },
        QStringList() // invalidated
    });

    QDBusConnection::sessionBus().send(signal);
}

void ServerPrivate::UnInhibit(uint cookie)
{
    qCDebug(NOTIFICATIONMANAGER) << "Request release inhibition for cookie" << cookie;

    const QString service = m_inhibitionServices.value(cookie);
    if (service.isEmpty()) {
        qCInfo(NOTIFICATIONMANAGER) << "Requested to release inhibition with cookie" << cookie << "that doesn't exist";
        // TODO if called from dbus raise error
        return;
    }

    m_inhibitionWatcher->removeWatchedService(service);
    m_externalInhibitions.remove(cookie);
    m_inhibitionServices.remove(cookie);

    if (m_externalInhibitions.isEmpty()) {
        Q_EMIT externalInhibitedChanged();
    }
    Q_EMIT externalInhibitionsChanged();
}

QList<Inhibition> ServerPrivate::externalInhibitions() const
{
    return m_externalInhibitions.values();
}

bool ServerPrivate::inhibited() const
{
    return m_inhibited;
}

void ServerPrivate::setInhibited(bool inhibited)
{
    if (m_inhibited != inhibited) {
        m_inhibited = inhibited;
        Q_EMIT inhibitedChanged();
    }
}

bool ServerPrivate::externalInhibited() const
{
    return !m_externalInhibitions.isEmpty();
}

void ServerPrivate::clearExternalInhibitions()
{
    if (m_externalInhibitions.isEmpty()) {
        return;
    }

    m_inhibitionWatcher->setWatchedServices(QStringList()); // remove all watches
    m_inhibitionServices.clear();
    m_externalInhibitions.clear();

    Q_EMIT externalInhibitedChanged();
    Q_EMIT externalInhibitionsChanged();
}

void ServerPrivate::RegisterWatcher()
{
    m_notificationWatchers->addWatchedService(message().service());
}

void ServerPrivate::UnRegisterWatcher()
{
    m_notificationWatchers->removeWatchedService(message().service());
}

void ServerPrivate::InvokeAction(uint id, const QString &actionKey)
{
    Q_EMIT ActionInvoked(id, actionKey);
}

#include "moc_server_p.cpp"
