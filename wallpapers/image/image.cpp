/***************************************************************************
 *   Copyright 2007 Paolo Capriotti <p.capriotti@gmail.com>                *
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>                           *
 *   Copyright 2008 Petri Damsten <damu@iki.fi>                            *
 *   Copyright 2008 Alexis Ménard <darktears31@gmail.com>                  *
 *   Copyright 2014 Sebastian Kügler <sebas@kde.org>                       *
 *   Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>                *
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

#include <math.h>
#include <float.h> // FLT_MAX

#include <QAction>
#include <QApplication>
#include <QPainter>
#include <QFile>
#include <QEasingCurve>
#include <QPropertyAnimation>
#include <QTimer>
#include <QMimeDatabase>
#include <QImageReader>

#include <QDebug>
#include <KDirWatch>
#include <QFileDialog>
#include <KRandom>
#include <KIO/Job>
#include <krun.h>
#include <KNewStuff3/KNS3/DownloadDialog>
#include <klocalizedstring.h>

#include <Plasma/Theme>
#include <Plasma/PluginLoader>
#include <qstandardpaths.h>
#include "backgroundlistmodel.h"

#include <KPackage/PackageLoader>

Image::Image(QObject *parent)
    : QObject(parent),
      m_ready(false),
      m_delay(10),
      m_dirWatch(new KDirWatch(this)),
      m_mode(SingleImage),
      m_currentSlide(-1),
      m_model(0),
      m_dialog(0),
      m_width(0),
      m_height(0)
{
    m_wallpaperPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    connect(&m_timer, &QTimer::timeout, this, &Image::nextSlide);

    connect(m_dirWatch, &KDirWatch::created, this, &Image::pathCreated);
    connect(m_dirWatch, &KDirWatch::dirty,   this, &Image::pathDirty);
    connect(m_dirWatch, &KDirWatch::deleted, this, &Image::pathDeleted);
    m_dirWatch->startScan();

    connect(this, &Image::sizeChanged, this, &Image::setTargetSize);

    useSingleImageDefaults();
    setSingleImage();
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
    for (const QString &url: urls) {
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
        if (m_slidePaths.isEmpty()) {
            m_slidePaths << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("share/wallpapers"), QStandardPaths::LocateDirectory);
        }

        QTimer::singleShot(200, this, &Image::startSlideshow);
        updateDirWatch(m_slidePaths);
        updateDirWatch(m_slidePaths);
    } else {
        // we need to reset the prefered image
        setSingleImage();
    }
}

float distance(const QSize& size, const QSize& desired)
{
    // compute difference of areas
    float desiredAspectRatio = ( desired.height() > 0 ) ? desired.width() / (float)desired.height() : 0;
    float candidateAspectRatio = ( size.height() > 0 ) ? size.width() / (float)size.height() : FLT_MAX;

    float delta = size.width() - desired.width();
    delta = (delta >= 0.0 ? delta : -delta*2 ); // Penalize for scaling up

    return qAbs(candidateAspectRatio - desiredAspectRatio)*25000 + delta;

}

QSize resSize(const QString &str)
{
    int index = str.indexOf('x');
    if (index != -1) {
        return QSize(str.leftRef(index).toInt(),
                     str.midRef(index + 1).toInt());
    }

    return QSize();
}

QString Image::findPreferedImage(const QStringList &images)
{
    if (images.empty()) {
        return QString();
    }

    //float targetAspectRatio = (m_targetSize.height() > 0 ) ? m_targetSize.width() / (float)m_targetSize.height() : 0;
    //qDebug() << "wanted" << m_targetSize << "options" << images << "aspect ratio" << targetAspectRatio;
    float best = FLT_MAX;

    QString bestImage;
    foreach (const QString &entry, images) {
        QSize candidate = resSize(QFileInfo(entry).baseName());
        if (candidate == QSize()) {
            continue;
        }
        //float candidateAspectRatio = (candidate.height() > 0 ) ? candidate.width() / (float)candidate.height() : FLT_MAX;

        float dist = distance(candidate, m_targetSize);
        //qDebug() << "candidate" << candidate << "distance" << dist << "aspect ratio" << candidateAspectRatio;

        if (bestImage.isEmpty() || dist < best) {
            bestImage = entry;
            best = dist;
            //qDebug() << "best" << bestImage;
        }
    }

    //qDebug() << "best image" << bestImage;
    return bestImage;
}

void Image::findPreferedImageInPackage(KPackage::Package &package)
{
    if (!package.isValid() || !package.filePath("preferred").isEmpty()) {
        return;
    }

    QString preferred = findPreferedImage( package.entryList("images") );

    package.removeDefinition("preferred");
    package.addFileDefinition("preferred", "images/" + preferred, i18n("Recommended wallpaper file"));
}

QSize Image::targetSize() const
{
    return m_targetSize;
}

void Image::setTargetSize(const QSize &size)
{
    m_targetSize = size;

    if (m_mode == SingleImage) {
        setSingleImage();
    }
}

int Image::height() const
{
    return m_height;
}

void Image::setHeight(int h)
{
    if (m_height != h) {
        m_height = h;
        emit sizeChanged(QSize(m_width, m_height));
    }
}

int Image::width() const
{
    return m_width;
}

void Image::setWidth(int w)
{
    if (m_width != w) {
        m_width = w;
        emit sizeChanged(QSize(m_width, m_height));
    }
}

KPackage::Package *Image::package()
{
    return &m_wallpaperPackage;
}

void Image::useSingleImageDefaults()
{
    Plasma::Theme theme;
    m_wallpaper = theme.wallpaperPath();
    int index = m_wallpaper.indexOf(QString::fromLatin1("/contents/images/"));
    if (index > -1) { // We have file from package -> get path to package
        m_wallpaper = m_wallpaper.left(index);
    }
}

QAbstractItemModel* Image::wallpaperModel()
{
    if (!m_model) {
        KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")),
                                                                  QStringLiteral("Wallpapers"));
        m_usersWallpapers = cfg.readEntry("usersWallpapers", m_usersWallpapers);

        m_model = new BackgroundListModel(this, this);
        m_model->reload(m_usersWallpapers);
    }

    return m_model;
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

    if (m_slidePaths.isEmpty()) {
        m_slidePaths << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("share/wallpapers"), QStandardPaths::LocateDirectory);
    }

    if (m_mode == SlideShow) {
        updateDirWatch(m_slidePaths);
        startSlideshow();
    }

    emit slidePathsChanged();
}

void Image::showAddSlidePathsDialog()
{
    QFileDialog *dialog = new QFileDialog(0, i18n("Directory with the wallpaper to show slides from"), QLatin1String(""));
    dialog->setAttribute(Qt::WA_DeleteOnClose, true );
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

        emit slidePathsChanged();
        startSlideshow();
    }
}

void Image::pathDirty(const QString& path)
{
    updateDirWatch(QStringList(path));
}

void Image::updateDirWatch(const QStringList &newDirs)
{
    Q_FOREACH(const QString &oldDir, m_dirs) {
        if(!newDirs.contains(oldDir)) {
            m_dirWatch->removeDir(oldDir);
        }
    }

    Q_FOREACH(const QString &newDir, newDirs) {
        if(!m_dirWatch->contains(newDir)) {
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
        //if it's not an absolute path, check if it's just a wallpaper name
        QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("wallpapers/") + QString(m_wallpaper + QString::fromLatin1("/metadata.json")));
        if (path.isEmpty())
            path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("wallpapers/") + QString(m_wallpaper + QString::fromLatin1("/metadata.desktop")));

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
    Q_FOREACH (const QUrl &url, urls) {
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
            path = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                          QLatin1String("wallpapers/") + url.path(),
                                          QStandardPaths::LocateDirectory);
        }

        if (path.isEmpty()) {
            return;
        }
    } else {
        QString wallpaperPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("wallpapers/") + url.path();

        if (!wallpaperPath.isEmpty()) {
            KIO::FileCopyJob *job = KIO::file_copy(url, QUrl(wallpaperPath), -1, KIO::HideProgressInfo);
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
            m_slideshowBackgrounds.append(path);
            m_unseenSlideshowBackgrounds.append(path);
        }
        // always add it to the user papers, though
        addUsersWallpaper(path);
    }
}

void Image::setWallpaperRetrieved(KJob *job)
{
    KIO::FileCopyJob *copyJob = qobject_cast<KIO::FileCopyJob *>(job);
    if (copyJob && !copyJob->error()) {
        setWallpaper(copyJob->destUrl().toLocalFile());
    }
}

void Image::addWallpaperRetrieved(KJob *job)
{
    KIO::FileCopyJob *copyJob = qobject_cast<KIO::FileCopyJob *>(job);
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
        m_slideshowBackgrounds.append(path);
        m_unseenSlideshowBackgrounds.clear();
        m_currentSlide = m_slideshowBackgrounds.size() - 2;
        nextSlide();
    }
    //addUsersWallpaper(path);
}

void Image::startSlideshow()
{
    if(m_findToken.isEmpty()) {
        // populate background list
        m_timer.stop();
        m_slideshowBackgrounds.clear();
        m_unseenSlideshowBackgrounds.clear();
        BackgroundFinder *finder = new BackgroundFinder(this, m_dirs);
        m_findToken = finder->token();
        connect(finder, &BackgroundFinder::backgroundsFound, this, &Image::backgroundsFound);
        finder->start();
        //TODO: what would be cool: paint on the wallpaper itself a busy widget and perhaps some text
        //about loading wallpaper slideshow while the thread runs
    } else {
        m_scanDirty = true;
    }
}

void Image::backgroundsFound(const QStringList &paths, const QString &token)
{
    if (token != m_findToken) {
        return;
    }

    m_findToken.clear();

    if(m_scanDirty) {
        m_scanDirty = false;
        startSlideshow();
        return;
    }

    m_slideshowBackgrounds = paths;
    m_unseenSlideshowBackgrounds.clear();
    // start slideshow
    if (m_slideshowBackgrounds.isEmpty()) {
        // no image has been found, which is quite weird... try again later (this is useful for events which
        // are not detected by KDirWatch, like a NFS directory being mounted)
        QTimer::singleShot(1000, this, &Image::startSlideshow);
    } else {
        m_currentSlide = -1;
        nextSlide();
        m_timer.start(m_delay * 1000);
    }
}

void Image::getNewWallpaper()
{
    if (!m_newStuffDialog) {
        m_newStuffDialog = new KNS3::DownloadDialog( QString::fromLatin1("wallpaper.knsrc") );
        KNS3::DownloadDialog *strong = m_newStuffDialog.data();
        strong->setTitle(i18n("Download Wallpapers"));
        connect(m_newStuffDialog.data(), &QDialog::accepted, this, &Image::newStuffFinished);
    }
    m_newStuffDialog.data()->show();
}

void Image::newStuffFinished()
{
    if (m_model && (!m_newStuffDialog || m_newStuffDialog.data()->changedEntries().size() > 0)) {
        m_model->reload(m_usersWallpapers);
    }
}

void Image::showFileDialog()
{
    if (!m_dialog) {
        QUrl baseUrl;
        if(m_wallpaper.indexOf(QDir::homePath()) > -1){
            baseUrl = QUrl(m_wallpaper);
        }
        /*
        m_dialog = new KFileDialog(baseUrl, QString::fromLatin1("*.png *.jpeg *.jpg *.xcf *.svg *.svgz *.bmp"), 0);
        m_dialog->setOperationMode(KFileDialog::Opening);
        m_dialog->setInlinePreviewShown(true);
        m_dialog->setModal(false);

        connect(m_dialog, SIGNAL(okClicked()), this, SLOT(wallpaperBrowseCompleted()));
        connect(m_dialog, SIGNAL(destroyed(QObject*)), this, SLOT(fileDialogFinished()));
        */

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
        foreach(const QByteArray &mimeType, QImageReader::supportedMimeTypes()) {
            QMimeType mime(db.mimeTypeForName(mimeType));
            imageGlobPatterns << mime.globPatterns();
        }

        m_dialog = new QFileDialog(0, i18n("Open Image"),
                                      path,
                                      i18n("Image Files") + " ("+imageGlobPatterns.join(' ') + ')');
        //i18n people, this isn't a "word puzzle". there is a specific string format for QFileDialog::setNameFilters

        m_dialog->setFileMode(QFileDialog::ExistingFile);
        connect(m_dialog, &QDialog::accepted, this, &Image::wallpaperBrowseCompleted);
    }

    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
}

