/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mediaproxy.h"

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

MediaProxy::MediaProxy(QObject *parent)
    : QObject(parent)
    , m_targetSize(qGuiApp->primaryScreen()->size() * qGuiApp->primaryScreen()->devicePixelRatio())
    , m_isDarkColorScheme(isDarkColorScheme())
{
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

    auto package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(m_source.toLocalFile());

    updateModelImage(package);
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

    m_source = sanitizedUrl;
    Q_EMIT sourceChanged();

    determineProviderType();

    KPackage::Package package;
    if (m_providerType == Provider::Type::Package) {
        package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
        package.setPath(m_source.toLocalFile());
    }

    determineBackgroundType(package);
    updateModelImage(package);
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
        auto package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
        package.setPath(m_source.toLocalFile());

        determineBackgroundType(package); // In case file format changes after size changes
        updateModelImage(package);
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
        auto package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
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
    // Try from the look and feel package first, then from the plasma theme
    KPackage::Package lookAndFeelPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "KDE");
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    // If empty, it will be the default (currently Breeze)
    if (!packageName.isEmpty()) {
        lookAndFeelPackage.setPath(packageName);
    }

    KConfigGroup lnfDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(lookAndFeelPackage.filePath("defaults")), "Wallpaper");

    const QString image = lnfDefaultsConfig.readEntry("Image", "");
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    if (!image.isEmpty()) {
        package.setPath(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/") + image, QStandardPaths::LocateDirectory));

        if (package.isValid()) {
            m_source = QUrl::fromLocalFile(package.path());
        }
    }

    // Try to get a default from the plasma theme
    if (m_source.isEmpty()) {
        Plasma::Theme theme;
        QString path = theme.wallpaperPath();
        int index = path.indexOf(QLatin1String("/contents/images/"));
        if (index > -1) { // We have file from package -> get path to package
            m_source = QUrl::fromLocalFile(path.left(index));
        } else {
            m_source = QUrl::fromLocalFile(path);
        }

        package.setPath(m_source.toLocalFile());
    }

    if (!package.isValid()) {
        // Use Next
        package.setPath(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/Next"), QStandardPaths::LocateDirectory));

        if (package.isValid()) {
            m_source = QUrl::fromLocalFile(package.path());
        } else {
            return;
        }
    }

    PackageFinder::findPreferredImageInPackage(package, m_targetSize);

    // Make sure the image can be read, or there will be dead loops.
    if (m_source.isEmpty() || QImage(package.filePath("preferred")).isNull()) {
        return;
    }

    Q_EMIT sourceChanged();

    determineProviderType();
    determineBackgroundType(package);
    updateModelImage(package);
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
        auto package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
        package.setPath(m_source.toLocalFile());
        updateModelImageWithoutSignal(package);
    }

    Q_EMIT colorSchemeChanged();
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
    const auto jsonIt = metaData.constFind(QStringLiteral("X-KDE-PlasmaImageWallpaper-AccentColor"));
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
            const auto darkIt = accentColorDict.constFind(QStringLiteral("Dark"));
            if (darkIt != accentColorDict.constEnd()) {
                colorString = darkIt.value().toString();
                if (!colorString.isEmpty()) {
                    break;
                }
            }
        }

        // Light color as fallback
        const auto lightIt = accentColorDict.constFind(QStringLiteral("Light"));
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

void MediaProxy::determineBackgroundType(KPackage::Package &package)
{
    QString filePath;
    if (m_providerType == Provider::Type::Package) {
        filePath = findPreferredImageInPackage(package).toLocalFile();
    } else {
        filePath = m_source.toLocalFile();
    }

    QMimeDatabase db;
    const QString type = db.mimeTypeForFile(filePath).name();

    QBuffer dummyBuffer;
    dummyBuffer.open(QIODevice::ReadOnly);
    // Don't use QMovie::supportedFormats() as it loads all available image plugins
    const bool isAnimated = QImageReader(&dummyBuffer, QFileInfo(filePath).suffix().toLower().toLatin1()).supportsOption(QImageIOHandler::Animation);

    if (isAnimated) {
        // Derived from the suffix
        m_backgroundType = BackgroundType::Type::AnimatedImage;
    } else if (type.startsWith(QLatin1String("image/"))) {
        m_backgroundType = BackgroundType::Type::Image;
    } else {
        m_backgroundType = BackgroundType::Type::Unknown;
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

void MediaProxy::updateModelImage(KPackage::Package &package, bool doesBlockSignal)
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

        const QColor color(getAccentColorFromMetaData(package));
        if (m_customColor != color && color.isValid() && color != Qt::transparent) {
            m_customColor = color;
            Q_EMIT customColorChanged();
        }

        if (m_backgroundType == BackgroundType::Type::AnimatedImage) {
            // Is an animated image
            newRealSource = findPreferredImageInPackage(package);
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

void MediaProxy::updateModelImageWithoutSignal(KPackage::Package &package)
{
    updateModelImage(package, true);
}
