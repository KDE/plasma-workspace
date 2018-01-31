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

#include <QDebug>
#include <KConfigGroup>
#include <klocalizedstring.h>
#include <KNotifyConfigWidget>
#include <QGuiApplication>

#include <QRegularExpression>

#include <Plasma/DataContainer>
#include <Plasma/Service>

#include <QImage>

#include <kiconloader.h>
#include <KConfig>

// for ::kill
#include <signal.h>

NotificationsEngine::NotificationsEngine( QObject* parent, const QVariantList& args )
    : Plasma::DataEngine( parent, args ), m_nextId( 1 ), m_alwaysReplaceAppsList({QStringLiteral("Clementine"), QStringLiteral("Spotify"), QStringLiteral("Amarok")})
{
    new NotificationsAdaptor(this);

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
                qDebug() << "Terminating current Notification service with pid" << pid;
                // Now finally terminate the service and register our own
                ::kill(pid, SIGTERM);
                // Wait 3 seconds and then try registering it again
                QTimer::singleShot(3000, this, &NotificationsEngine::registerDBusService);
            }
        }
    }

    // Read additional single-notification-popup-only from a config file
    KConfig singlePopupConfig(QStringLiteral("plasma_single_popup_notificationrc"));
    KConfigGroup singlePopupConfigGroup(&singlePopupConfig, "General");
    m_alwaysReplaceAppsList += QSet<QString>::fromList(singlePopupConfigGroup.readEntry("applications", QStringList()));
}

NotificationsEngine::~NotificationsEngine()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService( QStringLiteral("org.freedesktop.Notifications") );
}

void NotificationsEngine::init()
{
}

bool NotificationsEngine::registerDBusService()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    bool so = dbus.registerService(QStringLiteral("org.freedesktop.Notifications"));
    if (so) {
        bool ro = dbus.registerObject(QStringLiteral("/org/freedesktop/Notifications"), this);
        if (ro) {
            qDebug() << "Notifications service registered";
            return true;
        } else {
            dbus.unregisterService(QStringLiteral("org.freedesktop.Notifications"));
        }
    }

    qDebug() << "Failed to register Notifications service";
    return false;
}

inline void copyLineRGB32(QRgb* dst, const char* src, int width)
{
    const char* end = src + width * 3;
    for (; src != end; ++dst, src+=3) {
        *dst = qRgb(src[0], src[1], src[2]);
    }
}

inline void copyLineARGB32(QRgb* dst, const char* src, int width)
{
    const char* end = src + width * 4;
    for (; src != end; ++dst, src+=4) {
        *dst = qRgba(src[0], src[1], src[2], src[3]);
    }
}

static QImage decodeNotificationSpecImageHint(const QDBusArgument& arg)
{
    int width, height, rowStride, hasAlpha, bitsPerSample, channels;
    QByteArray pixels;
    char* ptr;
    char* end;

    arg.beginStructure();
    arg >> width >> height >> rowStride >> hasAlpha >> bitsPerSample >> channels >> pixels;
    arg.endStructure();
    //qDebug() << width << height << rowStride << hasAlpha << bitsPerSample << channels;

    #define SANITY_CHECK(condition) \
    if (!(condition)) { \
        qWarning() << "Sanity check failed on" << #condition; \
        return QImage(); \
    }

    SANITY_CHECK(width > 0);
    SANITY_CHECK(width < 2048);
    SANITY_CHECK(height > 0);
    SANITY_CHECK(height < 2048);
    SANITY_CHECK(rowStride > 0);

    #undef SANITY_CHECK

    QImage::Format format = QImage::Format_Invalid;
    void (*fcn)(QRgb*, const char*, int) = 0;
    if (bitsPerSample == 8) {
        if (channels == 4) {
            format = QImage::Format_ARGB32;
            fcn = copyLineARGB32;
        } else if (channels == 3) {
            format = QImage::Format_RGB32;
            fcn = copyLineRGB32;
        }
    }
    if (format == QImage::Format_Invalid) {
        qWarning() << "Unsupported image format (hasAlpha:" << hasAlpha << "bitsPerSample:" << bitsPerSample << "channels:" << channels << ")";
        return QImage();
    }

    QImage image(width, height, format);
    ptr = pixels.data();
    end = ptr + pixels.length();
    for (int y=0; y<height; ++y, ptr += rowStride) {
        if (ptr + channels * width > end) {
            qWarning() << "Image data is incomplete. y:" << y << "height:" << height;
            break;
        }
        fcn((QRgb*)image.scanLine(y), ptr, width);
    }

    return image;
}

