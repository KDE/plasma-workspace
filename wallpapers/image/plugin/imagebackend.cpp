/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2008 Petri Damsten <damu@iki.fi>
    SPDX-FileCopyrightText: 2008 Alexis Ménard <darktears31@gmail.com>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imagebackend.h"

#include <math.h>

#include <QFileDialog>
#include <QGuiApplication>
#include <QImageReader>
#include <QMimeDatabase>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QUrlQuery>

#include <KConfigGroup>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KPackage/PackageLoader>
#include <Plasma/Theme>

#include "debug.h"
#include "desktoppool.h"
#include "finder/packagefinder.h"
#include "model/imageproxymodel.h"
#include "slidefiltermodel.h"
#include "slidemodel.h"

namespace
{
static int s_instanceCount = 0;
static DesktopPool *s_desktopPool = nullptr;
}

ImageBackend::ImageBackend(QObject *parent)
    : QObject(parent)
    , m_targetSize(qGuiApp->primaryScreen()->size() * qGuiApp->primaryScreen()->devicePixelRatio())
    , m_slideFilterModel(new SlideFilterModel(this))
{
    connect(&m_timer, &QTimer::timeout, this, &ImageBackend::nextSlide);

    if (!s_desktopPool) {
        s_desktopPool = new DesktopPool;
    }
    s_instanceCount++;

    useSingleImageDefaults();
}

ImageBackend::~ImageBackend()
{
    delete m_dialog;

    if (!m_usedInConfig) {
        s_desktopPool->unsetDesktop(m_targetWindow);
    }

    if (!--s_instanceCount) {
        delete s_desktopPool;
        s_desktopPool = nullptr;
    }
}

void ImageBackend::classBegin()
{
}

void ImageBackend::componentComplete()
{
    // don't bother loading single image until all properties have settled
    // otherwise we would load a too small image (initial view size) just
    // to load the proper one afterwards etc etc
    m_ready = true;

    if (auto p = qobject_cast<QQuickItem *>(this->parent()); p && !m_usedInConfig) {
        connect(p, &QQuickItem::windowChanged, this, &ImageBackend::slotParentWindowChanged, Qt::DirectConnection);

        connect(s_desktopPool, &DesktopPool::desktopWindowChanged, this, &ImageBackend::slotScreenPoolChanged, Qt::QueuedConnection);
        connect(s_desktopPool, &DesktopPool::geometryChanged, this, &ImageBackend::slotScreenPoolChanged, Qt::QueuedConnection);

        slotParentWindowChanged(p->window());
    }

    if (m_mode == SingleImage) {
        setSingleImage();
    } else if (m_mode == SlideShow) {
        startSlideshow();
    }
}

QString ImageBackend::image() const
{
    return m_image.toString();
}

void ImageBackend::setImage(const QString &url)
{
    // Force refresh the wallpapaer when "span multiple screens" is on
    if ((!m_spanScreens && (m_image.toString() == url || m_image.toLocalFile() == url)) || url.isEmpty()) {
        return;
    }

    m_image = QUrl(url);
    Q_EMIT imageChanged();

    if (m_mode != SingleImage) {
        return;
    }

    setSingleImage();
}

QUrl ImageBackend::modelImage() const
{
    return m_modelImage;
}

ImageBackend::RenderingMode ImageBackend::renderingMode() const
{
    return m_mode;
}

void ImageBackend::setRenderingMode(RenderingMode mode)
{
    if (mode == m_mode) {
        return;
    }

    m_mode = mode;

    if (m_mode == SlideShow) {
        startSlideshow();
    } else {
        // we need to reset the preferred image
        setSingleImage();
    }
}

SortingMode::Mode ImageBackend::slideshowMode() const
{
    return m_slideshowMode;
}

void ImageBackend::setSlideshowMode(SortingMode::Mode slideshowMode)
{
    if (slideshowMode == m_slideshowMode) {
        return;
    }
    m_slideshowMode = slideshowMode;
    m_slideFilterModel->setSortingMode(m_slideshowMode, m_slideshowFoldersFirst);
    m_slideFilterModel->sort(0);
    if (m_mode == SlideShow) {
        startSlideshow();
    }
    Q_EMIT slideshowModeChanged();
}

bool ImageBackend::slideshowFoldersFirst() const
{
    return m_slideshowFoldersFirst;
}

