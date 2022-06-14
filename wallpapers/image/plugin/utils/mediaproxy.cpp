/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mediaproxy.h"

#include <QFileInfo>
#include <QGuiApplication>
#include <QMimeDatabase>
#include <QMovie>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QUrlQuery>

#include <KConfigGroup>
#include <KIO/OpenUrlJob>
#include <KNotificationJobUiDelegate>
#include <KPackage/PackageLoader>

#include <Plasma/Theme>

#include "../finder/packagefinder.h"
#include "../finder/suffixcheck.h"
#include "desktoppool.h"
#include "wideimage.h"

#include "debug.h"

namespace
{
static int s_instanceCount = 0;
static DesktopPool *s_desktopPool = nullptr;
}

MediaProxy::MediaProxy(QObject *parent)
    : QObject(parent)
    , m_targetSize(qGuiApp->primaryScreen()->size() * qGuiApp->primaryScreen()->devicePixelRatio())
    , m_isDarkColorScheme(isDarkColorScheme())
{
    useSingleImageDefaults();
}

MediaProxy::~MediaProxy()
{
    if (m_spanScreens) {
        disableSpanScreen();
    }
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

    enableSpanScreen();

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
        updateModelImage();
    }
}

bool MediaProxy::spanScreens() const
{
    return m_spanScreens;
}

void MediaProxy::setSpanScreens(bool span)
{
    if (m_spanScreens == span) {
        return;
    }

    m_spanScreens = span;
    Q_EMIT spanScreensChanged();

    if (m_ready && m_targetWindow) {
        if (m_spanScreens) {
            enableSpanScreen();
        } else {
            disableSpanScreen();
        }

        updateModelImage();
    }
}

QQuickWindow *MediaProxy::targetWindow() const
{
    return m_targetWindow;
}

void MediaProxy::setTargetWindow(QQuickWindow *window)
{
    if (m_targetWindow && s_desktopPool) {
        s_desktopPool->unsetDesktop(m_targetWindow);
    }

    if (m_ready && m_spanScreens && s_desktopPool && window && !m_formattedSource.isEmpty()) {
        s_desktopPool->setDesktop(window, m_formattedSource);
        updateModelImage();
    }

    if (m_targetWindow == window) {
        return;
    }

    m_targetWindow = window;
    Q_EMIT targetWindowChanged();
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

void MediaProxy::slotDesktopPoolChanged(QQuickWindow *sourceWindow)
{
    // m_targetWindow is only set after the initialization is finished
    if (!m_spanScreens || sourceWindow == m_targetWindow || !m_targetWindow) {
        return;
    }

    qCDebug(IMAGEWALLPAPER) << "new geometry from" << sourceWindow;

    // Wait for other tasks to complete, like unsetDesktop
    QTimer::singleShot(0, this, std::bind(&MediaProxy::updateModelImage, this, false));
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
    } else if (type.startsWith(QLatin1String("video/"))) {
        m_backgroundType = BackgroundType::Type::Video;
    } else {
        m_backgroundType = BackgroundType::Type::Unknown;
    }

    Q_EMIT backgroundTypeChanged();
}

void MediaProxy::determineProviderType()
{
    QFileInfo info(m_formattedSource.toLocalFile());

    if (info.isFile()) {
        m_providerType = Provider::Type::Image; // Including videos
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

        composedUrl.setQuery(urlQuery);
        newRealSource = composedUrl;

        break;
    }

    case Provider::Type::Unknown:
    default:
        return;
    }

    if (m_spanScreens && m_targetWindow) {
        s_desktopPool->setGlobalImage(m_targetWindow, m_formattedSource);
        setProperty("targetRect", WideImage::cropRect(m_targetWindow, s_desktopPool->boundingRect(m_formattedSource)));
        qCDebug(IMAGEWALLPAPER) << "new targetRect for" << m_targetWindow << property("targetRect").toRect();
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

void MediaProxy::enableSpanScreen()
{
    if (!m_spanScreens) {
        return;
    }

    if (!s_desktopPool) {
        s_desktopPool = new DesktopPool;
    }
    s_instanceCount++;

    // After DesktopPool is created, call setTargetWindow again to register the target window.
    setTargetWindow(m_targetWindow);

    // Connect to signals after setDesktop to avoid calling updateModelImage twice
    connect(s_desktopPool, &DesktopPool::desktopWindowChanged, this, &MediaProxy::slotDesktopPoolChanged);
    connect(s_desktopPool, &DesktopPool::geometryChanged, this, std::bind(&MediaProxy::slotDesktopPoolChanged, this, nullptr));
}

void MediaProxy::disableSpanScreen()
{
    // Don't check m_spanScreens as it may be set to false in setSpanScreens
    if (!s_desktopPool) {
        return;
    }

    s_desktopPool->unsetDesktop(m_targetWindow);
    disconnect(s_desktopPool, nullptr, this, nullptr);

    // Will trigger targetRectChanged() and call loadImage in QML
    setProperty("targetRect", QRect());

    if (!--s_instanceCount) {
        delete s_desktopPool;
        s_desktopPool = nullptr;
    }
}