void Image::fileDialogFinished()
{
    m_dialog = 0;
}

void Image::wallpaperBrowseCompleted()
{
    Q_ASSERT(m_model);
    if (m_dialog && m_dialog->selectedFiles().count() > 0) {
        addUsersWallpaper(m_dialog->selectedFiles().first());
        emit customWallpaperPicked();
    }
}

void Image::addUsersWallpaper(const QString &file)
{
    QString f = file;
    f.replace(QLatin1String("file:/"), QLatin1String(""));
    const QFileInfo info(f); // FIXME

    //the full file path, so it isn't broken when dealing with symlinks
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
    KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")),
                                                              QStringLiteral("Wallpapers"));
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
    if (m_slideshowBackgrounds.isEmpty()) {
        return;
    }

    QString previousPath;
    if (m_currentSlide > -1 && m_currentSlide < m_unseenSlideshowBackgrounds.size()) {
        previousPath = m_unseenSlideshowBackgrounds.takeAt(m_currentSlide);
    }

    if (m_unseenSlideshowBackgrounds.isEmpty()) {
        m_unseenSlideshowBackgrounds = m_slideshowBackgrounds;

        // We're filling the queue again, make sure we can't pick up again
        // the last one picked from the previous set
        if (!previousPath.isEmpty()) {
            m_unseenSlideshowBackgrounds.removeAll(previousPath);

            // prevent empty list
            if (m_unseenSlideshowBackgrounds.isEmpty()) {
                m_unseenSlideshowBackgrounds = m_slideshowBackgrounds;
            }
        }
    }

    m_currentSlide = KRandom::random() % m_unseenSlideshowBackgrounds.size();
    const QString currentPath = m_unseenSlideshowBackgrounds.at(m_currentSlide);

    m_wallpaperPackage.setPath(currentPath);
    findPreferedImageInPackage(m_wallpaperPackage);

    m_timer.stop();
    m_timer.start(m_delay * 1000);

    QString current = m_wallpaperPackage.filePath("preferred");
    if (current.isEmpty()) {
        m_wallpaperPath = currentPath;
    } else {
        m_wallpaperPath = current;
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
    qDebug() << "opening file " << filepath.path();
    new KRun(filepath, NULL);
}

