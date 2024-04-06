/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mediaproxy.h"

#include "defaultwallpaper.h"

#include <QBuffer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMimeDatabase>
#include <QMovie>
#include <QScreen>
#include <QUrlQuery>

#include <KConfigGroup>
#include <KIO/OpenUrlJob>
#include <KNotificationJobUiDelegate>
#include <KPackage/PackageLoader>

#include <Plasma/Theme>

#include "../finder/packagefinder.h"

#include "debug.h"

namespace
{
static const QString s_wallpaperPackageName = QStringLiteral("Wallpaper/Images");
}

MediaProxy::MediaProxy(QObject *parent)
    : QObject(parent)
    , m_targetSize(qGuiApp->primaryScreen()->size() * qGuiApp->primaryScreen()->devicePixelRatio())
    , m_isDarkColorScheme(isDarkColorScheme())
{
    connect(&m_dirWatch, &KDirWatch::created, this, &MediaProxy::slotSourceFileUpdated);
}

void MediaProxy::classBegin()
{
}

void MediaProxy::componentComplete()
{
    // don't bother loading single image until all properties have settled
    // otherwise we would load a too small image (initial view size) just
    // to load the proper one afterwards etc etc
    m_ready = true;

    // Follow system color scheme
    connect(qGuiApp, &QGuiApplication::paletteChanged, this, &MediaProxy::slotSystemPaletteChanged);

    processSource();
}

QString MediaProxy::source() const
{
    return m_source.toString();
}

void MediaProxy::setSource(const QString &url)
{
    // New desktop has empty url
    if (url.isEmpty()) {
        if (!m_isDefaultSource) {
            useSingleImageDefaults();
            m_isDefaultSource = true;
        }
        return;
    }

    m_isDefaultSource = false;

    const QUrl sanitizedUrl = QUrl::fromUserInput(url);
    if (m_source == sanitizedUrl) {
        return;
    }

    if (!m_source.isEmpty()) {
        m_dirWatch.removeFile(m_source.toLocalFile());
    }
    m_source = sanitizedUrl;
    if (QFileInfo(m_source.toLocalFile()).isFile()) {
        m_dirWatch.addFile(m_source.toLocalFile());
    }
    Q_EMIT sourceChanged();

    m_providerType = Provider::Type::Unknown;
    processSource();
}

QUrl MediaProxy::modelImage() const
{
    return m_modelImage;
}

QSize MediaProxy::targetSize() const
{
    return m_targetSize;
}

void MediaProxy::setTargetSize(const QSize &size)
{
    if (m_targetSize == size) {
        return;
    }

    m_targetSize = size;
    Q_EMIT targetSizeChanged(size);

    if (m_providerType == Provider::Type::Package) {
        // In case file format changes after size changes
        processSource();
    }
    if (m_providerType == Provider::Type::Image || m_backgroundType == BackgroundType::Type::AnimatedImage) {
        // When KPackage contains animated wallpapers, image provider is not used.
        Q_EMIT actualSizeChanged();
    }
}

Provider::Type MediaProxy::providerType() const
{
    return m_providerType;
}

void MediaProxy::openModelImage()
{
    QUrl url;

    switch (m_providerType) {
    case Provider::Type::Image: {
        url = m_modelImage;
        break;
    }

    case Provider::Type::Package: {
        auto package = KPackage::PackageLoader::self()->loadPackage(s_wallpaperPackageName);
        package.setPath(m_source.toLocalFile());

        url = findPreferredImageInPackage(package);
        break;
    }

    default:
        return;
    }

    KIO::OpenUrlJob *job = new KIO::OpenUrlJob(url);
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
    job->start();
}

