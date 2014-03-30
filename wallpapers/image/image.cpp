/***************************************************************************
 *   Copyright 2007 Paolo Capriotti <p.capriotti@gmail.com>                *
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>                           *
 *   Copyright 2008 Petri Damsten <damu@iki.fi>                            *
 *   Copyright 2008 Alexis Ménard <darktears31@gmail.com>                  *
 *   Copyright 2014 Sebastian Kügler <sebas@kde.org>                       *
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
#include "wallpaperpackage.h"

#include <QAction>
#include <QApplication>
#include <QPainter>
#include <QFile>
#include <QEasingCurve>
#include <QPropertyAnimation>
#include <QTimer>

#include <QDebug>
#include <KDirSelectDialog>
#include <KDirWatch>
#include <QFileDialog>
#include <KRandom>
#include <KStandardDirs>
#include <KIO/Job>
#include <krun.h>
#include <KNewStuff3/KNS3/DownloadDialog>
#include <klocalizedstring.h>
#include <KGlobal>

#include <Plasma/Theme>
#include <qstandardpaths.h>
#include "backgroundlistmodel.h"

Image::Image(QObject *parent)
    : QObject(parent),
      m_delay(10),
      m_dirWatch(new KDirWatch(this)),
      m_mode(SingleImage),
      m_currentSlide(-1),
      m_model(0),
      m_dialog(0)
{
    m_wallpaperPackage = Plasma::Package(new WallpaperPackage(this, this));

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(nextSlide()));

    connect(m_dirWatch, SIGNAL(created(QString)), SLOT(pathCreated(QString)));
    connect(m_dirWatch, SIGNAL(dirty(QString)),   SLOT(pathDirty(QString)));
    connect(m_dirWatch, SIGNAL(deleted(QString)), SLOT(pathDeleted(QString)));
    m_dirWatch->startScan();

    connect(this, &Image::sizeChanged, this, &Image::setTargetSize);

    useSingleImageDefaults();
    setSingleImage();
}

Image::~Image()
{
}

QString Image::wallpaperPath() const
{
    return m_wallpaperPath;
}

void Image::addUrl(const QString &url)
{
    addUrl(QUrl(url), true);
}

void Image::addUrls(const QStringList &urls)
{
    addUrls(urls);
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
            m_slidePaths << KStandardDirs::installPath("wallpaper");
        }

        QTimer::singleShot(200, this, SLOT(startSlideshow()));
        updateDirWatch(m_slidePaths);
        updateDirWatch(m_slidePaths);
    }

    emit renderingModeChanged();
}

QSize Image::targetSize() const
{
    return m_targetSize;
}

void Image::setTargetSize(const QSize &size)
{
    m_targetSize = size;
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



Plasma::Package *Image::package()
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

    if (m_slidePaths.isEmpty()) {
        m_slidePaths << KStandardDirs::installPath("wallpaper");
    }

    if (m_mode == SlideShow) {
        updateDirWatch(m_slidePaths);
        startSlideshow();
    }

    emit slidePathsChanged();
}

void Image::showAddSlidePathsDialog()
{
    QUrl empty;
    KDirSelectDialog *dialog = new KDirSelectDialog(empty, true);
    connect(dialog, SIGNAL(accepted()), this, SLOT(addDirFromSelectionDialog()));
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
    if (!path.isEmpty() && m_slidePaths.contains(path)) {
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
    KDirSelectDialog *dialog = qobject_cast<KDirSelectDialog *>(sender());
    if (dialog) {
        addSlidePath(dialog->url().path());
    }
}

void Image::setSingleImage()
{
    if (m_wallpaper.isEmpty()) {
        useSingleImageDefaults();
    }

    QString img;
    if (QDir::isAbsolutePath(m_wallpaper)) {
        m_wallpaperPackage.setPath(m_wallpaper);
        img = m_wallpaperPackage.filePath("preferred");
        m_wallpaperPath = m_wallpaperPackage.filePath("preferred");

        if (img.isEmpty() && QFile::exists(m_wallpaper)) {
            img = m_wallpaper;
            m_wallpaperPath = m_wallpaper;
        }
        Q_EMIT wallpaperPathChanged();
    } else {
        //if it's not an absolute path, check if it's just a wallpaper name
        const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("wallpapers/") + QString(m_wallpaper + QString::fromLatin1("/metadata.desktop")));

        if (!path.isEmpty()) {
            QDir dir(path);
            dir.cdUp();

            m_wallpaperPackage.setPath(m_wallpaper);
            img = m_wallpaperPackage.filePath("preferred");
            m_wallpaperPath = m_wallpaperPackage.filePath("preferred");
            Q_EMIT wallpaperPathChanged();
        }
    }

    if (img.isEmpty()) {
        // ok, so the package we have failed to work out; let's try the default
        // if we have already
        const QString wallpaper = m_wallpaper;
        useSingleImageDefaults();
        if (wallpaper != m_wallpaper) {
            setSingleImage();
        }
    }
    qDebug()<<"Found wallpaper" << m_wallpaperPath;
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
    ///qDebug() << "droppage!" << url << url.isLocalFile();
    if (url.isLocalFile()) {
        const QString path = url.toLocalFile();
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
    } else {
        QString wallpaperPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("wallpapers/") + url.path();

        if (!wallpaperPath.isEmpty()) {
            KIO::FileCopyJob *job = KIO::file_copy(url, QUrl(wallpaperPath));
            if (setAsCurrent) {
                connect(job, SIGNAL(result(KJob*)), this, SLOT(setWallpaperRetrieved(KJob*)));
            } else {
                connect(job, SIGNAL(result(KJob*)), this, SLOT(addWallpaperRetrieved(KJob*)));
            }
        }
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
        connect(finder, SIGNAL(backgroundsFound(QStringList,QString)), this, SLOT(backgroundsFound(QStringList,QString)));
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
        QTimer::singleShot(1000, this, SLOT(startSlideshow()));
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
        connect(m_newStuffDialog.data(), SIGNAL(accepted()), SLOT(newStuffFinished()));
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
        m_dialog = new QFileDialog(0, i18n("Open Image"),
                                      QDir::homePath()+QStringLiteral("/Pictures"),
                                      i18n("Image Files (*.png *.jpg *.jpeg *.bmp *.svg *.svgz *.xcf)"));
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
        foreach (const QString &file, m_dialog->selectedFiles()) {
            addUsersWallpaper(m_dialog->selectedFiles()[0]); // FIXME
        }
        emit customWallpaperPicked();
    }
}

void Image::addUsersWallpaper(const QString &file)
{
    QString f = file;
    f.replace("file:/", "");
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
        if(fileInfo.isFile() && BackgroundFinder::suffixes().contains(fileInfo.suffix().toLower())) {
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
    // save it
    KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")),
                                                              QStringLiteral("Wallpapers"));
    m_usersWallpapers = cfg.readEntry("usersWallpapers", m_usersWallpapers);
    int wallpaperIndex = m_usersWallpapers.indexOf(name);
    if (wallpaperIndex >= 0){
        m_usersWallpapers.removeAt(wallpaperIndex);
        m_model->reload(m_usersWallpapers);
        cfg.writeEntry("usersWallpapers", m_usersWallpapers);
        cfg.sync();
        emit usersWallpapersChanged();
        Q_EMIT settingsChanged(true);
    }
}

#include "image.moc"