void ImageBackend::setSlideshowFoldersFirst(bool slideshowFoldersFirst)
{
    if (slideshowFoldersFirst == m_slideshowFoldersFirst) {
        return;
    }
    m_slideshowFoldersFirst = slideshowFoldersFirst;
    m_slideFilterModel->setSortingMode(m_slideshowMode, m_slideshowFoldersFirst);
    m_slideFilterModel->sort(0);
    if (m_mode == SlideShow) {
        startSlideshow();
    }
    Q_EMIT slideshowFoldersFirstChanged();
}

QSize ImageBackend::targetSize() const
{
    return m_targetSize;
}

void ImageBackend::setTargetSize(const QSize &size)
{
    if (m_targetSize == size) {
        return;
    }

    m_targetSize = size;

    if (m_ready && m_providerType == Provider::Package) {
        Q_EMIT modelImageChanged();
    }

    // Will relay to ImageProxyModel
    Q_EMIT targetSizeChanged(m_targetSize);
}

void ImageBackend::useSingleImageDefaults()
{
    m_image.clear();

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
            m_image = QUrl::fromLocalFile(package.path());
        }
    }

    // Try to get a default from the plasma theme
    if (m_image.isEmpty()) {
        Plasma::Theme theme;
        QString path = theme.wallpaperPath();
        int index = path.indexOf(QLatin1String("/contents/images/"));
        if (index > -1) { // We have file from package -> get path to package
            m_image = QUrl::fromLocalFile(path.left(index));
        } else {
            m_image = QUrl::fromLocalFile(path);
        }

        package.setPath(m_image.toLocalFile());

        if (!package.isValid()) {
            return;
        }
    }

    PackageFinder::findPreferredImageInPackage(package, m_targetSize);

    // Make sure the image can be read, or there will be dead loops.
    if (m_image.isEmpty() || QImage(package.filePath("preferred")).isNull()) {
        return;
    }

    Q_EMIT imageChanged();
    setSingleImage();
}

QAbstractItemModel *ImageBackend::wallpaperModel()
{
    if (!m_model) {
        m_model = new ImageProxyModel({}, m_targetSize, this);
        connect(this, &ImageBackend::targetSizeChanged, m_model, &ImageProxyModel::targetSizeChanged);
        connect(m_model, &ImageProxyModel::loadingChanged, this, &ImageBackend::loadingChanged);
    }

    return m_model;
}

SlideModel *ImageBackend::slideshowModel()
{
    if (!m_slideshowModel) {
        m_slideshowModel = new SlideModel(m_targetSize, this);
        m_slideshowModel->setUncheckedSlides(m_uncheckedSlides);
        connect(this, &ImageBackend::uncheckedSlidesChanged, m_slideFilterModel, &SlideFilterModel::invalidateFilter);
        connect(this, &ImageBackend::targetSizeChanged, m_slideshowModel, &SlideModel::targetSizeChanged);
        connect(m_slideshowModel, &SlideModel::dataChanged, this, &ImageBackend::slotSlideModelDataChanged);
        connect(m_slideshowModel, &SlideModel::loadingChanged, this, &ImageBackend::loadingChanged);
    }
    return m_slideshowModel;
}

QAbstractItemModel *ImageBackend::slideFilterModel()
{
    if (!m_slideFilterModel->sourceModel()) {
        // make sure it's created
        connect(slideshowModel(), &SlideModel::done, this, &ImageBackend::backgroundsFound);
    }

    return m_slideFilterModel;
}

int ImageBackend::slideTimer() const
{
    return m_delay;
}

void ImageBackend::setSlideTimer(int time)
{
    if (time == m_delay || m_mode != SlideShow) {
        return;
    }

    m_delay = time;

    startSlideshow();

    Q_EMIT slideTimerChanged();
}

QStringList ImageBackend::slidePaths() const
{
    return m_slidePaths;
}

void ImageBackend::setSlidePaths(const QStringList &slidePaths)
{
    if (slidePaths == m_slidePaths) {
        return;
    }

    m_slidePaths = slidePaths;
    m_slidePaths.removeAll(QString());

    if (!m_slidePaths.isEmpty()) {
        // Replace 'preferred://wallpaperlocations' with real paths
        const auto it = std::remove_if(m_slidePaths.begin(), m_slidePaths.end(), [](const QString &path) {
            return path == QLatin1String("preferred://wallpaperlocations");
        });

        if (it != m_slidePaths.end()) {
            m_slidePaths.erase(it, m_slidePaths.end());
            m_slidePaths << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/"), QStandardPaths::LocateDirectory);
        }
    }
    if (!m_usedInConfig) {
        startSlideshow();
    } else {
        slideshowModel()->setSlidePaths(m_slidePaths);
    }
    Q_EMIT slidePathsChanged();
}