void MediaProxy::useSingleImageDefaults()
{
    m_source.clear(); // BUG 460692

    auto package = DefaultWallpaper::defaultWallpaperPackage();

    if (!package.isValid()) {
        return;
    }

    m_source = QUrl::fromLocalFile(package.path());

    PackageFinder::findPreferredImageInPackage(package, m_targetSize);

    // Make sure the image can be read, or there will be dead loops.
    if (m_source.isEmpty() || QImage(package.filePath("preferred")).isNull()) {
        return;
    }

    Q_EMIT sourceChanged();

    m_providerType = Provider::Type::Unknown;
    processSource(&package);
}

void MediaProxy::slotSystemPaletteChanged(const QPalette &palette)
{
    if (m_providerType != Provider::Type::Package) {
        // Currently only KPackage supports adaptive wallpapers
        return;
    }

    const bool dark = isDarkColorScheme(palette);

    if (dark == m_isDarkColorScheme) {
        return;
    }

    m_isDarkColorScheme = dark;

    if (m_providerType == Provider::Type::Package) {
        // In case light and dark variants have different formats
        processSource(nullptr, true /* Immediately change the wallpaper */);
    }

    Q_EMIT colorSchemeChanged();
}

void MediaProxy::slotSourceFileUpdated(const QString &path)
{
    Q_ASSERT(path == m_source.toLocalFile());
    if (m_providerType == Provider::Type::Unknown) {
        processSource();
    }
    Q_EMIT sourceFileUpdated();
}

bool MediaProxy::isDarkColorScheme(const QPalette &palette)
{
    // 192 is from kcm_colors
    if (palette == QPalette()) {
        return qGray(qGuiApp->palette().window().color().rgb()) < 192;
    }
    return qGray(palette.window().color().rgb()) < 192;
}

QColor MediaProxy::getAccentColorFromMetaData(const KPackage::Package &package)
{
    const QJsonObject metaData = package.metadata().rawData();
    const auto jsonIt = metaData.constFind(QLatin1String("X-KDE-PlasmaImageWallpaper-AccentColor"));
    if (jsonIt == metaData.constEnd()) {
        return QColor();
    }

    QString colorString = QStringLiteral("transparent");
    const auto accentColorValue = jsonIt.value();
    switch (accentColorValue.type()) {
    case QJsonValue::String: {
        colorString = accentColorValue.toString();
        if (!colorString.isEmpty()) {
            break;
        }
        [[fallthrough]];
    }

    case QJsonValue::Object: {
        const QJsonObject accentColorDict = accentColorValue.toObject();
        if (isDarkColorScheme()) {
            const auto darkIt = accentColorDict.constFind(QLatin1String("Dark"));
            if (darkIt != accentColorDict.constEnd()) {
                colorString = darkIt.value().toString();
                if (!colorString.isEmpty()) {
                    break;
                }
            }
        }

        // Light color as fallback
        const auto lightIt = accentColorDict.constFind(QLatin1String("Light"));
        if (lightIt != accentColorDict.constEnd()) {
            colorString = lightIt.value().toString();
            break;
        }
        [[fallthrough]];
    }

    default:
        qCWarning(IMAGEWALLPAPER, "Invalid value from \"X-KDE-PlasmaImageWallpaper-AccentColor\"");
        break;
    }

    return QColor(colorString);
}

void MediaProxy::determineBackgroundType(KPackage::Package *package)
{
    QString filePath;
    if (package) {
        filePath = findPreferredImageInPackage(*package).toLocalFile();
    } else {
        filePath = m_source.toLocalFile();
    }

    QMimeDatabase db;
    const QString type = db.mimeTypeForFile(filePath).name();

    if (type.startsWith(QLatin1String("video/"))) {
        m_backgroundType = BackgroundType::Type::AnimatedImage;
    } else {
        QBuffer dummyBuffer;
        dummyBuffer.open(QIODevice::ReadOnly);
        // Don't use QMovie::supportedFormats() as it loads all available image plugins
        const bool isAnimated = QImageReader(&dummyBuffer, QFileInfo(filePath).suffix().toLower().toLatin1()).supportsOption(QImageIOHandler::Animation);

        if (isAnimated) {
            m_backgroundType = BackgroundType::Type::AnimatedImage;
        } else if (type.startsWith(QLatin1String("image/"))) {
            m_backgroundType = BackgroundType::Type::Image;
        } else {
            m_backgroundType = BackgroundType::Type::Unknown;
        }
    }

    Q_EMIT backgroundTypeChanged();
}

