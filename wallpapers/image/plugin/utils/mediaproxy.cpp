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

    updateModelImage();
}

QString MediaProxy::source() const
{
    return m_source.toString();
}

void MediaProxy::setSource(const QString &url)
{
    // New desktop has empty url
    if (url.isEmpty()) {
        useSingleImageDefaults();
        return;
    } else if (m_source.toString() == url) {
        return;
    }

    m_source = QUrl(url);
    m_formattedSource = formatUrl(m_source);
    Q_EMIT sourceChanged();

    determineProviderType();
    determineBackgroundType();

    updateModelImage();
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
        determineBackgroundType(); // In case file format changes after size changes
        updateModelImage();
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
        url = findPreferredImageInPackage();
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

        if (!package.isValid()) {
            return;
        }
    }

    PackageFinder::findPreferredImageInPackage(package, m_targetSize);

    // Make sure the image can be read, or there will be dead loops.
    if (m_source.isEmpty() || QImage(package.filePath("preferred")).isNull()) {
        return;
    }

    m_formattedSource = formatUrl(m_source);
    Q_EMIT sourceChanged();

    determineProviderType();
    determineBackgroundType();
    updateModelImage();
}

QUrl MediaProxy::formatUrl(const QUrl &url)
{
    if (url.isLocalFile()) {
        return url;
    }

    // The url can be without file://, try again.
    return QUrl::fromLocalFile(url.toString());
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
        updateModelImageWithoutSignal();
    }

    Q_EMIT colorSchemeChanged();
}

bool MediaProxy::isDarkColorScheme(const QPalette &palette) const noexcept
{
    // 192 is from kcm_colors
    if (palette == QPalette()) {
        return qGray(qGuiApp->palette().window().color().rgb()) < 192;
    }
    return qGray(palette.window().color().rgb()) < 192;
}

void MediaProxy::determineBackgroundType()
{
    QString filePath;
    if (m_providerType == Provider::Type::Package) {
        filePath = findPreferredImageInPackage().toLocalFile();
    } else {
        filePath = m_formattedSource.toLocalFile();
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
    QFileInfo info(m_formattedSource.toLocalFile());

    if (info.isFile()) {
        m_providerType = Provider::Type::Image;
    } else if (info.isDir()) {
        m_providerType = Provider::Type::Package;
    } else {
        m_providerType = Provider::Type::Unknown;
    }
}

QUrl MediaProxy::findPreferredImageInPackage()
{
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(m_formattedSource.toLocalFile());

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

void MediaProxy::updateModelImage(bool doesBlockSignal)
{
    if (!m_ready) {
        return;
    }

    QUrl newRealSource;

    switch (m_providerType) {
    case Provider::Type::Image: {
        newRealSource = m_formattedSource;
        break;
    }

    case Provider::Type::Package: {
        if (m_backgroundType == BackgroundType::Type::AnimatedImage) {
            // Is an animated image
            newRealSource = findPreferredImageInPackage();
            break;
        }

        // Use a custom image provider
        QUrl composedUrl(QStringLiteral("image://package/get"));

        QUrlQuery urlQuery(composedUrl);
        urlQuery.addQueryItem(QStringLiteral("dir"), m_formattedSource.toLocalFile());
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

void MediaProxy::updateModelImageWithoutSignal()
{
    updateModelImage(true);
}