void ImageBackend::showAddSlidePathsDialog()
{
    QFileDialog *dialog = new QFileDialog(nullptr, i18n("Directory with the wallpaper to show slides from"), QString());
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setOptions(QFileDialog::ShowDirsOnly);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    connect(dialog, &QDialog::accepted, this, &ImageBackend::addDirFromSelectionDialog);
    dialog->show();
}

void ImageBackend::addSlidePath(const QUrl &url)
{
    if (url.isEmpty()) {
        return;
    }

    QString path = url.toLocalFile();

    // If path is a file, use its parent folder.
    const QFileInfo info(path);

    if (info.isFile()) {
        path = info.dir().absolutePath();
    }

    const QStringList results = m_slideshowModel->addDirs({path});

    if (results.empty()) {
        return;
    }

    m_slidePaths.append(results);
    Q_EMIT slidePathsChanged();
}

void ImageBackend::removeSlidePath(const QString &path)
{
    if (m_mode != SlideShow) {
        return;
    }

    const QString result = m_slideshowModel->removeDir(path);

    if (result.isEmpty()) {
        return;
    }

    if (m_slidePaths.removeOne(path)) {
        Q_EMIT slidePathsChanged();
    }
}

void ImageBackend::addDirFromSelectionDialog()
{
    QFileDialog *dialog = qobject_cast<QFileDialog *>(sender());
    if (dialog) {
        addSlidePath(dialog->directoryUrl());
    }
}

void ImageBackend::setSingleImage()
{
    if (!m_ready || m_image.isEmpty()) {
        return;
    }

    // supposedly QSize::isEmpty() is true if "either width or height are >= 0"
    if (!m_targetSize.width() || !m_targetSize.height()) {
        return;
    }

    if (m_image.isLocalFile()) {
        const QFileInfo info(m_image.toLocalFile());

        if (!info.exists()) {
            return;
        }

        if (info.isFile()) {
            m_providerType = Provider::Image;
        } else {
            m_providerType = Provider::Package;
        }
    } else {
        // The url can be without file://, try again.
        const QFileInfo info(m_image.toString());

        if (!info.exists()) {
            return;
        }

        if (info.isFile()) {
            m_providerType = Provider::Image;
        } else {
            m_providerType = Provider::Package;
        }

        m_image = QUrl::fromLocalFile(info.filePath());
    }

    switch (m_providerType) {
    case Provider::Image: {
        if (m_mode == SingleImage && m_spanScreens && m_targetWindow) {
            s_desktopPool->setGlobalImage(m_targetWindow, m_image);

            QUrl url(QStringLiteral("image://wideimage/get"));

            QUrlQuery urlQuery(url);
            urlQuery.addQueryItem(QStringLiteral("path"), m_image.toLocalFile());
            urlQuery.addQueryItem(QStringLiteral("desktopX"), QString::number(m_targetWindow->x()));
            urlQuery.addQueryItem(QStringLiteral("desktopY"), QString::number(m_targetWindow->y()));
            urlQuery.addQueryItem(QStringLiteral("desktopWidth"), QString::number(m_targetWindow->width()));
            urlQuery.addQueryItem(QStringLiteral("desktopHeight"), QString::number(m_targetWindow->height()));

            const QRect totalRect = s_desktopPool->totalRect(m_image);
            urlQuery.addQueryItem(QStringLiteral("totalRectX"), QString::number(totalRect.x()));
            urlQuery.addQueryItem(QStringLiteral("totalRectY"), QString::number(totalRect.y()));
            urlQuery.addQueryItem(QStringLiteral("totalRectWidth"), QString::number(totalRect.width()));
            urlQuery.addQueryItem(QStringLiteral("totalRectHeight"), QString::number(totalRect.height()));

            url.setQuery(urlQuery);
            m_modelImage = url;
            qCDebug(IMAGEWALLPAPER) << "m_modelImage on desktop" << m_targetWindow << "is set to" << url;
        } else {
            m_modelImage = m_image;
        }

        break;
    }

    case Provider::Package: {
        // Use a custom image provider
        QUrl url(QStringLiteral("image://package/get"));

        QUrlQuery urlQuery(url);
        urlQuery.addQueryItem(QStringLiteral("dir"), m_image.toLocalFile());

        url.setQuery(urlQuery);
        m_modelImage = url;
        break;
    }
    }

    if (!m_modelImage.isEmpty()) {
        Q_EMIT modelImageChanged();
    }
}

