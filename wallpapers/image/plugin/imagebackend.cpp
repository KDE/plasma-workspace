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
#include "debug.h"

#include <math.h>

#include <QAction>
#include <QApplication>
#include <QEasingCurve>
#include <QFile>
#include <QImageReader>
#include <QMimeDatabase>
#include <QPainter>
#include <QPropertyAnimation>
#include <QQuickItem>
#include <QQuickWindow>

#include <KConfigGroup>
#include <KDirWatch>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KIO/OpenUrlJob>
#include <KNotificationJobUiDelegate>
#include <KRandom>
#include <QDebug>
#include <QFileDialog>
#include <klocalizedstring.h>

#include "backgroundlistmodel.h"
#include "slidefiltermodel.h"
#include "slidemodel.h"
#include <Plasma/PluginLoader>
#include <Plasma/Theme>
#include <qstandardpaths.h>

#include <KPackage/PackageLoader>

ImageBackend::ImageBackend(QObject *parent)
    : QObject(parent)
    , m_ready(false)
    , m_delay(10)
    , m_dirWatch(new KDirWatch(this))
    , m_mode(SingleImage)
    , m_slideshowMode(SortingMode::Random)
    , m_slideshowFoldersFirst(false)
    , m_currentSlide(-1)
    , m_model(nullptr)
    , m_slideFilterModel(new SlideFilterModel(this))
    , m_dialog(nullptr)
{
    m_wallpaperPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    connect(&m_timer, &QTimer::timeout, this, &ImageBackend::nextSlide);

    connect(m_dirWatch, &KDirWatch::created, this, &ImageBackend::pathCreated);
    connect(m_dirWatch, &KDirWatch::dirty, this, &ImageBackend::pathDirty);
    connect(m_dirWatch, &KDirWatch::deleted, this, &ImageBackend::pathDeleted);
    m_dirWatch->startScan();

    useSingleImageDefaults();
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
    // don't bother loading single image until all properties have settled
    // otherwise we would load a too small image (initial view size) just
    // to load the proper one afterwards etc etc
    m_ready = true;
    if (m_mode == SingleImage) {
        setSingleImage();
    } else if (m_mode == SlideShow) {
        // show the last image shown the last time
        m_wallpaperPath = m_wallpaper;
        Q_EMIT wallpaperPathChanged();
        startSlideshow();
    }
}

QString ImageBackend::photosPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

QUrl ImageBackend::wallpaperPath() const
{
    return QUrl::fromLocalFile(m_wallpaperPath);
}

void ImageBackend::addUrl(const QString &url)
{
    addUrl(QUrl(url), true);
}

