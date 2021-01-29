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
#include "notification_p.h"

#include <QDBusArgument>
#include <QDebug>
#include <QImageReader>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <KConfig>
#include <KConfigGroup>
#include <KService>
#include <KServiceTypeTrader>

#include "debug.h"

using namespace NotificationManager;

Notification::Private::Private()
{
}

Notification::Private::~Private() = default;

QString Notification::Private::sanitize(const QString &text)
{
    // replace all \ns with <br/>
    QString t = text;

    t.replace(QLatin1String("\n"), QStringLiteral("<br/>"));
    // Now remove all inner whitespace (\ns are already <br/>s)
    t = t.simplified();
    // Finally, check if we don't have multiple <br/>s following,
    // can happen for example when "\n       \n" is sent, this replaces
    // all <br/>s in succession with just one
    t.replace(QRegularExpression(QStringLiteral("<br/>\\s*<br/>(\\s|<br/>)*")), QLatin1String("<br/>"));
    // This fancy RegExp escapes every occurrence of & since QtQuick Text will blatantly cut off
    // text where it finds a stray ampersand.
    // Only &{apos, quot, gt, lt, amp}; as well as &#123 character references will be allowed
    t.replace(QRegularExpression(QStringLiteral("&(?!(?:apos|quot|[gl]t|amp);|#)")), QLatin1String("&amp;"));

    // Don't bother adding some HTML structure if the body is now empty
    if (t.isEmpty()) {
        return t;
    }

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
                    // image denied for security reasons! Do not copy the image src here!
                }

                out.writeAttribute(QStringLiteral("alt"), alt);
            }
            if (name == QLatin1Char('a')) {
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
            out.writeCharacters(text); // this auto escapes chars -> HTML entities
        }
    }
    out.writeEndDocument();

    if (r.hasError()) {
        qCWarning(NOTIFICATIONMANAGER) << "Notification to send to backend contains invalid XML: " << r.errorString() << "line" << r.lineNumber() << "col"
                                       << r.columnNumber();
    }

    // The Text.StyledText format handles only html3.2 stuff and &apos; is html4 stuff
    // so we need to replace it here otherwise it will not render at all.
    result.replace(QLatin1String("&apos;"), QChar('\''));

    return result;
}

QImage Notification::Private::decodeNotificationSpecImageHint(const QDBusArgument &arg)
{
    int width, height, rowStride, hasAlpha, bitsPerSample, channels;
    QByteArray pixels;
    char *ptr;
    char *end;

    arg.beginStructure();
    arg >> width >> height >> rowStride >> hasAlpha >> bitsPerSample >> channels >> pixels;
    arg.endStructure();

#define SANITY_CHECK(condition)                                                                                                                                \
    if (!(condition)) {                                                                                                                                        \
        qCWarning(NOTIFICATIONMANAGER) << "Image decoding sanity check failed on" << #condition;                                                               \
        return QImage();                                                                                                                                       \
    }

    SANITY_CHECK(width > 0);
    SANITY_CHECK(width < 2048);
    SANITY_CHECK(height > 0);
    SANITY_CHECK(height < 2048);
    SANITY_CHECK(rowStride > 0);

#undef SANITY_CHECK

    auto copyLineRGB32 = [](QRgb *dst, const char *src, int width) {
        const char *end = src + width * 3;
        for (; src != end; ++dst, src += 3) {
            *dst = qRgb(src[0], src[1], src[2]);
        }
    };

    auto copyLineARGB32 = [](QRgb *dst, const char *src, int width) {
        const char *end = src + width * 4;
        for (; src != end; ++dst, src += 4) {
            *dst = qRgba(src[0], src[1], src[2], src[3]);
        }
    };

    QImage::Format format = QImage::Format_Invalid;
    void (*fcn)(QRgb *, const char *, int) = nullptr;
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
        qCWarning(NOTIFICATIONMANAGER) << "Unsupported image format (hasAlpha:" << hasAlpha << "bitsPerSample:" << bitsPerSample << "channels:" << channels
                                       << ")";
        return QImage();
    }

    QImage image(width, height, format);
    ptr = pixels.data();
    end = ptr + pixels.length();
    for (int y = 0; y < height; ++y, ptr += rowStride) {
        if (ptr + channels * width > end) {
            qCWarning(NOTIFICATIONMANAGER) << "Image data is incomplete. y:" << y << "height:" << height;
            break;
        }
        fcn((QRgb *)image.scanLine(y), ptr, width);
    }

    return image;
}