void ImageBackend::startSlideshow()
{
    if (!m_ready || m_usedInConfig || m_mode != SlideShow) {
        return;
    }
    // populate background list
    m_timer.stop();
    slideshowModel()->setSlidePaths(m_slidePaths);
    connect(m_slideshowModel, &SlideModel::done, this, &ImageBackend::backgroundsFound);
    // TODO: what would be cool: paint on the wallpaper itself a busy widget and perhaps some text
    // about loading wallpaper slideshow while the thread runs
}

void ImageBackend::backgroundsFound()
{
    disconnect(m_slideshowModel, &SlideModel::done, this, nullptr);

    // setSourceModel must be called after the model is loaded
    m_slideFilterModel->setSourceModel(m_slideshowModel);
    m_slideFilterModel->invalidate();

    if (m_slideFilterModel->rowCount() == 0 || m_usedInConfig) {
        return;
    }

    // start slideshow
    if (m_currentSlide == -1) {
        m_currentSlide = m_slideFilterModel->indexOf(m_image.toString()) - 1;
    } else {
        m_currentSlide = -1;
    }
    m_slideFilterModel->sort(0);
    nextSlide();
}

void ImageBackend::slotParentWindowChanged(QQuickWindow *window)
{
    if (m_targetWindow) {
        disconnect(m_targetWindow, nullptr, this, nullptr);
        s_desktopPool->unsetDesktop(m_targetWindow);
    }

    m_targetWindow = window;

    if (!m_targetWindow) {
        return;
    }

    if (!m_image.isEmpty()) {
        s_desktopPool->setDesktop(m_targetWindow, m_image);
    }
}

void ImageBackend::slotScreenPoolChanged()
{
    if (!m_spanScreens) {
        return;
    }

    // Wait for other tasks to complete, like unsetDesktop
    QTimer::singleShot(0, this, &ImageBackend::setSingleImage);
}

void ImageBackend::showFileDialog()
{
    if (!m_dialog) {
        QString path;
        const QStringList &locations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);

        if (!locations.isEmpty()) {
            path = locations.at(0);
        } else {
            // HomeLocation is guaranteed not to be empty.
            path = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
        }

        QMimeDatabase db;
        QStringList imageGlobPatterns;
        const auto supportedMimeTypes = QImageReader::supportedMimeTypes();

        for (const QByteArray &mimeType : supportedMimeTypes) {
            QMimeType mime(db.mimeTypeForName(QString::fromLatin1(mimeType)));
            imageGlobPatterns << mime.globPatterns();
        }

        m_dialog = new QFileDialog(nullptr, i18n("Open Image"), path, i18n("Image Files") + " (" + imageGlobPatterns.join(' ') + ')');
        // i18n people, this isn't a "word puzzle". there is a specific string format for QFileDialog::setNameFilters

        m_dialog->setFileMode(QFileDialog::ExistingFiles);
        connect(m_dialog, &QDialog::accepted, this, &ImageBackend::slotWallpaperBrowseCompleted);
    }

    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
}

void ImageBackend::slotWallpaperBrowseCompleted()
{
    if (!m_model || !m_dialog) {
        return;
    }

    const QStringList selectedFiles = m_dialog->selectedFiles();

    if (selectedFiles.empty()) {
        return;
    }

    for (const QString &p : selectedFiles) {
        m_model->addBackground(p);
    }

    Q_EMIT settingsChanged();
}

QString ImageBackend::addUsersWallpaper(const QUrl &url)
{
    auto results = static_cast<ImageProxyModel *>(wallpaperModel())->addBackground(url.toLocalFile());

    if (!m_usedInConfig) {
        m_model->commitAddition();
        m_model->deleteLater();
        m_model = nullptr;
    }

    if (results.empty()) {
        return QString();
    }

    Q_EMIT settingsChanged();

    return results.at(0);
}

