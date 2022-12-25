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
#include <QScreen>
#include <QStandardPaths>

#include <KLocalizedString>

#include "debug.h"
#include "finder/packagefinder.h"
#include "model/imageproxymodel.h"
#include "slidefiltermodel.h"
#include "slidemodel.h"

ImageBackend::ImageBackend(QObject *parent)
    : QObject(parent)
    , m_targetSize(qGuiApp->primaryScreen()->size() * qGuiApp->primaryScreen()->devicePixelRatio())
    , m_slideFilterModel(new SlideFilterModel(this))
{
    connect(&m_timer, &QTimer::timeout, this, &ImageBackend::nextSlide);
}

ImageBackend::~ImageBackend()
{
    delete m_dialog;
}

void ImageBackend::classBegin()
{
}

void ImageBackend::componentComplete()
{
    m_ready = true;

    // MediaProxy will handle SingleImage case
    if (m_mode == SlideShow) {
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

    if (m_mode == SlideShow) {
        startSlideshow();
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

    // Will relay to ImageProxyModel
    Q_EMIT targetSizeChanged(m_targetSize);
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

void ImageBackend::saveCurrentWallpaper()
{
    if (!m_ready || m_usedInConfig || m_mode != RenderingMode::SlideShow || m_configMap.isNull() || !m_image.isValid()) {
        return;
    }

    QMetaObject::invokeMethod(this, "writeImageConfig", Qt::QueuedConnection, Q_ARG(QString, m_image.toString()));
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

    /* BUG 461003 check path is in the config*/
    m_slideshowModel->removeDir(path);

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
    m_slideFilterModel->sort(0);
    m_currentSlide = m_configMap.isNull() || m_slideshowMode == SortingMode::Random
        ? -1
        : m_slideFilterModel->indexOf(m_configMap->value(QStringLiteral("Image")).toString()) - 1;
    nextSlide();
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

    Q_EMIT wallpaperBrowseCompleted();
    Q_EMIT settingsChanged();
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
    auto results = static_cast<ImageProxyModel *>(wallpaperModel())->addBackground(url.isLocalFile() ? url.toLocalFile() : url.toString());

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
        m_image = QUrl::fromLocalFile(previousPath);
    } else {
        m_image = QUrl::fromLocalFile(next);
        Q_EMIT imageChanged();
    }

    saveCurrentWallpaper();
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

bool ImageBackend::loading() const
{
    if (renderingMode() == SingleImage && m_model) {
        return m_model->loading();
    } else if (renderingMode() == SlideShow && m_slideshowModel) {
        return m_slideshowModel->loading();
    }

    return false;
}
