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

#include "backgroundlistmodel.h"

#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QThreadPool>
#include <QUuid>
#include <QGuiApplication>
#include <QFontMetrics>

#include <QDebug>
#include <KIO/PreviewJob>
#include <KLocalizedString>
#include <kimagecache.h>
#include <kaboutdata.h>

#include <KPackage/Package>
#include <KPackage/PackageStructure>
#include <KPackage/PackageLoader>

#include "image.h"

QSet<QString> BackgroundFinder::m_suffixes;

ImageSizeFinder::ImageSizeFinder(const QString &path, QObject *parent)
    : QObject(parent),
      m_path(path)
{
}

void ImageSizeFinder::run()
{
    QImage image(m_path);
    Q_EMIT sizeFound(m_path, image.size());
}


BackgroundListModel::BackgroundListModel(Image *wallpaper, QObject *parent)
    : QAbstractListModel(parent),
      m_wallpaper(wallpaper)
{
    connect(&m_dirwatch, SIGNAL(deleted(QString)), this, SLOT(removeBackground(QString)));

    QHash<int, QByteArray>roleNames;
    roleNames[Qt::DisplayRole] = "display";
    roleNames[Qt::DecorationRole] = "decoration";
    roleNames[AuthorRole] = "author";
    roleNames[ScreenshotRole] = "screenshot";
    roleNames[ResolutionRole] = "resolution";
    roleNames[PathRole] = "path";
    roleNames[PackageNameRole] = "packageName";
    roleNames[RemovableRole] = "removable";
    setRoleNames(roleNames);

    //TODO: on Qt 4.4 use the ui scale factor
    QFontMetrics fm(QGuiApplication::font());
    m_screenshotSize = fm.width('M') * 15;

    m_imageCache = new KImageCache("plasma_wallpaper_preview", 10485760);
}

BackgroundListModel::~BackgroundListModel()
{
    delete m_imageCache;
}

void BackgroundListModel::removeBackground(const QString &path)
{
    QModelIndex index;
    while ((index = indexOf(path)).isValid()) {
        beginRemoveRows(QModelIndex(), index.row(), index.row());
        m_packages.removeAt(index.row());
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
    if (!m_packages.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_packages.count() - 1);
        m_packages.clear();
        endRemoveRows();
        emit countChanged();
    }

    if (!m_wallpaper) {
        return;
    }

    if (!selected.isEmpty()) {
        qDebug() << "selected" << selected;
        processPaths(selected);
    }

    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "wallpapers/", QStandardPaths::LocateDirectory);
    qDebug() << " WP : -------" << dirs;

    BackgroundFinder *finder = new BackgroundFinder(m_wallpaper.data(), dirs);
    connect(finder, SIGNAL(backgroundsFound(QStringList,QString)), this, SLOT(backgroundsFound(QStringList,QString)));
    m_findToken = finder->token();
    finder->start();
    m_removableWallpapers = QSet<QString>::fromList(selected);
}

void BackgroundListModel::backgroundsFound(const QStringList &paths, const QString &token)
{
    if (token == m_findToken) {
        processPaths(paths);
    }
}

void BackgroundListModel::processPaths(const QStringList &paths)
{
    if (!m_wallpaper) {
        return;
    }

    QList<KPackage::Package> newPackages;
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
        int contentsIndex = file.indexOf(QStringLiteral("contents"));

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
            KPackage::Package package = KPackage::PackageLoader::self()->loadPackage("Wallpaper/Images");
            package.setPath(file);
            if (package.isValid()) {
                m_wallpaper->findPreferedImageInPackage(package);
                newPackages << package;
            }
        }
    }

    // add new files to dirwatch
    Q_FOREACH (KPackage::Package b, newPackages) {
        if (!m_dirwatch.contains(b.path())) {
            m_dirwatch.addFile(b.path());
        }
    }

    if (!newPackages.isEmpty()) {
        const int start = rowCount();
        beginInsertRows(QModelIndex(), start, start + newPackages.size() - 1);
        m_packages.append(newPackages);
        endInsertRows();
        emit countChanged();
    }
    //qDebug() << t.elapsed();
}