void MediaProxy::determineProviderType()
{
    QFileInfo info(m_source.toLocalFile());

    auto oldType = m_providerType;

    if (info.isFile()) {
        m_providerType = Provider::Type::Image;
    } else if (info.isDir()) {
        m_providerType = Provider::Type::Package;
    } else {
        m_providerType = Provider::Type::Unknown;
    }

    if (oldType != m_providerType) {
        Q_EMIT providerTypeChanged();
    }
}

void MediaProxy::processSource(KPackage::Package *package, bool doesBlockSignal)
{
    if (!m_ready) {
        return;
    }

    if (m_providerType == Provider::Type::Unknown) {
        determineProviderType();
    }

    if (!package && m_providerType == Provider::Type::Package) {
        KPackage::Package _package = KPackage::PackageLoader::self()->loadPackage(s_wallpaperPackageName);
        _package.setPath(m_source.toLocalFile());
        determineBackgroundType(&_package);
        updateModelImage(&_package, doesBlockSignal);
    } else {
        determineBackgroundType(package);
        updateModelImage(package, doesBlockSignal);
    }
}

QUrl MediaProxy::findPreferredImageInPackage(KPackage::Package &package)
{
    QUrl url;

    if (!package.isValid()) {
        return url;
    }

    PackageFinder::findPreferredImageInPackage(package, m_targetSize);
    url = package.fileUrl("preferred");

    if (isDarkColorScheme()) {
        const QUrl darkUrl = package.fileUrl("preferredDark");

        if (!darkUrl.isEmpty()) {
            url = darkUrl;
        }
    }

    return url;
}

void MediaProxy::updateModelImage(KPackage::Package *package, bool doesBlockSignal)
{
    if (!m_ready) {
        return;
    }

    m_customColor = Qt::transparent;

    QUrl newRealSource;

    switch (m_providerType) {
    case Provider::Type::Image: {
        newRealSource = m_source;
        break;
    }

    case Provider::Type::Package: {
        // Get custom accent color from

        const QColor color(getAccentColorFromMetaData(*package));
        if (m_customColor != color && color.isValid() && color != Qt::transparent) {
            m_customColor = color;
            Q_EMIT customColorChanged();
        }

        if (m_backgroundType == BackgroundType::Type::AnimatedImage) {
            // Is an animated image
            newRealSource = findPreferredImageInPackage(*package);
            break;
        }

        // Use a custom image provider
        QUrl composedUrl(QStringLiteral("image://package/get"));

        QUrlQuery urlQuery(composedUrl);
        urlQuery.addQueryItem(QStringLiteral("dir"), m_source.toLocalFile());
        // To make modelImageChaged work
        urlQuery.addQueryItem(QStringLiteral("targetWidth"), QString::number(m_targetSize.width()));
        urlQuery.addQueryItem(QStringLiteral("targetHeight"), QString::number(m_targetSize.height()));
        urlQuery.addQueryItem(QStringLiteral("darkMode"), QString::number(isDarkColorScheme() ? 1 : 0));

        composedUrl.setQuery(urlQuery);
        newRealSource = composedUrl;
        break;
    }

    case Provider::Type::Unknown:
    default:
        return;
    }

    if (m_modelImage == newRealSource) {
        return;
    }

    m_modelImage = newRealSource;
    if (!doesBlockSignal) {
        Q_EMIT modelImageChanged();
    }
}

void MediaProxy::updateModelImageWithoutSignal(KPackage::Package *package)
{
    updateModelImage(package, true);
}