void ImageBackend::addUrls(const QStringList &urls)
{
    bool first = true;
    for (const QString &url : urls) {
        // set the first drop as the current paper, just add the rest to the roll
        addUrl(QUrl(url), first);
        first = false;
    }
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

        updateDirWatch(m_slidePaths);
        updateDirWatch(m_slidePaths);
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

float distance(const QSize &size, const QSize &desired)
{
    // compute difference of areas
    float desiredAspectRatio = (desired.height() > 0) ? desired.width() / (float)desired.height() : 0;
    float candidateAspectRatio = (size.height() > 0) ? size.width() / (float)size.height() : std::numeric_limits<float>::max();

    float delta = size.width() - desired.width();
    delta = (delta >= 0.0 ? delta : -delta * 2); // Penalize for scaling up

    return qAbs(candidateAspectRatio - desiredAspectRatio) * 25000 + delta;
}

QSize resSize(const QString &str)
{
    int index = str.indexOf('x');
    if (index != -1) {
        return QSize(QStringView(str).left(index).toInt(), QStringView(str).mid(index + 1).toInt());
    }

    return QSize();
}

QString ImageBackend::findPreferedImage(const QStringList &images)
{
    if (images.empty()) {
        return QString();
    }

    // float targetAspectRatio = (m_targetSize.height() > 0 ) ? m_targetSize.width() / (float)m_targetSize.height() : 0;
    // qCDebug(IMAGEWALLPAPER) << "wanted" << m_targetSize << "options" << images << "aspect ratio" << targetAspectRatio;
    float best = std::numeric_limits<float>::max();

    QString bestImage;
    for (const QString &entry : images) {
        QSize candidate = resSize(QFileInfo(entry).baseName());
        if (candidate == QSize()) {
            continue;
        }

        float dist = distance(candidate, m_targetSize);
        // qCDebug(IMAGEWALLPAPER) << "candidate" << candidate << "distance" << dist << "aspect ratio" << candidateAspectRatio;

        if (bestImage.isEmpty() || dist < best) {
            bestImage = entry;
            best = dist;
            // qCDebug(IMAGEWALLPAPER) << "best" << bestImage;
        }
    }

    // qCDebug(IMAGEWALLPAPER) << "best image" << bestImage;
    return bestImage;
}

void ImageBackend::findPreferedImageInPackage(KPackage::Package &package)
{
    if (!package.isValid() || !package.filePath("preferred").isEmpty()) {
        return;
    }

    QString preferred = findPreferedImage(package.entryList("images"));

    package.removeDefinition("preferred");
    package.addFileDefinition("preferred", QStringLiteral("images/") + preferred, i18n("Recommended wallpaper file"));
}

QSize ImageBackend::targetSize() const
{
    return m_targetSize;
}

void ImageBackend::setTargetSize(const QSize &size)
{
    bool sizeChanged = m_targetSize != size;
    m_targetSize = size;

    if (m_mode == SingleImage) {
        if (sizeChanged) {
            // If screen size was changed, we may want to select a new preferred image
            // which has correct aspect ratio for the new screen size.
            m_wallpaperPackage.removeDefinition("preferred");
        }
        setSingleImage();
    }

    if (sizeChanged) {
        Q_EMIT targetSizeChanged();
    }
}

KPackage::Package *ImageBackend::package()
{
    return &m_wallpaperPackage;
}

void ImageBackend::useSingleImageDefaults()
{
    m_wallpaper = QString();

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
    if (!image.isEmpty()) {
        KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
        package.setPath(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/") + image, QStandardPaths::LocateDirectory));

        if (package.isValid()) {
            m_wallpaper = package.path();
        } else {
            m_wallpaper = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/") + image);
        }
    }

    // Try to get a default from the plasma theme
    if (m_wallpaper.isEmpty()) {
        Plasma::Theme theme;
        m_wallpaper = theme.wallpaperPath();
        int index = m_wallpaper.indexOf(QLatin1String("/contents/images/"));
        if (index > -1) { // We have file from package -> get path to package
            m_wallpaper = m_wallpaper.left(index);
        }
    }
}

QAbstractItemModel *ImageBackend::wallpaperModel()
{
    if (!m_model) {
        KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Wallpapers"));
        m_usersWallpapers = cfg.readEntry("usersWallpapers", m_usersWallpapers);

        m_model = new BackgroundListModel(this, this);
        m_model->reload(m_usersWallpapers);
    }

    return m_model;
}

SlideModel *ImageBackend::slideshowModel()
{
    if (!m_slideshowModel) {
        m_slideshowModel = new SlideModel(this, this);
        m_slideshowModel->reload(m_slidePaths);
        m_slideFilterModel->setSourceModel(m_slideshowModel);
        connect(this, &ImageBackend::uncheckedSlidesChanged, m_slideFilterModel, &SlideFilterModel::invalidateFilter);
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
    if (time == m_delay) {
        return;
    }

    m_delay = time;

    if (m_mode == SlideShow) {
        updateDirWatch(m_slidePaths);
        startSlideshow();
    }

    Q_EMIT slideTimerChanged();
}

QStringList ImageBackend::usersWallpapers() const
{
    return m_usersWallpapers;
}

void ImageBackend::setUsersWallpapers(const QStringList &usersWallpapers)
{
    if (usersWallpapers == m_usersWallpapers) {
        return;
    }

    m_usersWallpapers = usersWallpapers;

    Q_EMIT usersWallpapersChanged();
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
            m_slidePaths << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers"), QStandardPaths::LocateDirectory);
        }
    }

    if (m_mode == SlideShow) {
        updateDirWatch(m_slidePaths);
        startSlideshow();
    }
    if (m_slideshowModel) {
        m_slideshowModel->reload(m_slidePaths);
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

void ImageBackend::addSlidePath(const QString &path)
{
    if (!path.isEmpty() && !m_slidePaths.contains(path)) {
        m_slidePaths.append(path);
        if (m_mode == SlideShow) {
            updateDirWatch(m_slidePaths);
        }
        if (m_slideshowModel) {
            m_slideshowModel->addDirs({m_slidePaths});
        }
        Q_EMIT slidePathsChanged();
        startSlideshow();
    }
}

void ImageBackend::removeSlidePath(const QString &path)
{
    if (m_slidePaths.contains(path)) {
        m_slidePaths.removeAll(path);
        if (m_mode == SlideShow) {
            updateDirWatch(m_slidePaths);
        }
        if (m_slideshowModel) {
            bool haveParent = false;
            QStringList children;
            for (const QString &slidePath : qAsConst(m_slidePaths)) {
                if (path.startsWith(slidePath)) {
                    haveParent = true;
                }
                if (slidePath.startsWith(path)) {
                    children.append(slidePath);
                }
            }
            /*If we have the parent directory do nothing since the directories are recursively searched.
             * If we have child directories just reload since removing the parent and then readding the children would
             * induce a race.*/
            if (!haveParent) {
                if (children.size() > 0) {
                    m_slideshowModel->reload(m_slidePaths);
                } else {
                    m_slideshowModel->removeDir(path);
                }
            }
        }

        Q_EMIT slidePathsChanged();
        startSlideshow();
    }
}

void ImageBackend::pathDirty(const QString &path)
{
    updateDirWatch(QStringList(path));
}

void ImageBackend::updateDirWatch(const QStringList &newDirs)
{
    Q_FOREACH (const QString &oldDir, m_dirs) {
        if (!newDirs.contains(oldDir)) {
            m_dirWatch->removeDir(oldDir);
        }
    }

    Q_FOREACH (const QString &newDir, newDirs) {
        if (!m_dirWatch->contains(newDir)) {
            m_dirWatch->addDir(newDir, KDirWatch::WatchSubDirs | KDirWatch::WatchFiles);
        }
    }

    m_dirs = newDirs;
}

void ImageBackend::addDirFromSelectionDialog()
{
    QFileDialog *dialog = qobject_cast<QFileDialog *>(sender());
    if (dialog) {
        addSlidePath(dialog->directoryUrl().toLocalFile());
    }
}

void ImageBackend::syncWallpaperPackage()
{
    m_wallpaperPackage.setPath(m_wallpaper);
    findPreferedImageInPackage(m_wallpaperPackage);
    m_wallpaperPath = m_wallpaperPackage.filePath("preferred");
}

void ImageBackend::setSingleImage()
{
    if (!m_ready) {
        return;
    }

    // supposedly QSize::isEmpty() is true if "either width or height are >= 0"
    if (!m_targetSize.width() || !m_targetSize.height()) {
        return;
    }

    const QString oldPath = m_wallpaperPath;
    if (m_wallpaper.isEmpty()) {
        useSingleImageDefaults();
    }

    QString img;
    if (QDir::isAbsolutePath(m_wallpaper)) {
        syncWallpaperPackage();

        if (QFile::exists(m_wallpaperPath)) {
            img = m_wallpaperPath;
        }
    } else {
        // if it's not an absolute path, check if it's just a wallpaper name
        QString path =
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("wallpapers/") + m_wallpaper + QString::fromLatin1("/metadata.json"));
        if (path.isEmpty())
            path = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                          QLatin1String("wallpapers/") + m_wallpaper + QString::fromLatin1("/metadata.desktop"));

        if (!path.isEmpty()) {
            QDir dir(path);
            dir.cdUp();

            syncWallpaperPackage();
            img = m_wallpaperPath;
        }
    }

    if (img.isEmpty()) {
        // ok, so the package we have failed to work out; let's try the default
        useSingleImageDefaults();
        syncWallpaperPackage();
    }

    if (m_wallpaperPath != oldPath) {
        Q_EMIT wallpaperPathChanged();
    }
}

void ImageBackend::addUrls(const QList<QUrl> &urls)
{
    bool first = true;
    for (const QUrl &url : urls) {
        // set the first drop as the current paper, just add the rest to the roll
        addUrl(url, first);
        first = false;
    }
}

void ImageBackend::addUrl(const QUrl &url, bool setAsCurrent)
{
    QString path;
    if (url.isLocalFile()) {
        path = url.toLocalFile();
    } else if (url.scheme().isEmpty()) {
        if (QDir::isAbsolutePath(url.path())) {
            path = url.path();
        } else {
            path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("wallpapers/") + url.path(), QStandardPaths::LocateDirectory);
        }

        if (path.isEmpty()) {
            return;
        }
    } else {
        QDir wallpaperDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/wallpapers/");
        const QString wallpaperPath = wallpaperDir.absoluteFilePath(url.fileName());

        if (wallpaperDir.mkpath(wallpaperDir.absolutePath()) && !url.fileName().isEmpty()) {
            KIO::CopyJob *job = KIO::copy(url, QUrl::fromLocalFile(wallpaperPath), KIO::HideProgressInfo);

            if (setAsCurrent) {
                connect(job, &KJob::result, this, &ImageBackend::setWallpaperRetrieved);
            } else {
                connect(job, &KJob::result, this, &ImageBackend::addWallpaperRetrieved);
            }
        }

        return;
    }

    if (setAsCurrent) {
        setWallpaper(path);
    } else {
        if (m_mode != SingleImage) {
            // it's a slide show, add it to the slide show
            slideshowModel()->addBackground(path);
        }
        // always add it to the user papers, though
        addUsersWallpaper(path);
    }
}

