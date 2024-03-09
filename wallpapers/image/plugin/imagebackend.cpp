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

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QMimeDatabase>
#include <QScreen>
#include <QStandardPaths>

#include <KLocalizedString>

#include "model/imageproxymodel.h"
#include "slidefiltermodel.h"
#include "slidemodel.h"

ImageBackend::ImageBackend(QObject *parent)
    : QObject(parent)
    , m_targetSize(qGuiApp->primaryScreen()->size() * qGuiApp->primaryScreen()->devicePixelRatio())
{
    connect(&m_timer, &QTimer::timeout, this, &ImageBackend::nextSlide);
}

ImageBackend::~ImageBackend()
{
}

void ImageBackend::classBegin()
{
}

void ImageBackend::componentComplete()
{
    m_ready = true;

    // MediaProxy will handle SingleImage case
    if (m_usedInConfig) {
        ensureWallpaperModel();
        ensureSlideshowModel();
    } else {
        startSlideshow();
    }
}

QString ImageBackend::image() const
{
    return m_image.toString();
}

void ImageBackend::setImage(const QString &url)
{
    if (url.isEmpty() || m_image == QUrl::fromUserInput(url)) {
        return;
    }

    m_image = QUrl::fromUserInput(url);
    Q_EMIT imageChanged();
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
    Q_EMIT renderingModeChanged();

    startSlideshow();
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

    startSlideshow();
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

    startSlideshow();
}

QSize ImageBackend::targetSize() const
{
    return m_targetSize.value();
}

void ImageBackend::setTargetSize(const QSize &size)
{
    Q_ASSERT(size.isValid());
    m_targetSize = size;
}

QAbstractItemModel *ImageBackend::wallpaperModel() const
{
    Q_ASSERT(m_mode == SingleImage);
    return m_model;
}

void ImageBackend::ensureWallpaperModel()
{
    if (m_model || m_mode != SingleImage) {
        return;
    }

    m_model = new ImageProxyModel({}, QBindable<QSize>(&m_targetSize), QBindable<bool>(&m_usedInConfig), this);
    m_loading.setBinding(m_model->loading().makeBinding());

    Q_EMIT wallpaperModelChanged();
}

void ImageBackend::ensureSlideshowModel()
{
    if (m_slideshowModel || m_mode != SlideShow) {
        return;
    }

    m_slideshowModel = new SlideModel(QBindable<QSize>(&m_targetSize), QBindable<bool>(&m_usedInConfig), this);
    m_slideshowModel->setUncheckedSlides(m_uncheckedSlides);
    m_loading.setBinding(m_slideshowModel->loading().makeBinding());

    m_slideFilterModel = new SlideFilterModel(QBindable<bool>(&m_usedInConfig), //
                                              QBindable<SortingMode::Mode>(&m_slideshowMode), //
                                              QBindable<bool>(&m_slideshowFoldersFirst), //
                                              this);
    // setSourceModel(...) must be done in backgroundsFound() to generate a complete random order

    connect(this, &ImageBackend::uncheckedSlidesChanged, m_slideFilterModel, &SlideFilterModel::invalidateFilter);
    connect(m_slideshowModel, &SlideModel::dataChanged, this, &ImageBackend::slotSlideModelDataChanged);

    if (m_usedInConfig) {
        // When not used in config, slide paths are set in startSlideshow()
        m_slideshowModel->setSlidePaths(m_slidePaths);
        if (m_slideshowModel->loading().value()) {
            connect(m_slideshowModel, &SlideModel::done, this, &ImageBackend::backgroundsFound);
        } else {
            // In case it loads immediately
            m_slideFilterModel->setSourceModel(m_slideshowModel);
        }
    }

    Q_EMIT slideFilterModelChanged();
}

void ImageBackend::saveCurrentWallpaper()
{
    if (!m_ready || m_usedInConfig || m_mode != RenderingMode::SlideShow || m_configMap.isNull() || !m_image.isValid()) {
        return;
    }

    QMetaObject::invokeMethod(this, "writeImageConfig", Qt::QueuedConnection, Q_ARG(QString, m_image.toString()));
}

QAbstractItemModel *ImageBackend::slideFilterModel() const
{
    Q_ASSERT(m_mode == SlideShow);
    return m_slideFilterModel;
}

int ImageBackend::slideTimer() const
{
    return m_delay;
}

void ImageBackend::setSlideTimer(int time)
{
    if (time == m_delay) {
        return;
    }

    m_delay = time;
    Q_EMIT slideTimerChanged();

    startSlideshow();
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
    } else if (m_slideshowModel) {
        // When used in config, m_slideshowModel can be nullptr when the image wallpaper is being used.
        m_slideshowModel->setSlidePaths(m_slidePaths);
    }
    Q_EMIT slidePathsChanged();
}