void Notification::Private::sanitizeImage(QImage &image)
{
    if (image.isNull()) {
        return;
    }

    const QSize max = maximumImageSize();
    if (image.size().width() > max.width() || image.size().height() > max.height()) {
        image = image.scaled(max, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
}

void Notification::Private::loadImagePath(const QString &path)
{
    // image_path and appIcon should either be a URL with file scheme or the name of a themed icon.
    // We're lenient and also allow local paths.

    image = QImage(); // clear
    icon.clear();

    QUrl imageUrl;
    if (path.startsWith(QLatin1Char('/'))) {
        imageUrl = QUrl::fromLocalFile(path);
    } else if (path.contains(QLatin1Char('/'))) { // bad heuristic to detect a URL
        imageUrl = QUrl(path);

        if (!imageUrl.isLocalFile()) {
            qCDebug(NOTIFICATIONMANAGER) << "Refused to load image from" << path << "which isn't a valid local location.";
            return;
        }
    }

    if (!imageUrl.isValid()) {
        // try icon path instead;
        icon = path;
        return;
    }

    QImageReader reader(imageUrl.toLocalFile());
    reader.setAutoTransform(true);

    const QSize imageSize = reader.size();
    if (imageSize.isValid() && (imageSize.width() > maximumImageSize().width() || imageSize.height() > maximumImageSize().height())) {
        const QSize thumbnailSize = imageSize.scaled(maximumImageSize(), Qt::KeepAspectRatio);
        reader.setScaledSize(thumbnailSize);
    }

    image = reader.read();
}

QString Notification::Private::defaultComponentName()
{
    // NOTE Keep in sync with KNotification
    return QStringLiteral("plasma_workspace");
}

QSize Notification::Private::maximumImageSize()
{
    return QSize(256, 256);
}

KService::Ptr Notification::Private::serviceForDesktopEntry(const QString &desktopEntry)
{
    KService::Ptr service;

    if (desktopEntry.startsWith(QLatin1Char('/'))) {
        service = KService::serviceByDesktopPath(desktopEntry);
    } else {
        service = KService::serviceByDesktopName(desktopEntry);
    }

    if (!service) {
        const QString lowerDesktopEntry = desktopEntry.toLower();
        service = KService::serviceByDesktopName(lowerDesktopEntry);
    }

    // Try if it's a renamed flatpak
    if (!service) {
        const QString desktopId = desktopEntry + QLatin1String(".desktop");
        // HACK Querying for XDG lists in KServiceTypeTrader does not work, do it manually
        const auto services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), QStringLiteral("exist Exec and exist [X-Flatpak-RenamedFrom]"));
        for (auto it = services.constBegin(); it != services.constEnd() && !service; ++it) {
            const QVariant renamedFrom = (*it)->property(QStringLiteral("X-Flatpak-RenamedFrom"), QVariant::String);
            const auto names = renamedFrom.toString().split(QChar(';'));
            for (const QString &name : names) {
                if (name == desktopId) {
                    service = *it;
                    break;
                }
            }
        }
    }

    return service;
}

void Notification::Private::setDesktopEntry(const QString &desktopEntry)
{
    QString serviceName;

    configurableService = false;

    KService::Ptr service = serviceForDesktopEntry(desktopEntry);
    if (service) {
        this->desktopEntry = service->desktopEntryName();
        serviceName = service->name();
        applicationIconName = service->icon();
        configurableService = !service->noDisplay();
    }

    const bool isDefaultEvent = (notifyRcName == defaultComponentName());
    configurableNotifyRc = false;
    if (!notifyRcName.isEmpty()) {
        // Check whether the application actually has notifications we can configure
        KConfig config(notifyRcName + QStringLiteral(".notifyrc"), KConfig::NoGlobals);
        config.addConfigSources(
            QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("knotifications5/") + notifyRcName + QStringLiteral(".notifyrc")));

        KConfigGroup globalGroup(&config, "Global");

        const QString iconName = globalGroup.readEntry("IconName");

        // also only overwrite application icon name for non-default events (or if we don't have a service icon)
        if (!iconName.isEmpty() && (!isDefaultEvent || applicationIconName.isEmpty())) {
            applicationIconName = iconName;
        }

        const QRegularExpression regexp(QStringLiteral("^Event/([^/]*)$"));
        configurableNotifyRc = !config.groupList().filter(regexp).isEmpty();
    }

    // For default events we try to show the application name from the desktop entry if possible
    // This will have us show e.g. "Dr Konqi" instead of generic "Plasma Desktop"
    // The application may not send an applicationName. Use the name from the desktop entry then
    if ((isDefaultEvent || applicationName.isEmpty()) && !serviceName.isEmpty()) {
        applicationName = serviceName;
    }
}