void ImageBackend::setWallpaperRetrieved(KJob *job)
{
    KIO::CopyJob *copyJob = qobject_cast<KIO::CopyJob *>(job);
    if (copyJob && !copyJob->error()) {
        setWallpaper(copyJob->destUrl().toLocalFile());
    }
}

void ImageBackend::addWallpaperRetrieved(KJob *job)
{
    KIO::CopyJob *copyJob = qobject_cast<KIO::CopyJob *>(job);
    if (copyJob && !copyJob->error()) {
        addUrl(copyJob->destUrl(), false);
    }
}

void ImageBackend::setWallpaper(const QString &path)
{
    if (m_mode == SingleImage) {
        m_wallpaper = path;
        setSingleImage();
    } else {
        m_wallpaper = path;
        slideshowModel()->addBackground(path);
        m_currentSlide = m_slideFilterModel->indexOf(path) - 1;
        nextSlide();
    }
    // addUsersWallpaper(path);
}

void ImageBackend::startSlideshow()
{
    if (!m_ready || m_slideFilterModel->property("usedInConfig").toBool()) {
        return;
    }
    // populate background list
    m_timer.stop();
    slideshowModel()->reload(m_slidePaths);
    connect(m_slideshowModel, &SlideModel::done, this, &ImageBackend::backgroundsFound);
    // TODO: what would be cool: paint on the wallpaper itself a busy widget and perhaps some text
    // about loading wallpaper slideshow while the thread runs
}

