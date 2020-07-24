/***************************************************************************
 *   Copyright 2007 Paolo Capriotti <p.capriotti@gmail.com>                *
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

#ifndef BACKGROUNDLISTMODEL_CPP
#define BACKGROUNDLISTMODEL_CPP

#include "debug.h"
#include "backgroundlistmodel.h"

#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QThreadPool>
#include <QUuid>
#include <QGuiApplication>
#include <QFontMetrics>
#include <QImageReader>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMutex>
#include <QMutexLocker>
#include <QElapsedTimer>

#include <QDebug>
#include <KIO/PreviewJob>
#include <KLocalizedString>
#include <kaboutdata.h>

#include <KPackage/Package>
#include <KPackage/PackageStructure>
#include <KPackage/PackageLoader>

#include <KIO/OpenFileManagerWindowJob>

#include "image.h"

QStringList BackgroundFinder::s_suffixes;
QMutex BackgroundFinder::s_suffixMutex;

ImageSizeFinder::ImageSizeFinder(const QString &path, QObject *parent)
    : QObject(parent),
      m_path(path)
{
}

void ImageSizeFinder::run()
{
    QImageReader reader(m_path);
    Q_EMIT sizeFound(m_path, reader.size());
}


BackgroundListModel::BackgroundListModel(Image *wallpaper, QObject *parent)
    : QAbstractListModel(parent),
      m_wallpaper(wallpaper)
{
    m_imageCache.setMaxCost(10 * 1024 * 1024); // 10 MiB

    connect(&m_dirwatch, &KDirWatch::deleted, this, &BackgroundListModel::removeBackground);

    //TODO: on Qt 4.4 use the ui scale factor
    QFontMetrics fm(QGuiApplication::font());
    m_screenshotSize = fm.horizontalAdvance('M') * 15;
}

BackgroundListModel::~BackgroundListModel() = default;

QHash<int, QByteArray> BackgroundListModel::BackgroundListModel::roleNames() const
{
    return {
    { Qt::DisplayRole, "display" },
    { Qt::DecorationRole, "decoration" },
    { AuthorRole, "author" },
    { ScreenshotRole, "screenshot" },
    { ResolutionRole, "resolution" },
    { PathRole, "path" },
    { PackageNameRole, "packageName" },
    { RemovableRole, "removable" },
    { PendingDeletionRole, "pendingDeletion" },
    };
}

void BackgroundListModel::removeBackground(const QString &path)
{
    int index = -1;
    while ((index = indexOf(path)) >= 0) {
        beginRemoveRows(QModelIndex(), index, index);
        m_packages.removeAt(index);
        endRemoveRows();
        emit countChanged();
    }
}

void BackgroundListModel::reload()
{
    reload(QStringList());
}

void BackgroundListModel::reload(const QStringList &selected)
{
    if (!m_wallpaper) {
        beginRemoveRows(QModelIndex(), 0, m_packages.count() - 1);
        m_packages.clear();
        endRemoveRows();
        emit countChanged();
        return;
    }

    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/"), QStandardPaths::LocateDirectory);

    BackgroundFinder *finder = new BackgroundFinder(m_wallpaper.data(), dirs);
    const auto token = finder->token();
    connect(finder, &BackgroundFinder::backgroundsFound, this, [this, selected, token] (const QStringList &wallpapersFound) {
        if (token != m_findToken || !m_wallpaper) {
            return;
        }

        processPaths(selected + wallpapersFound);
        m_removableWallpapers = QSet<QString>(selected.constBegin(), selected.constEnd());
    });
    m_findToken = token;
    finder->start();
}

void BackgroundListModel::processPaths(const QStringList &paths)
{
    beginResetModel();
    m_packages.clear();

    QList<KPackage::Package> newPackages;
    newPackages.reserve(paths.count());
    Q_FOREACH (QString file, paths) {
        // check if the path is a symlink and if it is,
        // work with the target rather than the symlink
        QFileInfo info(file);
        if (info.isSymLink()) {
            file = info.symLinkTarget();
        }
        // now check if the path contains "contents" part
        // which could indicate that the file is part of some other
        // package (could have been symlinked) and we should work
        // with the package (which can already be present) rather
        // than just one file from it
        int contentsIndex = file.indexOf(QLatin1String("contents"));

        // FIXME: additionally check for metadata.desktop being present
        //        which would confirm a package but might be slowing things
        if (contentsIndex != -1) {
            file.truncate(contentsIndex);
        }

        // so now we have a path to a package, check if we're not
        // processing the same path twice (this is different from
        // the "!contains(file)" call lower down, that one checks paths
        // already in the model and does not include the paths
        // that are being checked in here); we want to check for duplicates
        // if and only if we actually changed the path (so the conditions from above
        // are reused here as that means we did change the path)
        if ((info.isSymLink() || contentsIndex != -1) && paths.contains(file)) {
            continue;
        }

        if (!contains(file) && QFile::exists(file)) {
            KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));
            package.setPath(file);
            if (package.isValid()) {
                m_wallpaper->findPreferedImageInPackage(package);
                newPackages << package;
            }
        }
    }

    // add new files to dirwatch
    Q_FOREACH (const KPackage::Package &b, newPackages) {
        if (!m_dirwatch.contains(b.path())) {
            m_dirwatch.addFile(b.path());
        }
    }

    if (!newPackages.isEmpty()) {
        m_packages.append(newPackages);
    }
    endResetModel();
    emit countChanged();
    //qCDebug(IMAGEWALLPAPER) << t.elapsed();
}

void BackgroundListModel::addBackground(const QString& path)
{
    if (!m_wallpaper || !contains(path)) {
        if (!m_dirwatch.contains(path)) {
            m_dirwatch.addFile(path);
        }
        beginInsertRows(QModelIndex(), 0, 0);
        KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

        m_removableWallpapers.insert(path);
        package.setPath(path);
        m_wallpaper->findPreferedImageInPackage(package);
        qCDebug(IMAGEWALLPAPER) << "Background added " << path << package.isValid();
        m_packages.prepend(package);
        endInsertRows();
        emit countChanged();
    }
}

int BackgroundListModel::indexOf(const QString &path) const
{
    for (int i = 0; i < m_packages.size(); i++) {
        // packages will end with a '/', but the path passed in may not
        QString package = m_packages[i].path();
        if (package.endsWith(QChar::fromLatin1('/'))) {
            package.chop(1);
        }
        //remove eventual file:///
        const QString filteredPath = QUrl(path).path();

        if (filteredPath.startsWith(package)) {
            // FIXME: ugly hack to make a difference between local files in the same dir
            // package->path does not contain the actual file name
            qCDebug(IMAGEWALLPAPER) << "prefix" << m_packages[i].contentsPrefixPaths() << m_packages[i].filePath("preferred") << package << filteredPath;
            QStringList ps = m_packages[i].contentsPrefixPaths();
            bool prefixempty = ps.count() == 0;
            if (!prefixempty) {
                prefixempty = ps[0].isEmpty();
            }

            //For local files (user wallpapers) filteredPath == m_packages[i].filePath("preferred")
            //E.X. filteredPath = "/home/kde/next.png"
            //m_packages[i].filePath("preferred") = "/home/kde/next.png"
            //
            //But for the system wallpapers this is not the case. filteredPath != m_packages[i].filePath("preferred")
            //E.X. filteredPath = /usr/share/wallpapers/Next/"
            //m_packages[i].filePath("preferred") = "/usr/share/wallpapers/Next/contents/images/1920x1080.png"
            if ((filteredPath == m_packages[i].filePath("preferred")) || m_packages[i].filePath("preferred").contains(filteredPath)) {
                return i;
            }
        }
    }
    return -1;
}

bool BackgroundListModel::contains(const QString &path) const
{
    //qCDebug(IMAGEWALLPAPER) << "WP contains: " << path << indexOf(path).isValid();
    return indexOf(path) >= 0;
}

int BackgroundListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_packages.size();
}

QSize BackgroundListModel::bestSize(const KPackage::Package &package) const
{
    if (m_sizeCache.contains(package.path())) {
        return m_sizeCache.value(package.path());
    }

    const QString image = package.filePath("preferred");
    if (image.isEmpty()) {
        return QSize();
    }

    ImageSizeFinder *finder = new ImageSizeFinder(image);
    connect(finder, &ImageSizeFinder::sizeFound, this,
            &BackgroundListModel::sizeFound);
    QThreadPool::globalInstance()->start(finder);

    QSize size(-1, -1);
    const_cast<BackgroundListModel *>(this)->m_sizeCache.insert(package.path(), size);
    return size;
}

void BackgroundListModel::sizeFound(const QString &path, const QSize &s)
{
    if (!m_wallpaper) {
        return;
    }

    int idx = indexOf(path);
    if (idx >= 0) {
        KPackage::Package package = m_packages.at(idx);
        m_sizeCache.insert(package.path(), s);
        emit dataChanged(index(idx, 0), index(idx, 0));
    }
}

QVariant BackgroundListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= m_packages.size()) {
        return QVariant();
    }

    KPackage::Package b = package(index.row());
    if (!b.isValid()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole: {
        QString title = b.metadata().isValid() ? b.metadata().name() : QString();

        if (title.isEmpty()) {
            return QFileInfo(b.filePath("preferred")).completeBaseName();
        }

        return title;
    }

    case ScreenshotRole: {
        const QString path = b.filePath("preferred");

        QPixmap *cachedPreview = m_imageCache.object(path);
        if (cachedPreview) {
            return *cachedPreview;
        }

        const QUrl url = QUrl::fromLocalFile(path);
        const QPersistentModelIndex persistentIndex(index);
        if (!m_previewJobsUrls.contains(persistentIndex) && url.isValid()) {
            KFileItemList list;
            list.append(KFileItem(url, QString(), 0));
            QStringList availablePlugins = KIO::PreviewJob::availablePlugins();
            KIO::PreviewJob* job = KIO::filePreview(list,
                                                    QSize(m_screenshotSize*1.6,
                                                    m_screenshotSize),  &availablePlugins);
            job->setIgnoreMaximumSize(true);
            connect(job, &KIO::PreviewJob::gotPreview,
                    this, &BackgroundListModel::showPreview);
            connect(job, &KIO::PreviewJob::failed,
                    this, &BackgroundListModel::previewFailed);
            const_cast<BackgroundListModel *>(this)->m_previewJobsUrls.insert(persistentIndex, url);
        }

        return QVariant();
    }

    case AuthorRole:
        if (b.metadata().isValid() && !b.metadata().authors().isEmpty()) {
            return b.metadata().authors().first().name();
        } else {
            return QString();
        }

    case ResolutionRole:{
        QSize size = bestSize(b);

        if (size.isValid()) {
            return QString::fromLatin1("%1x%2").arg(size.width()).arg(size.height());
        }

        return QString();
    }

    case PathRole:
        return QUrl::fromLocalFile(b.filePath("preferred"));

    case PackageNameRole:
        return !b.metadata().isValid() ? b.filePath("preferred") : b.path();

    case RemovableRole: {
        QString localWallpapers = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/wallpapers/";
        QString path = b.filePath("preferred");
        return path.startsWith(localWallpapers) || m_removableWallpapers.contains(path);
    }

    case PendingDeletionRole: {
        QUrl wallpaperUrl = QUrl::fromLocalFile(b.filePath("preferred"));
        return m_pendingDeletion.contains(wallpaperUrl.toLocalFile()) ? m_pendingDeletion[wallpaperUrl.toLocalFile()] : false;
    }

    default:
        return QVariant();
    }

    Q_UNREACHABLE();
}

bool BackgroundListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role == PendingDeletionRole) {
        KPackage::Package b = package(index.row());
        if (!b.isValid()) {
            return false;
        }

        const QUrl wallpaperUrl = QUrl::fromLocalFile(b.filePath("preferred"));
        m_pendingDeletion[wallpaperUrl.toLocalFile()] = value.toBool();

        emit dataChanged(index, index);
        return true;
    }

    return false;
}

void BackgroundListModel::showPreview(const KFileItem &item, const QPixmap &preview)
{
    if (!m_wallpaper) {
        return;
    }

    QPersistentModelIndex index = m_previewJobsUrls.key(item.url());
    m_previewJobsUrls.remove(index);

    if (!index.isValid()) {
        return;
    }

    KPackage::Package b = package(index.row());
    if (!b.isValid()) {
        return;
    }

    const int cost = preview.width() * preview.height() * preview.depth() / 8;
    m_imageCache.insert(b.filePath("preferred"), new QPixmap(preview), cost);

    //qCDebug(IMAGEWALLPAPER) << "WP preview size:" << preview.size();
    emit dataChanged(index, index);
}

void BackgroundListModel::previewFailed(const KFileItem &item)
{
    m_previewJobsUrls.remove(m_previewJobsUrls.key(item.url()));
}

KPackage::Package BackgroundListModel::package(int index) const
{
    return m_packages.at(index);
}

void BackgroundListModel::openContainingFolder(int rowIndex)
{
    KIO::highlightInFileManager({index(rowIndex, 0).data(PathRole).toUrl()});
}

void BackgroundListModel::setPendingDeletion(int rowIndex, bool pendingDeletion)
{
    setData(index(rowIndex, 0), pendingDeletion, PendingDeletionRole);
}

const QStringList BackgroundListModel::wallpapersAwaitingDeletion()
{
    QStringList candidates;
    for (const KPackage::Package &b : m_packages) {
        const QUrl wallpaperUrl = QUrl::fromLocalFile(b.filePath("preferred"));
        if (m_pendingDeletion.contains(wallpaperUrl.toLocalFile()) && m_pendingDeletion[wallpaperUrl.toLocalFile()]) {
            candidates << wallpaperUrl.toLocalFile();
        }
    }

    return candidates;
}

BackgroundFinder::BackgroundFinder(Image *wallpaper, const QStringList &paths)
    : QThread(wallpaper),
      m_paths(paths),
      m_token(QUuid::createUuid().toString())
{
}

BackgroundFinder::~BackgroundFinder()
{
    wait();
}

QString BackgroundFinder::token() const
{
    return m_token;
}

QStringList BackgroundFinder::suffixes()
{
    QMutexLocker lock(&s_suffixMutex);
    if (s_suffixes.isEmpty()) {
        QSet<QString> suffixes;

        QMimeDatabase db;
        Q_FOREACH (const QByteArray &mimeType, QImageReader::supportedMimeTypes()) {
            QMimeType mime(db.mimeTypeForName(mimeType));
            Q_FOREACH (const QString &pattern, mime.globPatterns()) {
                suffixes.insert(pattern);
            }
        }

        s_suffixes = suffixes.values();
    }

    return s_suffixes;
}

bool BackgroundFinder::isAcceptableSuffix(const QString &suffix)
{
    // Despite its name, suffixes() returns a list of glob patterns.
    // Therefore the file suffix check needs to include the "*." prefix.
    const QStringList &globPatterns = suffixes();
    return globPatterns.contains(QLatin1String("*.") + suffix.toLower());
}

void BackgroundFinder::run()
{
    QElapsedTimer t;
    t.start();

    QStringList papersFound;

    QDir dir;
    dir.setFilter(QDir::AllDirs | QDir::Files | QDir::Readable);
    dir.setNameFilters(suffixes());
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    int i;
    for (i = 0; i < m_paths.count(); ++i) {
        const QString path = m_paths.at(i);
        dir.setPath(path);
        const QFileInfoList files = dir.entryInfoList();
        Q_FOREACH (const QFileInfo &wp, files) {
            if (wp.isDir()) {
                //qCDebug(IMAGEWALLPAPER) << "scanning directory" << wp.fileName();

                const QString name = wp.fileName();
                if (name == QString::fromLatin1(".") || name == QString::fromLatin1("..")) {
                    // do nothing
                    continue;
                }

                const QString filePath = wp.filePath();
                if (QFile::exists(filePath + QString::fromLatin1("/metadata.desktop")) || QFile::exists(filePath + QString::fromLatin1("/metadata.json"))) {
                    package.setPath(filePath);
                    if (package.isValid()) {
                        if (!package.filePath("images").isEmpty()) {
                            papersFound << package.path();
                        }
                        //qCDebug(IMAGEWALLPAPER) << "adding package" << wp.filePath();
                        continue;
                    }
                }

                // add this to the directories we should be looking at
                m_paths.append(filePath);
            } else {
                //qCDebug(IMAGEWALLPAPER) << "adding image file" << wp.filePath();
                papersFound << wp.filePath();
            }
        }
    }

    //qCDebug(IMAGEWALLPAPER) << "WP background found!" << papersFound.size() << "in" << i << "dirs, taking" << t.elapsed() << "ms";
    Q_EMIT backgroundsFound(papersFound, m_token);
    deleteLater();
}



#endif // BACKGROUNDLISTMODEL_CPP
