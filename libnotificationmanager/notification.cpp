/*
 * Copyright 2008 Dmitry Suzdalev <dimsuz@gmail.com>
 * Copyright 2017 David Edmundson <davidedmundson@kde.org>
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

#include "notification.h"

#include <QDebug>
#include <QDBusArgument>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <KConfig>
#include <KConfigGroup>
#include <KIconLoader>
#include <KService>

#include "notifications.h"

using namespace NotificationManager;

static QString sanitize(const QString &text)
{
    // replace all \ns with <br/>
    QString t = text;

    t.replace(QLatin1String("\n"), QStringLiteral("<br/>"));
    // Now remove all inner whitespace (\ns are already <br/>s)
    t = t.simplified();
    // Finally, check if we don't have multiple <br/>s following,
    // can happen for example when "\n       \n" is sent, this replaces
    // all <br/>s in succsession with just one
    t.replace(QRegularExpression(QStringLiteral("<br/>\\s*<br/>(\\s|<br/>)*")), QLatin1String("<br/>"));
    // This fancy RegExp escapes every occurrence of & since QtQuick Text will blatantly cut off
    // text where it finds a stray ampersand.
    // Only &{apos, quot, gt, lt, amp}; as well as &#123 character references will be allowed
    t.replace(QRegularExpression(QStringLiteral("&(?!(?:apos|quot|[gl]t|amp);|#)")), QLatin1String("&amp;"));

    QXmlStreamReader r(QStringLiteral("<html>") + t + QStringLiteral("</html>"));
    QString result;
    QXmlStreamWriter out(&result);

    const QVector<QString> allowedTags = {"b", "i", "u", "img", "a", "html", "br", "table", "tr", "td"};

    out.writeStartDocument();
    while (!r.atEnd()) {
        r.readNext();

        if (r.tokenType() == QXmlStreamReader::StartElement) {
            const QString name = r.name().toString();
            if (!allowedTags.contains(name)) {
                continue;
            }
            out.writeStartElement(name);
            if (name == QLatin1String("img")) {
                auto src = r.attributes().value("src").toString();
                auto alt = r.attributes().value("alt").toString();

                const QUrl url(src);
                if (url.isLocalFile()) {
                    out.writeAttribute(QStringLiteral("src"), src);
                } else {
                    //image denied for security reasons! Do not copy the image src here!
                }

                out.writeAttribute(QStringLiteral("alt"), alt);
            }
            if (name == QLatin1String("a")) {
                out.writeAttribute(QStringLiteral("href"), r.attributes().value("href").toString());
            }
        }

        if (r.tokenType() == QXmlStreamReader::EndElement) {
            const QString name = r.name().toString();
            if (!allowedTags.contains(name)) {
                continue;
            }
            out.writeEndElement();
        }

        if (r.tokenType() == QXmlStreamReader::Characters) {
            const auto text = r.text().toString();
            out.writeCharacters(text); //this auto escapes chars -> HTML entities
        }
    }
    out.writeEndDocument();

    if (r.hasError()) { // FIXME FIXME FIXME
        /*qCWarning(NOTIFICATIONS) << "Notification to send to backend contains invalid XML: "
                      << r.errorString() << "line" << r.lineNumber()
                      << "col" << r.columnNumber();*/
    }

    // The Text.StyledText format handles only html3.2 stuff and &apos; is html4 stuff
    // so we need to replace it here otherwise it will not render at all.
    result = result.replace(QLatin1String("&apos;"), QChar('\''));

    return result;
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

    auto copyLineRGB32 = [](QRgb* dst, const char* src, int width)
    {
        const char* end = src + width * 3;
        for (; src != end; ++dst, src+=3) {
            *dst = qRgb(src[0], src[1], src[2]);
        }
    };

    auto copyLineARGB32 = [](QRgb* dst, const char* src, int width)
    {
        const char* end = src + width * 4;
        for (; src != end; ++dst, src+=4) {
            *dst = qRgba(src[0], src[1], src[2], src[3]);
        }
    };

    QImage::Format format = QImage::Format_Invalid;
    void (*fcn)(QRgb*, const char*, int) = nullptr;
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

Notification::Notification(uint id)
    : m_id(id)
    , m_created(QDateTime::currentDateTimeUtc())
{
    // QVector needs default constructor
    //Q_ASSERT(id > 0);
}

Notification::~Notification() = default;

uint Notification::id() const
{
    return m_id;
}

QDateTime Notification::created() const
{
    return m_created;
}

QDateTime Notification::updated() const
{
    return m_updated;
}

void Notification::setUpdated()
{
    m_updated = QDateTime::currentDateTimeUtc();
}

QString Notification::summary() const
{
    return m_summary;
}

void Notification::setSummary(const QString &summary)
{
    m_summary = summary;
}

QString Notification::body() const
{
    return m_body;
}

void Notification::setBody(const QString &body)
{
    m_body = sanitize(body.trimmed());
}

QString Notification::iconName() const
{
    return m_iconName;
}

void Notification::setIconName(const QString &iconName)
{
    m_iconName = iconName;
}

QImage Notification::image() const
{
    return m_image;
}

void Notification::setImage(const QImage &image)
{
    m_image = image;
}

QString Notification::applicationName() const
{
    return m_applicationName;
}

void Notification::setApplicationName(const QString &applicationName)
{
    m_applicationName = applicationName;

    KService::Ptr service = KService::serviceByStorageId(applicationName);
    if (service) {
        m_applicationName = service->name();
        m_applicationIconName = service->icon();
    }
}