bool ImageBackend::addSlidePath(const QUrl &url)
{
    Q_ASSERT(m_mode == SlideShow);
    if (url.isEmpty()) {
        return false;
    }

    QString path = url.toLocalFile();

    // If path is a file, use its parent folder.
    const QFileInfo info(path);

    if (info.isFile()) {
        path = info.dir().absolutePath();
    }

    const QStringList results = m_slideshowModel->addDirs({path});

    if (results.empty()) {
        return false;
    }

    m_slidePaths.append(results);
    Q_EMIT slidePathsChanged();

    return true;
}

void ImageBackend::removeSlidePath(const QString &path)
{
    Q_ASSERT(m_mode == SlideShow);

    /* BUG 461003 check path is in the config*/
    m_slideshowModel->removeDir(path);

    if (m_slidePaths.removeOne(path)) {
        Q_EMIT slidePathsChanged();
    }
}

void ImageBackend::startSlideshow()
{
    if (!m_ready || m_usedInConfig || m_mode != SlideShow || m_pauseSlideshow) {
        return;
    }
    // populate background list
    m_timer.stop();
    ensureSlideshowModel();
    m_slideFilterModel->setSourceModel(nullptr);
    connect(m_slideshowModel, &SlideModel::done, this, &ImageBackend::backgroundsFound);
    m_slideshowModel->setSlidePaths(m_slidePaths);
    // TODO: what would be cool: paint on the wallpaper itself a busy widget and perhaps some text
    // about loading wallpaper slideshow while the thread runs
}

void ImageBackend::backgroundsFound()
{
    disconnect(m_slideshowModel, &SlideModel::done, this, nullptr);

    // setSourceModel must be called after the model is loaded to generate a complete random order
    Q_ASSERT(!m_slideFilterModel->sourceModel());
    m_slideFilterModel->setSourceModel(m_slideshowModel);

    if (m_slideFilterModel->rowCount() == 0 || m_usedInConfig) {
        return;
    }

    // start slideshow
    m_slideFilterModel->sort(0);
    m_currentSlide = m_configMap.isNull() || m_slideshowMode == SortingMode::Random
        ? -1
        : m_slideFilterModel->indexOf(m_configMap->value(QStringLiteral("Image")).toString()) - 1;
    nextSlide();
}

QString ImageBackend::nameFilters() const
{
    QStringList imageGlobPatterns;
    QMimeDatabase db;
    const auto supportedMimeTypes = QImageReader::supportedMimeTypes();
    for (const QByteArray &mimeType : supportedMimeTypes) {
        QMimeType mime(db.mimeTypeForName(QString::fromLatin1(mimeType)));
        imageGlobPatterns << mime.globPatterns();
    }
    // i18n people, this isn't a "word puzzle". there is a specific string format for QFileDialog::setNameFilters
    return i18n("Image Files") + QLatin1String(" (") + imageGlobPatterns.join(QLatin1Char(' ')) + QLatin1Char(')');
}

QQmlPropertyMap *ImageBackend::configMap() const
{
    return m_configMap.data();
}

void ImageBackend::setConfigMap(QQmlPropertyMap *configMap)
{
    if (configMap == m_configMap.data()) {
        return;
    }

    m_configMap = configMap;
    Q_EMIT configMapChanged();

    if (!m_configMap.isNull()) {
        Q_ASSERT(m_configMap->contains(QStringLiteral("Image")));
    }

    saveCurrentWallpaper();
}

QString ImageBackend::addUsersWallpaper(const QUrl &url)
{
    Q_ASSERT(m_mode == SingleImage);
    ensureWallpaperModel(); // The model is not created by default when used in desktop
    auto results = m_model->addBackground(url.isLocalFile() ? url.toLocalFile() : url.toString());

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
    if (m_currentSlide >= rowCount - 1 /* ">" in case the last wallpaper is deleted before */ || m_currentSlide < 0) {
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
        m_image = QUrl::fromLocalFile(previousPath);
    } else {
        m_image = QUrl::fromLocalFile(next);
        Q_EMIT imageChanged();
    }

    saveCurrentWallpaper();
}

void ImageBackend::slotSlideModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
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

bool ImageBackend::pauseSlideshow() const
{
    return m_pauseSlideshow;
}

void ImageBackend::setPauseSlideshow(bool pauseSlideshow)
{
    if (m_pauseSlideshow == pauseSlideshow) {
        return;
    }

    m_pauseSlideshow = pauseSlideshow;
    Q_EMIT pauseSlideshowChanged();

    if (!m_slideFilterModel) {
        return;
    }

    if (pauseSlideshow && m_timer.isActive()) {
        // Pause timer and store the remaining time
        m_remainingTime = m_timer.remainingTimeAsDuration();
        m_timer.stop();
    } else if (!pauseSlideshow && !m_timer.isActive()) {
        if (m_slideFilterModel->rowCount() > 0) {
            // Resume from the last point
            m_timer.start(m_remainingTime.value_or(std::chrono::seconds(m_delay)));
            m_remainingTime.reset();
        } else {
            // Start a new slideshow
            startSlideshow();
        }
    }
}

bool ImageBackend::loading() const
{
    return m_loading;
}