void ImageBackend::backgroundsFound()
{
    disconnect(m_slideshowModel, &SlideModel::done, this, 0);

    // start slideshow
    if (m_slideFilterModel->rowCount() == 0) {
        // no image has been found, which is quite weird... try again later (this is useful for events which
        // are not detected by KDirWatch, like a NFS directory being mounted)
        QTimer::singleShot(1000, this, &ImageBackend::startSlideshow);
    } else {
        if (m_currentSlide == -1) {
            m_currentSlide = m_slideFilterModel->indexOf(m_wallpaper) - 1;
        } else {
            m_currentSlide = -1;
        }
        m_slideFilterModel->sort(0);
        nextSlide();
        m_timer.start(m_delay * 1000);
    }
}

void ImageBackend::newStuffFinished()
{
    if (m_model) {
        m_model->reload(m_usersWallpapers);
    }
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
        foreach (const QByteArray &mimeType, QImageReader::supportedMimeTypes()) {
            QMimeType mime(db.mimeTypeForName(mimeType));
            imageGlobPatterns << mime.globPatterns();
        }

        m_dialog = new QFileDialog(nullptr, i18n("Open Image"), path, i18n("Image Files") + " (" + imageGlobPatterns.join(' ') + ')');
        // i18n people, this isn't a "word puzzle". there is a specific string format for QFileDialog::setNameFilters

        m_dialog->setFileMode(QFileDialog::ExistingFiles);
        connect(m_dialog, &QDialog::accepted, this, &ImageBackend::wallpaperBrowseCompleted);
    }

    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
}