QString Notification::applicationIconName() const
{
    return m_applicationIconName;
}

void Notification::setApplicationIconName(const QString &applicationIconName)
{
    m_applicationIconName = applicationIconName;
}

QStringList Notification::actionNames() const
{
    return m_actionNames;
}

QStringList Notification::actionLabels() const
{
    return m_actionLabels;
}

bool Notification::hasDefaultAction() const
{
    return m_hasDefaultAction;
}

/*bool Notification::hasConfigureAction() const
{
    return m_hasConfigureAction;
}*/

void Notification::setActions(const QStringList &actions)
{
    if (actions.count() % 2 != 0) {
        // FIXME qCWarning
        qWarning() << "List of actions must contain an even number of items, tried to set actions to" << actions;
        return;
    }

    m_hasDefaultAction = false;
    m_hasConfigureAction = false;

    QStringList names;
    QStringList labels;

    for (int i = 0; i < actions.count(); i += 2) {
        const QString &name = actions.at(i);
        const QString &label = actions.at(i + 1);

        if (!m_hasDefaultAction && name == QLatin1String("default")) {
            m_hasDefaultAction = true;
            continue;
        }

        if (!m_hasConfigureAction && name == QLatin1String("settings")) {
            m_hasConfigureAction = true;
            m_configureActionLabel = label;
            continue;
        }

        names << name;
        labels << label;
    }

    m_actionNames = names;
    m_actionLabels = labels;
}

QList<QUrl> Notification::urls() const
{
    return m_urls;
}

void Notification::setUrls(const QList<QUrl> &urls)
{
    m_urls = urls;
}

Notifications::Urgencies Notification::urgency() const
{
    return m_urgency;
}

void Notification::setUrgency(Notifications::Urgencies urgency)
{
    m_urgency = urgency;

    // Critical notifications must not time out
    if (urgency == Notifications::CriticalUrgency) {
        m_timeout = 0;
    }
}

int Notification::timeout() const
{
    return m_timeout;
}

void Notification::setTimeout(int timeout)
{
    m_timeout = timeout;
}

bool Notification::configurable() const
{
    return m_hasConfigureAction || m_configurableNotifyRc;
}

QString Notification::configureActionLabel() const
{
    return m_configureActionLabel;
}

bool Notification::expired() const
{
    return m_expired;
}

void Notification::setExpired(bool expired)
{
    m_expired = expired;
}

bool Notification::dismissed() const
{
    return m_dismissed;
}

void Notification::setDismissed(bool dismissed)
{
    m_dismissed = dismissed;
}

void Notification::processHints(const QVariantMap &hints)
{
    auto end = hints.end();

    m_notifyRcName = hints.value(QStringLiteral("x-kde-appname")).toString();
    if (!m_notifyRcName.isEmpty()) {
        // Check whether the application actually has notifications we can configure
        // FIXME move out into a separate function
        KConfig config(m_notifyRcName + QStringLiteral(".notifyrc"), KConfig::NoGlobals);
        config.addConfigSources(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                QStringLiteral("knotifications5/") + m_notifyRcName + QStringLiteral(".notifyrc")));

        KConfigGroup globalGroup(&config, "Global");

        QString applicationName = globalGroup.readEntry("Name");
        if (applicationName.isEmpty()) {
            applicationName = globalGroup.readEntry("Comment");
        }
        m_applicationName = applicationName;

        const QString iconName = globalGroup.readEntry("IconName");
        m_applicationIconName = iconName;

        const QRegularExpression regexp(QStringLiteral("^Event/([^/]*)$"));

        m_configurableNotifyRc = !config.groupList().filter(regexp).isEmpty();
    }

    m_eventId = hints.value(QStringLiteral("x-kde-eventId")).toString();

    const QString desktopEntry = hints.value(QStringLiteral("desktop-entry")).toString();
    if (!desktopEntry.isEmpty()) {
        KService::Ptr service = KService::serviceByStorageId(desktopEntry);
        if (service) {
            m_applicationName = service->name();
            m_applicationIconName = service->icon();
        }
    }

    bool ok;
    const int urgency = hints.value(QStringLiteral("urgency")).toInt(&ok); // DBus type is actually "byte"
    if (ok) {
        switch (urgency) {
        case 0:
            setUrgency(Notifications::LowUrgency);
            break;
        case 1:
            setUrgency(Notifications::NormalUrgency);
            break;
        case 2:
            setUrgency(Notifications::CriticalUrgency);
            break;
        }
    }

    // Underscored hints was in use in version 1.1 of the spec but has been
    // replaced by dashed hints in version 1.2. We need to support it for
    // users of the 1.2 version of the spec.
    auto it = hints.find(QStringLiteral("image-data"));
    if (it == end) {
        it = hints.find(QStringLiteral("image_data"));
    }
    if (it == end) {
        // This hint was in use in version 1.0 of the spec but has been
        // replaced by "image_data" in version 1.1. We need to support it for
        // users of the 1.0 version of the spec.
        it = hints.find(QStringLiteral("icon_data"));
    }

    if (it != end) {
        m_image = decodeNotificationSpecImageHint(it->value<QDBusArgument>());
    }

    if (m_image.isNull()) {
        it = hints.find(QStringLiteral("image-path"));
        if (it == end) {
            it = hints.find(QStringLiteral("image_path"));
        }

        if (it != end) {
            const QString path = findImageForSpecImagePath(it->toString());
            if (!path.isEmpty()) {
                m_image.load(path);
            }
        }
    }
}

bool Notification::operator==(const Notification &other) const
{
    return other.m_id == m_id;
}