void BackgroundListModel::addBackground(const QString& path)
{
    if (!m_wallpaper || !contains(path)) {
        if (!m_dirwatch.contains(path)) {
            m_dirwatch.addFile(path);
        }
        beginInsertRows(QModelIndex(), 0, 0);
        KPackage::Package package = KPackage::PackageLoader::self()->loadPackage("Wallpaper/Images");

        m_removableWallpapers.insert(path);
        package.setPath(path);
        m_wallpaper->findPreferedImageInPackage(package);
        qDebug() << "WP Bckground added " << path << package.isValid();
        m_packages.prepend(package);
        endInsertRows();
        emit countChanged();
    }
}

QModelIndex BackgroundListModel::indexOf(const QString &path) const
{
    for (int i = 0; i < m_packages.size(); i++) {
        // packages will end with a '/', but the path passed in may not
        QString package = m_packages[i].path();
        if (package.at(package.length() - 1) == QChar::fromLatin1('/')) {
            package.truncate(package.length() - 1);
        }

        if (path.startsWith(package)) {
            // FIXME: ugly hack to make a difference between local files in the same dir
            // package->path does not contain the actual file name
            qDebug() << "WP prefix" << m_packages[i].contentsPrefixPaths() << m_packages[i].filePath("preferred") << package << path;
            QStringList ps = m_packages[i].contentsPrefixPaths();
            bool prefixempty = ps.count() == 0;
            if (!prefixempty) {
                prefixempty = ps[0].isEmpty();
            }
//             if ((!m_packages[i].contentsPrefixPaths().isEmpty()) ||
//                 (path == m_packages[i].filePath("preferred"))) {
            if ((path == m_packages[i].filePath("preferred"))) {
                qDebug() << "WP TRUE" << (!m_packages[i].contentsPrefixPaths().isEmpty()) << (path == m_packages[i].filePath("preferred"));
                return index(i, 0);
            }
        }
    }
    return QModelIndex();
}

bool BackgroundListModel::contains(const QString &path) const
{
    //qDebug() << "WP contains: " << path << indexOf(path).isValid();
    return indexOf(path).isValid();
}

int BackgroundListModel::rowCount(const QModelIndex &) const
{
    return m_packages.size();
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
    connect(finder, SIGNAL(sizeFound(QString,QSize)), this,
            SLOT(sizeFound(QString,QSize)));
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

    QModelIndex index = indexOf(path);
    if (index.isValid()) {
        KPackage::Package package = m_packages.at(index.row());
        m_sizeCache.insert(package.path(), s);
        emit dataChanged(index, index);
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
    break;

    case ScreenshotRole: {
        QPixmap preview = QPixmap(QSize(m_screenshotSize*1.6,
                                                    m_screenshotSize));
        if (m_imageCache->findPixmap(b.filePath("preferred"), &preview)) {
            return preview;
        }
//         qDebug() << "WP preferred: " << b.filePath("preferred");
//         qDebug() << "WP screenshot: " << b.filePath("screenshot");
        QUrl file = QUrl::fromLocalFile(b.filePath("preferred"));
        if (!m_previewJobs.contains(file) && file.isValid()) {

            KFileItemList list;
            list.append(KFileItem(file, QString(), 0));
            KIO::PreviewJob* job = KIO::filePreview(list,
                                                    QSize(m_screenshotSize*1.6,
                                                    m_screenshotSize));
            job->setIgnoreMaximumSize(true);
            connect(job, SIGNAL(gotPreview(KFileItem,QPixmap)),
                    this, SLOT(showPreview(KFileItem,QPixmap)));
            connect(job, SIGNAL(failed(KFileItem)),
                    this, SLOT(previewFailed(KFileItem)));
            const_cast<BackgroundListModel *>(this)->m_previewJobs.insert(file, QPersistentModelIndex(index));
        }

        return QVariant();
    }
    break;

    case AuthorRole:
        if (b.metadata().isValid() && !b.metadata().authors().isEmpty()) {
            return b.metadata().authors().first().name();
        } else {
            return i18nc("Unknown Author", "Unknown");
        }
    break;

    case ResolutionRole:{
        QSize size = bestSize(b);

        if (size.isValid()) {
            return QString::fromLatin1("%1x%2").arg(size.width()).arg(size.height());
        }

        return QString();
    }
    break;

    case PathRole:
        return QUrl::fromLocalFile(b.filePath("preferred"));
    break;

    case PackageNameRole:
        return !b.metadata().isValid() || b.metadata().pluginId().isEmpty() ? b.filePath("preferred") : b.metadata().pluginId();
    break;

    case RemovableRole: {
        QString localWallpapers = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/wallpapers/";
        QString path = b.filePath("preferred");
        return path.startsWith(localWallpapers) || m_removableWallpapers.contains(path);
    }
    break;

    default:
        return QVariant();
    break;
    }
    return QVariant();
}