void ImageBackend::fileDialogFinished()
{
    m_dialog = nullptr;
}

void ImageBackend::wallpaperBrowseCompleted()
{
    Q_ASSERT(m_model);
    if (m_dialog && m_dialog->selectedFiles().count() > 0) {
        const QStringList selectedFiles = m_dialog->selectedFiles();
        for (const QString &image : selectedFiles) {
            addUsersWallpaper(image);
        }
        Q_EMIT customWallpaperPicked(m_dialog->selectedFiles().first());
    }
}

void ImageBackend::addUsersWallpaper(const QString &file)
{
    QString f = file;
    f.remove(QLatin1String("file:/"));
    const QFileInfo info(f); // FIXME

    // the full file path, so it isn't broken when dealing with symlinks
    const QString wallpaper = info.canonicalFilePath();

    if (wallpaper.isEmpty()) {
        return;
    }
    if (m_model) {
        if (m_model->contains(wallpaper)) {
            return;
        }
        // add background to the model
        m_model->addBackground(wallpaper);
    }
    // save it
    KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Wallpapers"));
    m_usersWallpapers = cfg.readEntry("usersWallpapers", m_usersWallpapers);

    if (!m_usersWallpapers.contains(wallpaper)) {
        m_usersWallpapers.prepend(wallpaper);
        cfg.writeEntry("usersWallpapers", m_usersWallpapers);
        cfg.sync();
        Q_EMIT usersWallpapersChanged();
    }
}

void ImageBackend::nextSlide()
{
    if (!m_ready || m_slideFilterModel->rowCount() == 0) {
        return;
    }
    int previousSlide = m_currentSlide;
    QUrl previousPath = m_slideFilterModel->index(m_currentSlide, 0).data(BackgroundListModel::PathRole).toUrl();
    if (m_currentSlide == m_slideFilterModel->rowCount() - 1 || m_currentSlide < 0) {
        m_currentSlide = 0;
    } else {
        m_currentSlide += 1;
    }
    // We are starting again - avoid having the same random order when we restart the slideshow
    if (m_slideshowMode == SortingMode::Random && m_currentSlide == 0) {
        m_slideFilterModel->invalidate();
    }
    QUrl next = m_slideFilterModel->index(m_currentSlide, 0).data(BackgroundListModel::PathRole).toUrl();
    // And  avoid showing the same picture twice
    if (previousSlide == m_slideFilterModel->rowCount() - 1 && previousPath == next && m_slideFilterModel->rowCount() > 1) {
        m_currentSlide += 1;
        next = m_slideFilterModel->index(m_currentSlide, 0).data(BackgroundListModel::PathRole).toUrl();
    }
    m_timer.stop();
    m_timer.start(m_delay * 1000);
    if (next.isEmpty()) {
        m_wallpaperPath = previousPath.toLocalFile();
    } else {
        m_wallpaperPath = next.toLocalFile();
    }
    Q_EMIT wallpaperPathChanged();
}