void Notification::Private::processHints(const QVariantMap &hints)
{
    auto end = hints.end();

    notifyRcName = hints.value(QStringLiteral("x-kde-appname")).toString();

    setDesktopEntry(hints.value(QStringLiteral("desktop-entry")).toString());

    // Special override for KDE Connect since the notification is sent by kdeconnectd
    // but actually comes from a different app on the phone
    const QString applicationDisplayName = hints.value(QStringLiteral("x-kde-display-appname")).toString();
    if (!applicationDisplayName.isEmpty()) {
        applicationName = applicationDisplayName;
    }

    originName = hints.value(QStringLiteral("x-kde-origin-name")).toString();

    eventId = hints.value(QStringLiteral("x-kde-eventId")).toString();

    bool ok;
    const int urgency = hints.value(QStringLiteral("urgency")).toInt(&ok); // DBus type is actually "byte"
    if (ok) {
        // FIXME use separate enum again
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

    userActionFeedback = hints.value(QStringLiteral("x-kde-user-action-feedback")).toBool();
    if (userActionFeedback) {
        // A confirmation of an explicit user interaction is assumed to have been seen by the user.
        read = true;
    }

    urls = QUrl::fromStringList(hints.value(QStringLiteral("x-kde-urls")).toStringList());

    replyPlaceholderText = hints.value(QStringLiteral("x-kde-reply-placeholder-text")).toString();
    replySubmitButtonText = hints.value(QStringLiteral("x-kde-reply-submit-button-text")).toString();
    replySubmitButtonIconName = hints.value(QStringLiteral("x-kde-reply-submit-button-icon-name")).toString();

    category = hints.value(QStringLiteral("category")).toString();

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
        image = decodeNotificationSpecImageHint(it->value<QDBusArgument>());
    }

    if (image.isNull()) {
        it = hints.find(QStringLiteral("image-path"));
        if (it == end) {
            it = hints.find(QStringLiteral("image_path"));
        }

        if (it != end) {
            loadImagePath(it->toString());
        }
    }

    sanitizeImage(image);
}

void Notification::Private::setUrgency(Notifications::Urgency urgency)
{
    this->urgency = urgency;

    // Critical notifications must not time out
    // TODO should we really imply this here and not on the view side?
    // are there usecases for critical but can expire?
    // "critical updates available"?
    if (urgency == Notifications::CriticalUrgency) {
        timeout = 0;
    }
}

Notification::Notification(uint id)
    : d(new Private())
{
    d->id = id;
    d->created = QDateTime::currentDateTimeUtc();
}

Notification::Notification(const Notification &other)
    : d(new Private(*other.d))
{
}

Notification::Notification(Notification &&other) noexcept
    : d(other.d)
{
    other.d = nullptr;
}

Notification &Notification::operator=(const Notification &other)
{
    d = new Private(*other.d);
    return *this;
}

Notification &Notification::operator=(Notification &&other) noexcept
{
    d = other.d;
    other.d = nullptr;
    return *this;
}

Notification::~Notification()
{
    delete d;
}

uint Notification::id() const
{
    return d->id;
}

QString Notification::dBusService() const
{
    return d->dBusService;
}

void Notification::setDBusService(const QString &dBusService)
{
    d->dBusService = dBusService;
}

QDateTime Notification::created() const
{
    return d->created;
}

QDateTime Notification::updated() const
{
    return d->updated;
}

void Notification::resetUpdated()
{
    d->updated = QDateTime::currentDateTimeUtc();
}

bool Notification::read() const
{
    return d->read;
}

void Notification::setRead(bool read)
{
    d->read = read;
}

QString Notification::summary() const
{
    return d->summary;
}

void Notification::setSummary(const QString &summary)
{
    d->summary = summary;
}

QString Notification::body() const
{
    return d->body;
}

void Notification::setBody(const QString &body)
{
    d->rawBody = body;
    d->body = Private::sanitize(body.trimmed());
}

