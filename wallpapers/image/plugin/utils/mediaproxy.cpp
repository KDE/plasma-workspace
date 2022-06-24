/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mediaproxy.h"

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

#include "debug.h"
#include "dynamicwallpaperupdatetimer.h"

MediaProxy::MediaProxy(QObject *parent)
    : QObject(parent)
    , m_targetSize(qGuiApp->primaryScreen()->size() * qGuiApp->primaryScreen()->devicePixelRatio())
    , m_isDarkColorScheme(isDarkColorScheme())
{
    useSingleImageDefaults();
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
    if (url.isEmpty() || m_source.toString() == url) {
        return;
    }

    m_source = QUrl(url);
    m_formattedSource = formatUrl(m_source);
    Q_EMIT sourceChanged();

    updateWallpaper();
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
        updateModelImage();
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

    const KPackage::ImagePackage imagePackage(package, m_targetSize);

    // Make sure the image can be read, or there will be dead loops.
    if (m_source.isEmpty() || QImage(imagePackage.preferred().toLocalFile()).isNull()) {
        return;
    }

    m_formattedSource = formatUrl(m_source);
    Q_EMIT sourceChanged();

    updateWallpaper();
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

void MediaProxy::slotUpdateDynamicWallpaper()
{
    updateModelImage();
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

    if (QMovie::supportedFormats().contains(QFileInfo(filePath).suffix().toLower().toLatin1())) {
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

void MediaProxy::updateDynamicWallpaper()
{
    if (m_providerType != Provider::Type::Package) {
        delete m_dynamicTimer;
        m_dynamicTimer = nullptr;
        m_imagePackage.reset(nullptr);
        return;
    }

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
    package.setPath(m_formattedSource.toLocalFile());
    m_imagePackage.reset(new KPackage::ImagePackage(package, m_targetSize));

    m_dynamicType = m_imagePackage->dynamicType();
    if (m_dynamicType == DynamicType::None) {
        delete m_dynamicTimer;
        m_dynamicTimer = nullptr;
        return;
    }

    if (!m_dynamicTimer) {
        m_dynamicTimer = new DynamicWallpaperUpdateTimer(m_imagePackage.get(), this);
        connect(m_dynamicTimer, &QTimer::timeout, this, &MediaProxy::slotUpdateDynamicWallpaper);
        connect(m_dynamicTimer, &DynamicWallpaperUpdateTimer::clockSkewed, this, &MediaProxy::slotUpdateDynamicWallpaper);
    }

    m_dynamicTimer->setActive(true);
}

void MediaProxy::updateWallpaper()
{
    determineProviderType();
    updateDynamicWallpaper();
    determineBackgroundType();
    updateModelImage();
}

QUrl MediaProxy::findPreferredImageInPackage()
{
    QUrl url;

    if (!m_imagePackage || !m_imagePackage->isValid()) {
        return url;
    }

    switch (m_dynamicType) {
    case DynamicType::None: {
        if (isDarkColorScheme()) {
            const QUrl darkUrl = m_imagePackage->preferredDark();

            if (!darkUrl.isEmpty()) {
                url = darkUrl;
            } else {
                url = m_imagePackage->preferred();
            }
        } else {
            url = m_imagePackage->preferred();
        }
        break;
    }

    case DynamicType::Solar: {
        // TODO: not implemented yet
        if (m_imagePackage->dynamicMetadataSize() > 0) {
            const auto &item = m_imagePackage->dynamicMetadataAtIndex(0);
            url = QUrl::fromLocalFile(item.filename);
        }
        break;
    }

    case DynamicType::Timed: {
        const int index = m_imagePackage->indexAndIntervalAtDateTime(QDateTime::currentDateTime()).first;
        if (index < 0) {
            break;
        }

        const auto &item = m_imagePackage->dynamicMetadataAtIndex(index);
        url = QUrl::fromLocalFile(item.filename);

        break;
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
        if (m_backgroundType == BackgroundType::Type::AnimatedImage && m_dynamicType == DynamicType::None) {
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
        // To make dynamic wallpaper work
        if (m_dynamicType != DynamicType::None) {
            urlQuery.addQueryItem(QStringLiteral("dynamicCurrentIndex"),
                                  QString::number(m_imagePackage->indexAndIntervalAtDateTime(QDateTime::currentDateTime()).first));
            urlQuery.addQueryItem(QStringLiteral("dynamicCurrentTime"), QTime::currentTime().toString(QStringLiteral("hms")));
        }

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