void ImageBackend::pathCreated(const QString &path)
{
    if (slideshowModel()->indexOf(path) == -1) {
        QFileInfo fileInfo(path);
        if (fileInfo.isFile() && BackgroundFinder::isAcceptableSuffix(fileInfo.suffix())) {
            m_slideshowModel->addBackground(path);
            if (m_slideFilterModel->rowCount() == 1) {
                nextSlide();
            }
        }
    }
}

void ImageBackend::pathDeleted(const QString &path)
{
    if (slideshowModel()->indexOf(path) != -1) {
        slideshowModel()->removeBackground(path);
        if (path == m_img) {
            nextSlide();
        }
    }
}

// FIXME: we have to save the configuration also when the dialog cancel button is clicked.
void ImageBackend::removeWallpaper(QString name)
{
    QString localWallpapers = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/wallpapers/";
    QUrl nameUrl(name);

    // Package plugin name
    if (!name.contains('/')) {
        KPackage::Package p = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
        KJob *j = p.uninstall(name, localWallpapers);
        connect(j, &KJob::finished, [=]() {
            m_model->reload(m_usersWallpapers);
        });
        // absolute path in the home
    } else if (nameUrl.path().startsWith(localWallpapers)) {
        QFile f(nameUrl.path());
        if (f.exists()) {
            f.remove();
        }
        m_model->reload(m_usersWallpapers);
    } else {
        // save it
        KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Wallpapers"));
        m_usersWallpapers = cfg.readEntry("usersWallpapers", m_usersWallpapers);

        int wallpaperIndex = -1;
        // passed as a path or as a file:// url?
        if (nameUrl.isValid()) {
            wallpaperIndex = m_usersWallpapers.indexOf(nameUrl.path());
        } else {
            wallpaperIndex = m_usersWallpapers.indexOf(name);
        }
        if (wallpaperIndex >= 0) {
            m_usersWallpapers.removeAt(wallpaperIndex);
            m_model->reload(m_usersWallpapers);
            cfg.writeEntry("usersWallpapers", m_usersWallpapers);
            cfg.sync();
            Q_EMIT usersWallpapersChanged();
            Q_EMIT settingsChanged(true);
        }
    }
}

void ImageBackend::commitDeletion()
{
    // This is invokable from qml, so at any moment
    // we can't be sure the model exists
    if (!m_model) {
        return;
    }

    for (const QString &wallpaperCandidate : m_model->wallpapersAwaitingDeletion()) {
        removeWallpaper(wallpaperCandidate);
    }
}

void ImageBackend::openFolder(const QString &path)
{
    auto *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(path));
    auto *delegate = new KNotificationJobUiDelegate;
    delegate->setAutoErrorHandlingEnabled(true);
    job->setUiDelegate(delegate);
    job->start();
}

void ImageBackend::toggleSlide(const QString &path, bool checked)
{
    if (checked && m_uncheckedSlides.contains(path)) {
        m_uncheckedSlides.removeAll(path);
        Q_EMIT uncheckedSlidesChanged();
        startSlideshow();
    } else if (!checked && !m_uncheckedSlides.contains(path)) {
        m_uncheckedSlides.append(path);
        Q_EMIT uncheckedSlidesChanged();
        startSlideshow();
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
    Q_EMIT uncheckedSlidesChanged();
    startSlideshow();
}