QString Notification::rawBody() const
{
    return d->rawBody;
}

QString Notification::icon() const
{
    return d->icon;
}

void Notification::setIcon(const QString &icon)
{
    d->loadImagePath(icon);
    Private::sanitizeImage(d->image);
}

QImage Notification::image() const
{
    return d->image;
}

void Notification::setImage(const QImage &image)
{
    d->image = image;
}

QString Notification::desktopEntry() const
{
    return d->desktopEntry;
}

void Notification::setDesktopEntry(const QString &desktopEntry)
{
    d->setDesktopEntry(desktopEntry);
}

QString Notification::notifyRcName() const
{
    return d->notifyRcName;
}

QString Notification::eventId() const
{
    return d->eventId;
}

QString Notification::applicationName() const
{
    return d->applicationName;
}

void Notification::setApplicationName(const QString &applicationName)
{
    d->applicationName = applicationName;
}

QString Notification::applicationIconName() const
{
    return d->applicationIconName;
}

void Notification::setApplicationIconName(const QString &applicationIconName)
{
    d->applicationIconName = applicationIconName;
}

QString Notification::originName() const
{
    return d->originName;
}

QStringList Notification::actionNames() const
{
    return d->actionNames;
}

QStringList Notification::actionLabels() const
{
    return d->actionLabels;
}

bool Notification::hasDefaultAction() const
{
    return d->hasDefaultAction;
}

QString Notification::defaultActionLabel() const
{
    return d->defaultActionLabel;
}

void Notification::setActions(const QStringList &actions)
{
    if (actions.count() % 2 != 0) {
        qCWarning(NOTIFICATIONMANAGER) << "List of actions must contain an even number of items, tried to set actions to" << actions;
        return;
    }

    d->hasDefaultAction = false;
    d->hasConfigureAction = false;
    d->hasReplyAction = false;

    QStringList names;
    QStringList labels;

    for (int i = 0; i < actions.count(); i += 2) {
        const QString &name = actions.at(i);
        const QString &label = actions.at(i + 1);

        if (!d->hasDefaultAction && name == QLatin1String("default")) {
            d->hasDefaultAction = true;
            d->defaultActionLabel = label;
            continue;
        }

        if (!d->hasConfigureAction && name == QLatin1String("settings")) {
            d->hasConfigureAction = true;
            d->configureActionLabel = label;
            continue;
        }

        if (!d->hasReplyAction && name == QLatin1String("inline-reply")) {
            d->hasReplyAction = true;
            d->replyActionLabel = label;
            continue;
        }

        names << name;
        labels << label;
    }

    d->actionNames = names;
    d->actionLabels = labels;
}

QList<QUrl> Notification::urls() const
{
    return d->urls;
}

void Notification::setUrls(const QList<QUrl> &urls)
{
    d->urls = urls;
}

Notifications::Urgency Notification::urgency() const
{
    return d->urgency;
}

bool Notification::userActionFeedback() const
{
    return d->userActionFeedback;
}

int Notification::timeout() const
{
    return d->timeout;
}

void Notification::setTimeout(int timeout)
{
    d->timeout = timeout;
}

bool Notification::configurable() const
{
    return d->hasConfigureAction || d->configurableNotifyRc || d->configurableService;
}

QString Notification::configureActionLabel() const
{
    return d->configureActionLabel;
}

bool Notification::hasReplyAction() const
{
    return d->hasReplyAction;
}

QString Notification::replyActionLabel() const
{
    return d->replyActionLabel;
}

QString Notification::replyPlaceholderText() const
{
    return d->replyPlaceholderText;
}

QString Notification::replySubmitButtonText() const
{
    return d->replySubmitButtonText;
}

QString Notification::replySubmitButtonIconName() const
{
    return d->replySubmitButtonIconName;
}

QString Notification::category() const
{
    return d->category;
}

bool Notification::expired() const
{
    return d->expired;
}

void Notification::setExpired(bool expired)
{
    d->expired = expired;
}

bool Notification::dismissed() const
{
    return d->dismissed;
}

void Notification::setDismissed(bool dismissed)
{
    d->dismissed = dismissed;
}

QVariantMap Notification::hints() const
{
    return d->hints;
}

void Notification::setHints(const QVariantMap &hints)
{
    d->hints = hints;
}

void Notification::processHints(const QVariantMap &hints)
{
    d->processHints(hints);
}