void Image::pathCreated(const QString &path)
{
    if(!m_slideshowBackgrounds.contains(path)) {
        QFileInfo fileInfo(path);
        if(fileInfo.isFile() && BackgroundFinder::isAcceptableSuffix(fileInfo.suffix())) {
            m_slideshowBackgrounds.append(path);
            m_unseenSlideshowBackgrounds.append(path);
            if(m_slideshowBackgrounds.count() == 1) {
                nextSlide();
            }
        }
    }
}

void Image::pathDeleted(const QString &path)
{
    if(m_slideshowBackgrounds.removeAll(path)) {
        m_unseenSlideshowBackgrounds.removeAll(path);
        if(path == m_img) {
            nextSlide();
        }
    }
}

//FIXME: we have to save the configuration also when the dialog cancel button is clicked.
void Image::removeWallpaper(QString name)
{
    QString localWallpapers = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/wallpapers/";
    QUrl nameUrl(name);

    //Package plugin name
    if (!name.contains('/')) {
        KPackage::Package p = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
        KJob *j = p.uninstall(name, localWallpapers);
        connect(j, &KJob::finished, [=] () {
            m_model->reload(m_usersWallpapers);
        });
    //absolute path in the home
    } else if (nameUrl.path().startsWith(localWallpapers)) {
        QFile f(nameUrl.path());
        if (f.exists()) {
            f.remove();
        }
        m_model->reload(m_usersWallpapers);
    } else {
        // save it
        KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")),
                                                                QStringLiteral("Wallpapers"));
        m_usersWallpapers = cfg.readEntry("usersWallpapers", m_usersWallpapers);

        int wallpaperIndex = -1;
        //passed as a path or as a file:// url?
        if (nameUrl.isValid()) {
            wallpaperIndex = m_usersWallpapers.indexOf(nameUrl.path());
        } else {
            wallpaperIndex = m_usersWallpapers.indexOf(name);
        }
        if (wallpaperIndex >= 0){
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
    //This is invokable from qml, so at any moment
    //we can't be sure the model exists
    if (!m_model) {
        return;
    }

    for (const QString &wallpaperCandidate : m_model->wallpapersAwaitingDeletion()) {
        removeWallpaper(wallpaperCandidate);
    }
}