void ImageBackend::nextSlide()
{
    const int rowCount = m_slideFilterModel->rowCount();

    if (!m_ready || m_usedInConfig || rowCount == 0) {
        return;
    }
    int previousSlide = m_currentSlide;
    QString previousPath;
    if (previousSlide >= 0) {
        previousPath = m_slideFilterModel->index(m_currentSlide, 0).data(ImageRoles::PackageNameRole).toString();
    }
    if (m_currentSlide == rowCount - 1 || m_currentSlide < 0) {
        m_currentSlide = 0;
    } else {
        m_currentSlide += 1;
    }
    // We are starting again - avoid having the same random order when we restart the slideshow
    if (m_slideshowMode == SortingMode::Random && m_currentSlide == 0) {
        m_slideFilterModel->invalidate();
    }
    QString next = m_slideFilterModel->index(m_currentSlide, 0).data(ImageRoles::PackageNameRole).toString();
    // And  avoid showing the same picture twice
    if (previousSlide == rowCount - 1 && previousPath == next && rowCount > 1) {
        m_currentSlide += 1;
        next = m_slideFilterModel->index(m_currentSlide, 0).data(ImageRoles::PackageNameRole).toString();
    }
    m_timer.stop();
    m_timer.start(m_delay * 1000);
    if (next.isEmpty()) {
        m_image = QUrl(previousPath); // setSingleImage will add "file://"
    } else {
        m_image = QUrl(next);
        Q_EMIT imageChanged();
        setSingleImage();
    }
}

void ImageBackend::slotSlideModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight);

    if (!topLeft.isValid()) {
        return;
    }

    if (roles.contains(ImageRoles::ToggleRole)) {
        if (topLeft.data(ImageRoles::ToggleRole).toBool()) {
            m_uncheckedSlides.removeOne(topLeft.data(ImageRoles::PackageNameRole).toString());
        } else {
            m_uncheckedSlides.append(topLeft.data(ImageRoles::PackageNameRole).toString());
        }

        Q_EMIT uncheckedSlidesChanged();
    }
}

void ImageBackend::openFolder(const QString &path)
{
    // TODO: Move to SlideFilterModel
    auto *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(path));
    auto *delegate = new KNotificationJobUiDelegate;
    delegate->setAutoErrorHandlingEnabled(true);
    job->setUiDelegate(delegate);
    job->start();
}

void ImageBackend::openModelImage() const
{
    QUrl url;

    switch (m_providerType) {
    case Provider::Image: {
        url = m_image;
        break;
    }

    case Provider::Package: {
        KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
        package.setPath(m_image.toLocalFile());

        if (!package.isValid()) {
            return;
        }

        PackageFinder::findPreferredImageInPackage(package, m_targetSize);
        url = QUrl::fromLocalFile(package.filePath("preferred"));
        break;
    }
    }

    KIO::OpenUrlJob *job = new KIO::OpenUrlJob(url);
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
    job->start();
}

QStringList ImageBackend::uncheckedSlides() const
{
    return m_uncheckedSlides;
}

void ImageBackend::setUncheckedSlides(const QStringList &uncheckedSlides)
{
    if (uncheckedSlides == m_uncheckedSlides) {
        return;
    }
    m_uncheckedSlides = uncheckedSlides;

    if (m_slideshowModel) {
        m_slideshowModel->setUncheckedSlides(m_uncheckedSlides);
    }

    Q_EMIT uncheckedSlidesChanged();
    startSlideshow();
}

bool ImageBackend::spanScreens() const
{
    return m_spanScreens;
}

void ImageBackend::setSpanScreens(bool span)
{
    if (m_spanScreens == span || m_mode != SingleImage) {
        return;
    }

    m_spanScreens = span;
    Q_EMIT spanScreensChanged();

    if (m_ready && m_targetWindow) {
        if (m_spanScreens) {
            s_desktopPool->setDesktop(m_targetWindow, m_image);
            // desktopWindowChanged will call setSingleImage()
        } else {
            s_desktopPool->unsetDesktop(m_targetWindow);
            // The signal is disconnected, need to manually call setSingleImage()
            setSingleImage();
        }
    }
}

bool ImageBackend::loading() const
{
    if (renderingMode() == SingleImage && m_model) {
        return m_model->loading();
    } else if (renderingMode() == SlideShow && m_slideshowModel) {
        return m_slideshowModel->loading();
    }

    return false;
}