static QString findImageForSpecImagePath(const QString &_path)
{
    QString path = _path;
    if (path.startsWith(QLatin1String("file:"))) {
        QUrl url(path);
        path = url.toLocalFile();
    }
    return KIconLoader::global()->iconPath(path, -KIconLoader::SizeHuge,
                                           true /* canReturnNull */);
}

uint NotificationsEngine::Notify(const QString &app_name, uint replaces_id,
                                 const QString &app_icon, const QString &summary, const QString &body,
                                 const QStringList &actions, const QVariantMap &hints, int timeout)
{
    uint partOf = 0;
    const QString appRealName = hints[QStringLiteral("x-kde-appname")].toString();
    const QString eventId = hints[QStringLiteral("x-kde-eventId")].toString();
    const bool skipGrouping = hints[QStringLiteral("x-kde-skipGrouping")].toBool();

    // group notifications that have the same title coming from the same app
    // or if they are on the "blacklist", honor the skipGrouping hint sent
    if (!replaces_id && m_activeNotifications.values().contains(app_name + summary) && !skipGrouping && !m_alwaysReplaceAppsList.contains(app_name)) {
        // cut off the "notification " from the source name
        partOf = m_activeNotifications.key(app_name + summary).midRef(13).toUInt();
    }

    qDebug() << "Currrent active notifications:" << m_activeNotifications;
    qDebug() << "Guessing partOf as:" << partOf;
    qDebug() << " New Notification: " << summary << body << timeout << "& Part of:" << partOf;
    QString _body;

    if (partOf > 0) {
        const QString source = QStringLiteral("notification %1").arg(partOf);
        Plasma::DataContainer *container = containerForSource(source);
        if (container) {
            // append the body text
            _body = container->data()[QStringLiteral("body")].toString();
            if (_body != body) {
                _body.append("\n").append(body);
            } else {
                _body = body;
            }

            replaces_id = partOf;

            // remove the old notification and replace it with the new one
            // TODO: maybe just update the current notification?
            CloseNotification(partOf);
        }
    }

    uint id = replaces_id ? replaces_id : m_nextId++;

    // If the current app is in the "blacklist"...
    if (m_alwaysReplaceAppsList.contains(app_name)) {
        // ...check if we already have a notification from that particular
        // app and if yes, use its id to replace it
        if (m_notificationsFromReplaceableApp.contains(app_name)) {
            id = m_notificationsFromReplaceableApp.value(app_name);
        } else {
            m_notificationsFromReplaceableApp.insert(app_name, id);
        }
    }

    QString appname_str = app_name;
    if (appname_str.isEmpty()) {
        appname_str = i18n("Unknown Application");
    }

    bool isPersistent = timeout == 0;

    const int AVERAGE_WORD_LENGTH = 6;
    const int WORD_PER_MINUTE = 250;
    int count = summary.length() + body.length();

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

    QString bodyFinal = (partOf == 0 ? body : _body);
    bodyFinal = NotificationSanitizer::parse(bodyFinal);

    Plasma::DataEngine::Data notificationData;
    notificationData.insert(QStringLiteral("id"), QString::number(id));
    notificationData.insert(QStringLiteral("eventId"), eventId);
    notificationData.insert(QStringLiteral("appName"), appname_str);
    notificationData.insert(QStringLiteral("appIcon"), app_icon);
    notificationData.insert(QStringLiteral("summary"), summary);
    notificationData.insert(QStringLiteral("body"), bodyFinal);
    notificationData.insert(QStringLiteral("actions"), actions);
    notificationData.insert(QStringLiteral("isPersistent"), isPersistent);
    notificationData.insert(QStringLiteral("expireTimeout"), timeout);

    bool configurable = false;
    if (!appRealName.isEmpty()) {

        if (m_configurableApplications.contains(appRealName)) {
            configurable = m_configurableApplications.value(appRealName);
        } else {
            // Check whether the application actually has notifications we can configure
            QScopedPointer<KConfig> config(new KConfig(appRealName + QStringLiteral(".notifyrc"), KConfig::NoGlobals));
            config->addConfigSources(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                     QStringLiteral("knotifications5/") + appRealName + QStringLiteral(".notifyrc")));

            const QRegularExpression regexp(QStringLiteral("^Event/([^/]*)$"));
            configurable = !config->groupList().filter(regexp).isEmpty();
            m_configurableApplications.insert(appRealName, configurable);
        }
    }
    notificationData.insert(QStringLiteral("appRealName"), appRealName);
    notificationData.insert(QStringLiteral("configurable"), configurable);

    QImage image;
    // Underscored hints was in use in version 1.1 of the spec but has been
    // replaced by dashed hints in version 1.2. We need to support it for
    // users of the 1.2 version of the spec.
    if (hints.contains(QStringLiteral("image-data"))) {
        QDBusArgument arg = hints[QStringLiteral("image-data")].value<QDBusArgument>();
        image = decodeNotificationSpecImageHint(arg);
    } else if (hints.contains(QStringLiteral("image_data"))) {
        QDBusArgument arg = hints[QStringLiteral("image_data")].value<QDBusArgument>();
        image = decodeNotificationSpecImageHint(arg);
    } else if (hints.contains(QStringLiteral("image-path"))) {
        QString path = findImageForSpecImagePath(hints[QStringLiteral("image-path")].toString());
        if (!path.isEmpty()) {
            image.load(path);
        }
    } else if (hints.contains(QStringLiteral("image_path"))) {
        QString path = findImageForSpecImagePath(hints[QStringLiteral("image_path")].toString());
        if (!path.isEmpty()) {
            image.load(path);
        }
    } else if (hints.contains(QStringLiteral("icon_data"))) {
        // This hint was in use in version 1.0 of the spec but has been
        // replaced by "image_data" in version 1.1. We need to support it for
        // users of the 1.0 version of the spec.
        QDBusArgument arg = hints[QStringLiteral("icon_data")].value<QDBusArgument>();
        image = decodeNotificationSpecImageHint(arg);
    }
    notificationData.insert(QStringLiteral("image"), image.isNull() ? QVariant() : image);

    if (hints.contains(QStringLiteral("urgency"))) {
        notificationData.insert(QStringLiteral("urgency"), hints[QStringLiteral("urgency")].toInt());
    }

    setData(source, notificationData);

    m_activeNotifications.insert(source, app_name + summary);

    return id;
}