void BackgroundListModel::showPreview(const KFileItem &item, const QPixmap &preview)
{
    if (!m_wallpaper) {
        return;
    }

    QPersistentModelIndex index = m_previewJobs.value(item.url());
    m_previewJobs.remove(item.url());

    if (!index.isValid()) {
        return;
    }

    KPackage::Package b = package(index.row());
    if (!b.isValid()) {
        return;
    }

    m_imageCache->insertPixmap(b.filePath("preferred"), preview);
    //qDebug() << "WP preview size:" << preview.size();
    emit dataChanged(index, index);
}

void BackgroundListModel::previewFailed(const KFileItem &item)
{
    m_previewJobs.remove(item.url());
}

KPackage::Package BackgroundListModel::package(int index) const
{
    return m_packages.at(index);
}

BackgroundFinder::BackgroundFinder(Image *wallpaper, const QStringList &paths)
    : QThread(wallpaper),
      m_paths(paths),
      m_token(QUuid().toString())
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

const QSet<QString> &BackgroundFinder::suffixes()
{
    if(m_suffixes.isEmpty()) {
        m_suffixes << QString::fromLatin1("png") << QString::fromLatin1("jpeg") << QString::fromLatin1("jpg") << QString::fromLatin1("svg") << QString::fromLatin1("svgz");
    }

    return m_suffixes;
}

void BackgroundFinder::run()
{
    QTime t;
    t.start();
    const QSet<QString> &fileSuffixes = suffixes();

    QStringList papersFound;

    QDir dir;
    dir.setFilter(QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::Readable);
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage("Wallpaper/Images");

    int i;
    for (i = 0; i < m_paths.count(); ++i) {
        const QString path = m_paths.at(i);
        dir.setPath(path);
        const QFileInfoList files = dir.entryInfoList();
        Q_FOREACH (const QFileInfo &wp, files) {
            if (wp.isDir()) {
                //qDebug() << "scanning directory" << wp.fileName();

                const QString name = wp.fileName();
                if (name == QString::fromLatin1(".") || name == QString::fromLatin1("..")) {
                    // do nothing
                    continue;
                }

                const QString filePath = wp.filePath();
                if (QFile::exists(filePath + QString::fromLatin1("/metadata.desktop"))) {
                    package.setPath(filePath);
                    if (package.isValid()) {
                        if (!package.filePath("images").isEmpty()) {
                            papersFound << package.path();
                        }
                        //qDebug() << "adding package" << wp.filePath();
                        continue;
                    }
                }

                // add this to the directories we should be looking at
                m_paths.append(filePath);
            } else if (fileSuffixes.contains(wp.suffix().toLower())) {
                //qDebug() << "adding image file" << wp.filePath();
                papersFound << wp.filePath();
            }
        }
    }

    //qDebug() << "WP background found!" << papersFound.size() << "in" << i << "dirs, taking" << t.elapsed() << "ms";
    Q_EMIT backgroundsFound(papersFound, m_token);
    deleteLater();
}

#include "backgroundlistmodel.moc"


#endif // BACKGROUNDLISTMODEL_CPP
