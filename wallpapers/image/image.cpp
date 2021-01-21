/***************************************************************************
 *   Copyright 2007 Paolo Capriotti <p.capriotti@gmail.com>                *
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>                           *
 *   Copyright 2008 Petri Damsten <damu@iki.fi>                            *
 *   Copyright 2008 Alexis Ménard <darktears31@gmail.com>                  *
 *   Copyright 2014 Sebastian Kügler <sebas@kde.org>                       *
 *   Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>                *
 *   Copyright 2019 David Redondo <kde@david-redondo.de>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "image.h"
#include "debug.h"

#include <float.h> // FLT_MAX
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

#include <KDirWatch>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KRandom>
#include <QDebug>
#include <QFileDialog>
#include <klocalizedstring.h>
#include <krun.h>

#include "backgroundlistmodel.h"
#include "slidefiltermodel.h"
#include "slidemodel.h"
#include <Plasma/PluginLoader>
#include <Plasma/Theme>
#include <qstandardpaths.h>

#include <KPackage/PackageLoader>

Image::Image(QObject *parent)
    : QObject(parent)
    , m_ready(false)
    , m_delay(10)
    , m_dirWatch(new KDirWatch(this))
    , m_mode(SingleImage)
    , m_slideshowMode(Random)
    , m_currentSlide(-1)
    , m_model(nullptr)
    , m_slideshowModel(new SlideModel(this, this))
    , m_slideFilterModel(new SlideFilterModel(this))
    , m_dialog(nullptr)
{
    m_wallpaperPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    connect(&m_timer, &QTimer::timeout, this, &Image::nextSlide);

    connect(m_dirWatch, &KDirWatch::created, this, &Image::pathCreated);
    connect(m_dirWatch, &KDirWatch::dirty, this, &Image::pathDirty);
    connect(m_dirWatch, &KDirWatch::deleted, this, &Image::pathDeleted);
    m_dirWatch->startScan();

    m_slideFilterModel->setSourceModel(m_slideshowModel);
    connect(this, &Image::uncheckedSlidesChanged, m_slideFilterModel, &SlideFilterModel::invalidateFilter);

    useSingleImageDefaults();
}

Image::~Image()
{
    delete m_dialog;
}

void Image::classBegin()
{
}

void Image::componentComplete()
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
        emit wallpaperPathChanged();
        startSlideshow();
    }
}

QString Image::photosPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

QUrl Image::wallpaperPath() const
{
    return QUrl::fromLocalFile(m_wallpaperPath);
}

void Image::addUrl(const QString &url)
{
    addUrl(QUrl(url), true);
}

void Image::addUrls(const QStringList &urls)
{
    bool first = true;
    for (const QString &url : urls) {
        // set the first drop as the current paper, just add the rest to the roll
        addUrl(QUrl(url), first);
        first = false;
    }
}

Image::RenderingMode Image::renderingMode() const
{
    return m_mode;
}

void Image::setRenderingMode(RenderingMode mode)
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

Image::SlideshowMode Image::slideshowMode() const
{
    return m_slideshowMode;
}

void Image::setSlideshowMode(Image::SlideshowMode mode)
{
    if (mode == m_slideshowMode) {
        return;
    }
    m_slideshowMode = mode;
    m_slideFilterModel->setSortingMode(mode);
    m_slideFilterModel->sort(0);
    if (m_mode == SlideShow) {
        startSlideshow();
    }
    emit slideshowModeChanged();
}

float distance(const QSize &size, const QSize &desired)
{
    // compute difference of areas
    float desiredAspectRatio = (desired.height() > 0) ? desired.width() / (float)desired.height() : 0;
    float candidateAspectRatio = (size.height() > 0) ? size.width() / (float)size.height() : FLT_MAX;

    float delta = size.width() - desired.width();
    delta = (delta >= 0.0 ? delta : -delta * 2); // Penalize for scaling up

    return qAbs(candidateAspectRatio - desiredAspectRatio) * 25000 + delta;
}

QSize resSize(const QString &str)
{
    int index = str.indexOf('x');
    if (index != -1) {
        return QSize(str.leftRef(index).toInt(), str.midRef(index + 1).toInt());
    }

    return QSize();
}

QString Image::findPreferedImage(const QStringList &images)
{
    if (images.empty()) {
        return QString();
    }

    // float targetAspectRatio = (m_targetSize.height() > 0 ) ? m_targetSize.width() / (float)m_targetSize.height() : 0;
    // qCDebug(IMAGEWALLPAPER) << "wanted" << m_targetSize << "options" << images << "aspect ratio" << targetAspectRatio;
    float best = FLT_MAX;

    QString bestImage;
    for (const QString &entry : images) {
        QSize candidate = resSize(QFileInfo(entry).baseName());
        if (candidate == QSize()) {
            continue;
        }
        // float candidateAspectRatio = (candidate.height() > 0 ) ? candidate.width() / (float)candidate.height() : FLT_MAX;

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

void Image::findPreferedImageInPackage(KPackage::Package &package)
{
    if (!package.isValid() || !package.filePath("preferred").isEmpty()) {
        return;
    }

    QString preferred = findPreferedImage(package.entryList("images"));

    package.removeDefinition("preferred");
    package.addFileDefinition("preferred", QStringLiteral("images/") + preferred, i18n("Recommended wallpaper file"));
}

QSize Image::targetSize() const
{
    return m_targetSize;
}

void Image::setTargetSize(const QSize &size)
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
        emit targetSizeChanged();
    }
}

KPackage::Package *Image::package()
{
    return &m_wallpaperPackage;
}

void Image::useSingleImageDefaults()
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
        int index = m_wallpaper.indexOf(QString::fromLatin1("/contents/images/"));
        if (index > -1) { // We have file from package -> get path to package
            m_wallpaper = m_wallpaper.left(index);
        }
    }
}

QAbstractItemModel *Image::wallpaperModel()
{
    if (!m_model) {
        KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Wallpapers"));
        m_usersWallpapers = cfg.readEntry("usersWallpapers", m_usersWallpapers);

        m_model = new BackgroundListModel(this, this);
        m_model->reload(m_usersWallpapers);
    }

    return m_model;
}

QAbstractItemModel *Image::slideFilterModel()
{
    return m_slideFilterModel;
}
int Image::slideTimer() const
{
    return m_delay;
}

void Image::setSlideTimer(int time)
{
    if (time == m_delay) {
        return;
    }

    m_delay = time;

    if (m_mode == SlideShow) {
        updateDirWatch(m_slidePaths);
        startSlideshow();
    }

    emit slideTimerChanged();
}

QStringList Image::usersWallpapers() const
{
    return m_usersWallpapers;
}

void Image::setUsersWallpapers(const QStringList &usersWallpapers)
{
    if (usersWallpapers == m_usersWallpapers) {
        return;
    }

    m_usersWallpapers = usersWallpapers;

    emit usersWallpapersChanged();
}

QStringList Image::slidePaths() const
{
    return m_slidePaths;
}

void Image::setSlidePaths(const QStringList &slidePaths)
{
    if (slidePaths == m_slidePaths) {
        return;
    }

    m_slidePaths = slidePaths;
    m_slidePaths.removeAll(QString());

    if (!m_slidePaths.isEmpty()) {
        // Replace 'preferred://wallpaperlocations' with real paths
        const QStringList preProcessedPaths = m_slidePaths;
        for (const QString &path : preProcessedPaths) {
            if (path == QLatin1String("preferred://wallpaperlocations")) {
                m_slidePaths << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers"), QStandardPaths::LocateDirectory);
                m_slidePaths.removeAll(path);
            }
        }
    }

    if (m_mode == SlideShow) {
        updateDirWatch(m_slidePaths);
        startSlideshow();
    }
    if (m_slideshowModel) {
        m_slideshowModel->reload(m_slidePaths);
    }
    emit slidePathsChanged();
}

void Image::showAddSlidePathsDialog()
{
    QFileDialog *dialog = new QFileDialog(nullptr, i18n("Directory with the wallpaper to show slides from"), QString());
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setOptions(QFileDialog::ShowDirsOnly);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    connect(dialog, &QDialog::accepted, this, &Image::addDirFromSelectionDialog);
    dialog->show();
}

void Image::addSlidePath(const QString &path)
{
    if (!path.isEmpty() && !m_slidePaths.contains(path)) {
        m_slidePaths.append(path);
        if (m_mode == SlideShow) {
            updateDirWatch(m_slidePaths);
        }
        if (m_slideshowModel) {
            m_slideshowModel->addDirs({m_slidePaths});
        }
        emit slidePathsChanged();
        startSlideshow();
    }
}

void Image::removeSlidePath(const QString &path)
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

        emit slidePathsChanged();
        startSlideshow();
    }
}

void Image::pathDirty(const QString &path)
{
    updateDirWatch(QStringList(path));
}

void Image::updateDirWatch(const QStringList &newDirs)
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

void Image::addDirFromSelectionDialog()
{
    QFileDialog *dialog = qobject_cast<QFileDialog *>(sender());
    if (dialog) {
        addSlidePath(dialog->directoryUrl().toLocalFile());
    }
}

void Image::syncWallpaperPackage()
{
    m_wallpaperPackage.setPath(m_wallpaper);
    findPreferedImageInPackage(m_wallpaperPackage);
    m_wallpaperPath = m_wallpaperPackage.filePath("preferred");
}

void Image::setSingleImage()
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

void Image::addUrls(const QList<QUrl> &urls)
{
    bool first = true;
    for (const QUrl &url : urls) {
        // set the first drop as the current paper, just add the rest to the roll
        addUrl(url, first);
        first = false;
    }
}

void Image::addUrl(const QUrl &url, bool setAsCurrent)
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
                connect(job, &KJob::result, this, &Image::setWallpaperRetrieved);
            } else {
                connect(job, &KJob::result, this, &Image::addWallpaperRetrieved);
            }
        }

        return;
    }

    if (setAsCurrent) {
        setWallpaper(path);
    } else {
        if (m_mode != SingleImage) {
            // it's a slide show, add it to the slide show
            m_slideshowModel->addBackground(path);
        }
        // always add it to the user papers, though
        addUsersWallpaper(path);
    }
}

void Image::setWallpaperRetrieved(KJob *job)
{
    KIO::CopyJob *copyJob = qobject_cast<KIO::CopyJob *>(job);
    if (copyJob && !copyJob->error()) {
        setWallpaper(copyJob->destUrl().toLocalFile());
    }
}

void Image::addWallpaperRetrieved(KJob *job)
{
    KIO::CopyJob *copyJob = qobject_cast<KIO::CopyJob *>(job);
    if (copyJob && !copyJob->error()) {
        addUrl(copyJob->destUrl(), false);
    }
}

void Image::setWallpaper(const QString &path)
{
    if (m_mode == SingleImage) {
        m_wallpaper = path;
        setSingleImage();
    } else {
        m_wallpaper = path;
        m_slideshowModel->addBackground(path);
        m_currentSlide = m_slideFilterModel->indexOf(path) - 1;
        nextSlide();
    }
    // addUsersWallpaper(path);
}

void Image::startSlideshow()
{
    if (!m_ready || m_slideFilterModel->property("usedInConfig").toBool()) {
        return;
    }
    // populate background list
    m_timer.stop();
    m_slideshowModel->reload(m_slidePaths);
    connect(m_slideshowModel, &SlideModel::done, this, &Image::backgroundsFound);
    // TODO: what would be cool: paint on the wallpaper itself a busy widget and perhaps some text
    // about loading wallpaper slideshow while the thread runs
}

void Image::backgroundsFound()
{
    disconnect(m_slideshowModel, &SlideModel::done, this, 0);

    if (m_scanDirty) {
        m_scanDirty = false;
        startSlideshow();
        return;
    }

    // start slideshow
    if (m_slideFilterModel->rowCount() == 0) {
        // no image has been found, which is quite weird... try again later (this is useful for events which
        // are not detected by KDirWatch, like a NFS directory being mounted)
        QTimer::singleShot(1000, this, &Image::startSlideshow);
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

void Image::newStuffFinished()
{
    if (m_model) {
        m_model->reload(m_usersWallpapers);
    }
}

void Image::showFileDialog()
{
    if (!m_dialog) {
        QUrl baseUrl;
        if (m_wallpaper.indexOf(QDir::homePath()) > -1) {
            baseUrl = QUrl(m_wallpaper);
        }

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
        connect(m_dialog, &QDialog::accepted, this, &Image::wallpaperBrowseCompleted);
    }

    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
}

void Image::fileDialogFinished()
{
    m_dialog = nullptr;
}

void Image::wallpaperBrowseCompleted()
{
    Q_ASSERT(m_model);
    if (m_dialog && m_dialog->selectedFiles().count() > 0) {
        const QStringList selectedFiles = m_dialog->selectedFiles();
        for (const QString &image : selectedFiles) {
            addUsersWallpaper(image);
        }
        emit customWallpaperPicked(m_dialog->selectedFiles().first());
    }
}

void Image::addUsersWallpaper(const QString &file)
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
        emit usersWallpapersChanged();
    }
}

void Image::nextSlide()
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
    if (m_slideshowMode == Random && m_currentSlide == 0) {
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

void Image::openSlide()
{
    if (!m_wallpaperPackage.isValid()) {
        return;
    }

    // open in image viewer
    QUrl filepath(m_wallpaperPackage.filePath("preferred"));
    qCDebug(IMAGEWALLPAPER) << "opening file " << filepath.path();
    new KRun(filepath, nullptr);
}

void Image::pathCreated(const QString &path)
{
    if (m_slideshowModel->indexOf(path) == -1) {
        QFileInfo fileInfo(path);
        if (fileInfo.isFile() && BackgroundFinder::isAcceptableSuffix(fileInfo.suffix())) {
            m_slideshowModel->addBackground(path);
            if (m_slideFilterModel->rowCount() == 1) {
                nextSlide();
            }
        }
    }
}

void Image::pathDeleted(const QString &path)
{
    if (m_slideshowModel->indexOf(path) != -1) {
        m_slideshowModel->removeBackground(path);
        if (path == m_img) {
            nextSlide();
        }
    }
}

// FIXME: we have to save the configuration also when the dialog cancel button is clicked.
void Image::removeWallpaper(QString name)
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
            emit usersWallpapersChanged();
            Q_EMIT settingsChanged(true);
        }
    }
}

void Image::commitDeletion()
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

void Image::openFolder(const QString &path)
{
    new KRun(QUrl::fromLocalFile(path), nullptr);
}

void Image::toggleSlide(const QString &path, bool checked)
{
    if (checked && m_uncheckedSlides.contains(path)) {
        m_uncheckedSlides.removeAll(path);
        emit uncheckedSlidesChanged();
        startSlideshow();
    } else if (!checked && !m_uncheckedSlides.contains(path)) {
        m_uncheckedSlides.append(path);
        emit uncheckedSlidesChanged();
        startSlideshow();
    }
}

QStringList Image::uncheckedSlides() const
{
    return m_uncheckedSlides;
}

void Image::setUncheckedSlides(const QStringList &uncheckedSlides)
{
    if (uncheckedSlides == m_uncheckedSlides) {
        return;
    }
    m_uncheckedSlides = uncheckedSlides;
    emit uncheckedSlidesChanged();
    startSlideshow();
}