void NotificationsEngine::CloseNotification(uint id)
{
    removeNotification(id, 3);
}

void NotificationsEngine::removeNotification(uint id, uint closeReason)
{
    const QString source = QStringLiteral("notification %1").arg(id);
    // if we don't have that notification in our local list,
    // it has already been closed so don't notify a second time
    if (m_activeNotifications.remove(source) > 0) {
        removeSource(source);
        emit NotificationClosed(id, closeReason);
    }
}

Plasma::Service* NotificationsEngine::serviceForSource(const QString& source)
{
    return new NotificationService(this, source);
}

QStringList NotificationsEngine::GetCapabilities()
{
    return QStringList()
        << QStringLiteral("body")
        << QStringLiteral("body-hyperlinks")
        << QStringLiteral("body-markup")
        << QStringLiteral("icon-static")
        << QStringLiteral("actions")
        ;
}

// FIXME: Signature is ugly
QString NotificationsEngine::GetServerInformation(QString& vendor, QString& version, QString& specVersion)
{
    vendor = QLatin1String("KDE");
    version = QLatin1String("2.0"); // FIXME
    specVersion = QLatin1String("1.1");
    return QStringLiteral("Plasma");
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

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(notifications, NotificationsEngine, "plasma-dataengine-notifications.json")

#include "notificationsengine.moc"
