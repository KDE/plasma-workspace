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
#include "notificationsadaptor.h"
#include "notificationsanitizer.h"

#include <notificationmanager/notificationserver.h>
#include <notificationmanager/notification.h>

#include <KConfigGroup>
#include <klocalizedstring.h>
#include <KSharedConfig>
#include <KNotifyConfigWidget>
#include <KUser>
#include <QGuiApplication>

#include <QRegularExpression>

#include <Plasma/DataContainer>
#include <Plasma/Service>

#include <QImage>

#include <kiconloader.h>
#include <KConfig>

// for ::kill
#include <signal.h>

#include "debug.h"

using namespace NotificationManager;

NotificationsEngine::NotificationsEngine( QObject* parent, const QVariantList& args )
    : Plasma::DataEngine( parent, args ), m_nextId( 1 ), m_alwaysReplaceAppsList({QStringLiteral("Clementine"), QStringLiteral("Spotify"), QStringLiteral("Amarok")})
{

    connect(&NotificationServer::self(), &NotificationServer::notificationAdded, this, [this](const Notification &notification) {
        // FIXME handle replaced
        notificationAdded(notification);
    });

    connect(&NotificationServer::self(), &NotificationServer::notificationReplaced, this, [this](uint replacedId, const Notification &notification) {

    });

    connect(&NotificationServer::self(), &NotificationServer::notificationRemoved, this, [this](uint id, NotificationServer::CloseReason reason) {
        const QString source = QStringLiteral("notification %1").arg(id);
        // if we don't have that notification in our local list,
        // it has already been closed so don't notify a second time
        if (m_activeNotifications.remove(source) > 0) {
            removeSource(source);
        }
    });

    // FIXME let the new notification plasmoid do the killing
    /*
    if (!registerDBusService()) {
        QDBusConnection dbus = QDBusConnection::sessionBus();
        // Retrieve the pid of the current o.f.Notifications service
        QDBusReply<uint> pidReply = dbus.interface()->servicePid(QStringLiteral("org.freedesktop.Notifications"));
        uint pid = pidReply.value();
        // Check if it's not the same app as our own
        if (pid != qApp->applicationPid()) {
            QDBusReply<uint> plasmaPidReply = dbus.interface()->servicePid(QStringLiteral("org.kde.plasmashell"));
            // It's not the same but check if it isn't plasma,
            // we don't want to kill Plasma
            if (pid != plasmaPidReply.value()) {
                qCDebug(NOTIFICATIONS) << "Terminating current Notification service with pid" << pid;
                // Now finally terminate the service and register our own
                ::kill(pid, SIGTERM);
                // Wait 3 seconds and then try registering it again
                QTimer::singleShot(3000, this, &NotificationsEngine::registerDBusService);
            }
        }
    }*/

    // FIXME implement in notification server
    /*
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Notifications"));
    const bool broadcastsEnabled = config.readEntry("ListenForBroadcasts", false);

    if (broadcastsEnabled) {
        qCDebug(NOTIFICATIONS) << "Notifications engine is configured to listen for broadcasts";
        QDBusConnection::systemBus().connect({}, {}, QStringLiteral("org.kde.BroadcastNotifications"),
                                             QStringLiteral("Notify"), this, SLOT(onBroadcastNotification(QMap<QString,QVariant>)));
    }*/

    // Read additional single-notification-popup-only from a config file
    //KConfig singlePopupConfig(QStringLiteral("plasma_single_popup_notificationrc"));
    //KConfigGroup singlePopupConfigGroup(&singlePopupConfig, "General");
    //m_alwaysReplaceAppsList += QSet<QString>::fromList(singlePopupConfigGroup.readEntry("applications", QStringList()));
}

NotificationsEngine::~NotificationsEngine()
{

}

void NotificationsEngine::init()
{
}

