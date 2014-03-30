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

#include <QDebug>
#include <KConfigGroup>
#include <klocalizedstring.h>
#include <KNotifyConfigWidget>

#include <Plasma/DataContainer>
#include <Plasma/Service>

#include <QImage>

#include <kiconloader.h>

NotificationsEngine::NotificationsEngine( QObject* parent, const QVariantList& args )
    : Plasma::DataEngine( parent, args ), m_nextId( 1 )
{
    new NotificationsAdaptor(this);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    bool so = dbus.registerService( "org.freedesktop.Notifications" );
    bool ro = dbus.registerObject( "/org/freedesktop/Notifications", this );
    qDebug() << "Are we the only client? (Both have to be true) " << so << ro;
}

NotificationsEngine::~NotificationsEngine()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService( "org.freedesktop.Notifications" );
}

void NotificationsEngine::init()
{
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

    if (m_activeNotifications.values().contains(app_name + summary)) {
        // cut off the "notification " from the source name
        partOf = m_activeNotifications.key(app_name + summary).mid(13).toUInt();

    }

    qDebug() << "Currrent active notifications:" << m_activeNotifications;
    qDebug() << "Guessing partOf as:" << partOf;
    qDebug() << " New Notification: " << summary << body << timeout << "& Part of:" << partOf;
    QString _body;

    if (partOf > 0) {
        const QString source = QString("notification %1").arg(partOf);
        Plasma::DataContainer *container = containerForSource(source);
        if (container) {
            // append the body text
            _body = container->data()["body"].toString();
            if (_body != body) {
                _body.append("\n").append(body);
            } else {
                _body = body;
            }

            // remove the old notification and replace it with the new one
            // TODO: maybe just update the current notification?
            CloseNotification(partOf);
        }
    }

    uint id = 0;
    id = replaces_id ? replaces_id : m_nextId++;

    QString appname_str = app_name;
    if (appname_str.isEmpty()) {
        appname_str = i18n("Unknown Application");
    }

    bool isPersistent = timeout == 0;

    const int AVERAGE_WORD_LENGTH = 6;
    const int WORD_PER_MINUTE = 250;
    int count = summary.length() + body.length();
    timeout = 60000 * count / AVERAGE_WORD_LENGTH / WORD_PER_MINUTE;

    // Add two seconds for the user to notice the notification, and ensure
    // it last at least five seconds, otherwise all the user see is a
    // flash
    timeout = 2000 + qMax(timeout, 3000);

    const QString source = QString("notification %1").arg(id);

    Plasma::DataEngine::Data notificationData;
    notificationData.insert("id", QString::number(id));
    notificationData.insert("appName", appname_str);
    notificationData.insert("appIcon", app_icon);
    notificationData.insert("summary", summary);
    notificationData.insert("body", partOf == 0 ? body : _body);
    notificationData.insert("actions", actions);
    notificationData.insert("isPersistent", isPersistent);
    notificationData.insert("expireTimeout", timeout);

    QString appRealName;
    bool configurable = false;
    if (hints.contains("x-kde-appname")) {
        appRealName = hints["x-kde-appname"].toString();
        configurable = true;
    }
    notificationData.insert("appRealName", appRealName);
    notificationData.insert("configurable", configurable);

    QImage image;
    if (hints.contains("image_data")) {
        QDBusArgument arg = hints["image_data"].value<QDBusArgument>();
        image = decodeNotificationSpecImageHint(arg);
    } else if (hints.contains("image_path")) {
        QString path = findImageForSpecImagePath(hints["image_path"].toString());
        if (!path.isEmpty()) {
            image.load(path);
        }
    } else if (hints.contains("icon_data")) {
        // This hint was in use in version 1.0 of the spec but has been
        // replaced by "image_data" in version 1.1. We need to support it for
        // users of the 1.0 version of the spec.
        QDBusArgument arg = hints["icon_data"].value<QDBusArgument>();
        image = decodeNotificationSpecImageHint(arg);
    }
    notificationData.insert("image", image);

    if (hints.contains("urgency")) {
        notificationData.insert("urgency", hints["urgency"].toInt());
    }

    setData(source, notificationData);

    m_activeNotifications.insert(source, app_name + summary);

    return id;
}

void NotificationsEngine::CloseNotification(uint id)
{
    const QString source = QString("notification %1").arg(id);
    // if we don't have that notification in our list,
    // it was already closed, so don't emit
    if (m_activeNotifications.remove(source)) {
        removeSource(source);
        emit NotificationClosed(id, 3);
    }
}

void NotificationsEngine::userClosedNotification(uint id)
{
    const QString source = QString("notification %1").arg(id);
    // if we don't have that notification in our list,
    // it was already closed, so don't emit
    if (m_activeNotifications.remove(source) > 0) {
        removeSource(source);
        emit NotificationClosed(id, 2);
    }
}

Plasma::Service* NotificationsEngine::serviceForSource(const QString& source)
{
    return new NotificationService(this, source);
}

QStringList NotificationsEngine::GetCapabilities()
{
    return QStringList()
        << "body"
        << "body-hyperlinks"
        << "body-markup"
        << "icon-static"
        << "actions"
        ;
}

// FIXME: Signature is ugly
QString NotificationsEngine::GetServerInformation(QString& vendor, QString& version, QString& specVersion)
{
    vendor = "KDE";
    version = "2.0"; // FIXME
    specVersion = "1.1";
    return "Plasma";
}

int NotificationsEngine::createNotification(const QString &appName, const QString &appIcon, const QString &summary, const QString &body, int timeout, bool configurable, const QString &appRealName)
{
    const QString source = QString("notification %1").arg(++m_nextId);
    Plasma::DataEngine::Data notificationData;
    notificationData.insert("id", QString::number(m_nextId));
    notificationData.insert("appName", appName);
    notificationData.insert("appIcon", appIcon);
    notificationData.insert("summary", summary);
    notificationData.insert("body", body);
    notificationData.insert("expireTimeout", timeout);
    notificationData.insert("configurable", configurable);
    notificationData.insert("appRealName", appRealName);

    setData(source, notificationData);
    return m_nextId;
}

void NotificationsEngine::configureNotification(const QString &appName)
{
    KNotifyConfigWidget::configure(0, appName);
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(notifications, NotificationsEngine, "plasma-dataengine-notifications.json")

#include "notificationsengine.moc"