void NotificationsEngine::notificationAdded(const Notification &notification)
{
    // FIXME
    /*foreach(NotificationInhibiton *ni, m_inhibitions) {
        if (hints[ni->hint] == ni->value) {
            qCDebug(NOTIFICATIONS) << "notification inhibited. Skipping";
            return -1;
        }
    }*/

    uint partOf = 0;
    const QString appRealName; // FIXME = hints[QStringLiteral("x-kde-appname")].toString();
    const QString eventId; // FIXME = hints[QStringLiteral("x-kde-eventId")].toString();
    const bool skipGrouping = false;// FIXME hints[QStringLiteral("x-kde-skipGrouping")].toBool();
    const QStringList urls; // FIXME there's no QUrl toStringList? = notification.urls();
    const QString desktopEntry; // FIXME = hints[QStringLiteral("desktop-entry")].toString();

    // group notifications that have the same title coming from the same app
    // or if they are on the "blacklist", honor the skipGrouping hint sent
    // FIXME
    /*if (!replaces_id && m_activeNotifications.values().contains(app_name + summary) && !skipGrouping && urls.isEmpty() && !m_alwaysReplaceAppsList.contains(app_name)) {
        // cut off the "notification " from the source name
        partOf = m_activeNotifications.key(app_name + summary).midRef(13).toUInt();
    }*/

    //qCDebug(NOTIFICATIONS) << "Currrent active notifications:" << m_activeNotifications;
    //qCDebug(NOTIFICATIONS) << "Guessing partOf as:" << partOf;
    //qCDebug(NOTIFICATIONS) << " New Notification: " << summary << body << timeout << "& Part of:" << partOf;

    QString bodyFinal = notification.body(); // is already sanitized
    QString summaryFinal = notification.summary();
    int timeout = notification.timeout();

    if (partOf > 0) {
        const QString source = QStringLiteral("notification %1").arg(partOf);
        Plasma::DataContainer *container = containerForSource(source);
        if (container) {
            // append the body text
            const QString previousBody = container->data()[QStringLiteral("body")].toString();
            if (previousBody != bodyFinal) {
                // FIXME: This will just append the entire old XML document to another one, leading to:
                // <?xml><html>old</html><br><?xml><html>new</html>
                // It works but is not very clean.
                bodyFinal = previousBody + QStringLiteral("<br/>") + bodyFinal;
            }

            //replaces_id = partOf;

            // remove the old notification and replace it with the new one
            // TODO: maybe just update the current notification?
            //CloseNotification(partOf);
        }
    } else if (bodyFinal.isEmpty()) {
        //some ridiculous apps will send just a title (#372112), in that case, treat it as though there's only a body
        //bodyFinal = summary;
        //summaryFinal = app_name;
    }

    uint id = notification.id();// replaces_id ? replaces_id : m_nextId++;

    // If the current app is in the "blacklist"...
    /*if (m_alwaysReplaceAppsList.contains(app_name)) {
        // ...check if we already have a notification from that particular
        // app and if yes, use its id to replace it
        if (m_notificationsFromReplaceableApp.contains(app_name)) {
            id = m_notificationsFromReplaceableApp.value(app_name);
        } else {
            m_notificationsFromReplaceableApp.insert(app_name, id);
        }
    }*/

    /*QString appname_str = app_name;
    if (appname_str.isEmpty()) {
        appname_str = i18n("Unknown Application");
    }*/

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
    notificationData.insert(QStringLiteral("actions"), QStringList()); // FIXME
    notificationData.insert(QStringLiteral("isPersistent"), isPersistent);
    notificationData.insert(QStringLiteral("expireTimeout"), timeout);

    notificationData.insert(QStringLiteral("desktopEntry"), desktopEntry);

    KService::Ptr service = KService::serviceByStorageId(desktopEntry);
    if (service) {
        notificationData.insert(QStringLiteral("appServiceName"), service->name());
        notificationData.insert(QStringLiteral("appServiceIcon"), service->icon());
    }

    bool configurable = false;
    if (!appRealName.isEmpty()) {

        if (m_configurableApplications.contains(appRealName)) {
            configurable = m_configurableApplications.value(appRealName);
        } else {
            // Check whether the application actually has notifications we can configure
            KConfig config(appRealName + QStringLiteral(".notifyrc"), KConfig::NoGlobals);
            config.addConfigSources(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                    QStringLiteral("knotifications5/") + appRealName + QStringLiteral(".notifyrc")));

            const QRegularExpression regexp(QStringLiteral("^Event/([^/]*)$"));
            configurable = !config.groupList().filter(regexp).isEmpty();
            m_configurableApplications.insert(appRealName, configurable);
        }
    }
    notificationData.insert(QStringLiteral("appRealName"), appRealName);
    notificationData.insert(QStringLiteral("configurable"), configurable);

    QImage image = notification.image();
    notificationData.insert(QStringLiteral("image"), image.isNull() ? QVariant() : image);

    // FIXME did we even have this?
    /*if (hints.contains(QStringLiteral("urgency"))) {
        notificationData.insert(QStringLiteral("urgency"), hints[QStringLiteral("urgency")].toInt());
    }*/

    notificationData.insert(QStringLiteral("urls"), urls);

    setData(source, notificationData);

    m_activeNotifications.insert(source, notification.applicationName() + notification.summary());
}

uint NotificationsEngine::Notify(const QString &app_name, uint replaces_id,
                                 const QString &app_icon, const QString &summary, const QString &body,
                                 const QStringList &actions, const QVariantMap &hints, int timeout)
{
    // FIXME wire this thing up to the new one, it's used by notification action job or something
    return 0;
}

Plasma::Service* NotificationsEngine::serviceForSource(const QString& source)
{
    return new NotificationService(this, source);
}

int NotificationsEngine::createNotification(const QString &appName, const QString &appIcon, const QString &summary,
                                            const QString &body, int timeout, const QStringList &actions, const QVariantMap &hints)
{
    Notify(appName, 0, appIcon, summary, body, actions, hints, timeout);
    return m_nextId;
}

void NotificationsEngine::configureNotification(const QString &appName, const QString &eventId)
{
    KNotifyConfigWidget *widget = KNotifyConfigWidget::configure(nullptr, appName);
    if (!eventId.isEmpty()) {
        widget->selectEvent(eventId);
    }
}

QSharedPointer<NotificationInhibiton> NotificationsEngine::createInhibition(const QString &hint, const QString &value) {
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

void NotificationsEngine::onBroadcastNotification(const QMap<QString, QVariant> &properties)
{
    qCDebug(NOTIFICATIONS) << "Received broadcast notification";

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
            qCDebug(NOTIFICATIONS) << "It is not meant for us, ignoring";
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

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(notifications, NotificationsEngine, "plasma-dataengine-notifications.json")

#include "notificationsengine.moc"
