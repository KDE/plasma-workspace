/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BACKGROUNDLISTMODEL_CPP
#define BACKGROUNDLISTMODEL_CPP

#include "backgroundlistmodel.h"
#include "debug.h"

#include <QCollator>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QImageReader>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QThreadPool>
#include <QUuid>
#include <QXmlStreamReader> // Handle wallpaper xml files

#include <KIO/PreviewJob>
#include <KLocalizedString>
#include <QDebug>
#include <kaboutdata.h>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <KIO/OpenFileManagerWindowJob>

#include <algorithm>

QStringList BackgroundFinder::s_suffixes;
QMutex BackgroundFinder::s_suffixMutex;

ImagePackage::ImagePackage(const QString &_path, const QString &_darkPath, const QString &_packagePath, const QString &_name, const QString &_author)
    : name(_name)
    , author(_author)
{
    const QFileInfo pathInfo(_path);
    if (pathInfo.isSymLink()) {
        path = pathInfo.symLinkTarget();
    } else {
        path = _path;
    }

    const QFileInfo darkPathInfo(_darkPath);
    if (darkPathInfo.isSymLink()) {
        darkPath = darkPathInfo.symLinkTarget();
    } else {
        darkPath = _darkPath;
    }

    const QFileInfo info(_packagePath);
    if (info.isSymLink()) {
        packagePath = info.symLinkTarget();
    } else {
        packagePath = _packagePath;
    }

    if (name.isEmpty()) {
        if (!path.isEmpty()) {
            name = QFileInfo(path).completeBaseName();
        } else if (!darkPath.isEmpty()) {
            name = QFileInfo(darkPath).completeBaseName();
        } else {
            name = i18n("Unknown");
        }
    }
}

QString packageTitle(const KPackage::Package &package)
{
    QString title = package.metadata().isValid() ? package.metadata().name() : QString();
    if (title.isEmpty()) {
        title = QFileInfo(package.filePath("preferred")).completeBaseName();
    }
    return title;
}

QString packageFirstAuthor(const KPackage::Package &package)
{
    if (!package.metadata().isValid()) {
        return QString();
    }

    const auto authors = package.metadata().authors();

    if (authors.size() > 0) {
        return authors.constFirst().name();
    }

    return QString();
}

ImageSizeFinder::ImageSizeFinder(const QString &path, QObject *parent)
    : QObject(parent)
    , m_path(path)
{
}

void ImageSizeFinder::run()
{
    QImageReader reader(m_path);
    Q_EMIT sizeFound(m_path, reader.size());
}

BackgroundListModel::BackgroundListModel(Image *wallpaper, QObject *parent)
    : QAbstractListModel(parent)
    , m_wallpaper(wallpaper)
{
    m_imageCache.setMaxCost(10 * 1024 * 1024); // 10 MiB

    connect(&m_dirwatch, &KDirWatch::deleted, this, &BackgroundListModel::removeBackground);

    // TODO: on Qt 4.4 use the ui scale factor
    QFontMetrics fm(QGuiApplication::font());
    m_screenshotSize = fm.horizontalAdvance('M') * 15;

    qRegisterMetaType<ImagePackageList>();
}

BackgroundListModel::~BackgroundListModel() = default;

QHash<int, QByteArray> BackgroundListModel::BackgroundListModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {Qt::DecorationRole, "decoration"},
        {AuthorRole, "author"},
        {ScreenshotRole, "screenshot"},
        {ResolutionRole, "resolution"},
        {PathRole, "path"},
        {DarkPathRole, "darkPath"},
        {PackageNameRole, "packageName"},
        {RemovableRole, "removable"},
        {PendingDeletionRole, "pendingDeletion"},
    };
}

void BackgroundListModel::removeBackground(const QString &path)
{
    int index = -1;
    while ((index = indexOf(path)) >= 0) {
        beginRemoveRows(QModelIndex(), index, index);
        m_packages.removeAt(index);
        endRemoveRows();
        Q_EMIT countChanged();
    }
}

void BackgroundListModel::reload()
{
    reload(QStringList());
}

void BackgroundListModel::reload(const QStringList &_selected)
{
    if (!m_wallpaper) {
        beginRemoveRows(QModelIndex(), 0, m_packages.count() - 1);
        m_packages.clear();
        endRemoveRows();
        Q_EMIT countChanged();
        return;
    }

    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/"), QStandardPaths::LocateDirectory);

    dirs += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("gnome-background-properties/"), QStandardPaths::LocateDirectory);

    BackgroundFinder *finder = new BackgroundFinder(m_wallpaper.data(), dirs);
    const auto token = finder->token();
    connect(finder, &BackgroundFinder::backgroundsFound, this, [this, _selected, token](const ImagePackageList &wallpapersFound) {
        if (token != m_findToken || !m_wallpaper) {
            return;
        }

        ImagePackageList selected;
        for (const auto &s : _selected) {
            if (QFileInfo(s).isFile()) {
                selected.append(ImagePackage(s));
            } else {
                selected << ImagePackage(QString(), QString(), s);
            }
        }

        processImagePackages(selected + wallpapersFound);
        m_removableWallpapers = QSet<QString>(_selected.cbegin(), _selected.cend());
    });
    m_findToken = token;
    finder->start();
}

void BackgroundListModel::processImagePackages(const ImagePackageList &packages)
{
    beginResetModel();
    m_packages.clear();

    m_packages.reserve(packages.count());
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    for (const ImagePackage &p : packages) {
        if (QFileInfo(p.path).isSymLink() || contains(p.path) || contains(p.packagePath)) {
            continue;
        }

        // Single wallpaper
        if ((p.packagePath.isEmpty() || p.packagePath.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) && !p.path.isEmpty()) {
            m_packages << p;
            continue;
        }

        // Wallpaper package (folder)
        if (QFile::exists(p.packagePath)) {
            package.setPath(p.packagePath);
            if (package.isValid()) {
                m_wallpaper->findPreferedImageInPackage(package);
                m_packages << ImagePackage(package.filePath("preferred"), QString(), p.packagePath, packageTitle(package), packageFirstAuthor(package));

                // add new files to dirwatch
                if (!m_dirwatch.contains(p.packagePath)) {
                    m_dirwatch.addDir(p.packagePath);
                }
            }
        }
    }

    if (!m_packages.isEmpty()) {
        QCollator collator;
        // Make sure 2 comes before 10
        collator.setNumericMode(true);
        // Behave like Dolphin with natural sorting enabled
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        const auto compare = [&collator](const ImagePackage &a, const ImagePackage &b) {
            // Checking if less than zero makes ascending order (A-Z)
            return collator.compare(a.name, b.name) < 0;
        };
        std::stable_sort(m_packages.begin(), m_packages.end(), compare);
    }
    endResetModel();
    Q_EMIT countChanged();
    // qCDebug(IMAGEWALLPAPER) << t.elapsed();
}

void BackgroundListModel::addBackground(const QString &path)
{
    if (!m_wallpaper || !contains(path)) {
        if (!m_dirwatch.contains(path)) {
            m_dirwatch.addFile(path);
        }
        beginInsertRows(QModelIndex(), 0, 0);
        m_removableWallpapers.insert(path);
        qCDebug(IMAGEWALLPAPER) << "Background added " << path;

        if (path.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
            // Parse xml wallpaper
            const ImagePackageList list = BackgroundFinder::parseXmlWallpaper(path);
            if (!list.isEmpty()) {
                m_packages.prepend(list.at(0));
            }
        } else {
            // Normal image
            m_packages.prepend(ImagePackage(path));
        }
        endInsertRows();
        Q_EMIT countChanged();
    }
}

int BackgroundListModel::indexOf(const QString &path) const
{
    if (path.isEmpty()) {
        return -1;
    }

    // remove eventual file:///
    const QString filteredPath = QUrl(path).path();

    const auto it = std::find_if(m_packages.cbegin(), m_packages.cend(), [&filteredPath](const ImagePackage &package) {
        // packages will end with a '/', but the path passed in may not
        QString packagePath = package.packagePath;
        if (packagePath.endsWith(QChar::fromLatin1('/'))) {
            packagePath.chop(1);
        }

        // For local files (user wallpapers) filteredPath == package.path
        // E.X. filteredPath = "/home/kde/next.png"
        // package.path = "/home/kde/next.png"
        //
        // But for the system wallpapers this is not the case. filteredPath == package.packagePath
        // E.X. filteredPath = /usr/share/wallpapers/Next/"
        // package.path = "/usr/share/wallpapers/Next/contents/images/1920x1080.png"
        // package.packagePath = "/usr/share/wallpapers/Next/"
        if (filteredPath == package.path || (!packagePath.isEmpty() && filteredPath.contains(packagePath))) {
            return true;
        }
        return false;
    });

    return it == m_packages.cend() ? -1 : static_cast<int>(std::distance(m_packages.cbegin(), it));
}

bool BackgroundListModel::contains(const QString &path) const
{
    // qCDebug(IMAGEWALLPAPER) << "WP contains: " << path << indexOf(path).isValid();
    return indexOf(path) >= 0;
}

int BackgroundListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_packages.size();
}

QSize BackgroundListModel::bestSize(const QString &path) const
{
    if (path.isEmpty()) {
        return QSize();
    }

    if (m_sizeCache.contains(path)) {
        return m_sizeCache.value(path);
    }

    ImageSizeFinder *finder = new ImageSizeFinder(path);
    connect(finder, &ImageSizeFinder::sizeFound, this, &BackgroundListModel::sizeFound);
    QThreadPool::globalInstance()->start(finder);

    QSize size(-1, -1);
    const_cast<BackgroundListModel *>(this)->m_sizeCache.insert(path, size);
    return size;
}

void BackgroundListModel::sizeFound(const QString &path, const QSize &s)
{
    if (!m_wallpaper) {
        return;
    }

    int idx = indexOf(path);
    if (idx >= 0) {
        m_sizeCache.insert(path, s);
        Q_EMIT dataChanged(index(idx, 0), index(idx, 0));
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

    const ImagePackage &p = m_packages.at(index.row());
    if (p.path.isEmpty()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole: {
        return p.name;
    }

    case ScreenshotRole: {
        const QString path = p.path;

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
            KIO::PreviewJob *job = KIO::filePreview(list, QSize(m_screenshotSize * 1.6, m_screenshotSize), &availablePlugins);
            job->setIgnoreMaximumSize(true);
            connect(job, &KIO::PreviewJob::gotPreview, this, &BackgroundListModel::showPreview);
            connect(job, &KIO::PreviewJob::failed, this, &BackgroundListModel::previewFailed);
            const_cast<BackgroundListModel *>(this)->m_previewJobsUrls.insert(persistentIndex, url);
        }

        return QVariant();
    }

    case AuthorRole:
        return p.author;

    case ResolutionRole: {
        const QSize size = bestSize(p.path);

        if (size.isValid()) {
            return QString::fromLatin1("%1x%2").arg(size.width()).arg(size.height());
        }

        return QString();
    }

    case PathRole:
        return p.path;

    case DarkPathRole:
        return p.darkPath;

    case PackageNameRole:
        return p.packagePath.isEmpty() ? p.path : p.packagePath;

    case RemovableRole: {
        QString localWallpapers = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/wallpapers/";
        return p.path.startsWith(localWallpapers) || m_removableWallpapers.contains(p.path) || m_removableWallpapers.contains(p.packagePath);
    }

    case PendingDeletionRole: {
        const QUrl wallpaperUrl = QUrl::fromLocalFile(p.path);
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
        const ImagePackage &p = m_packages.at(index.row());

        const QUrl wallpaperUrl = QUrl::fromLocalFile(p.path);
        m_pendingDeletion[wallpaperUrl.toLocalFile()] = value.toBool();

        Q_EMIT dataChanged(index, index);
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

    const int cost = preview.width() * preview.height() * preview.depth() / 8;
    m_imageCache.insert(p.path, new QPixmap(preview), cost);

    // qCDebug(IMAGEWALLPAPER) << "WP preview size:" << preview.size();
    Q_EMIT dataChanged(index, index);
}

void BackgroundListModel::previewFailed(const KFileItem &item)
{
    m_previewJobsUrls.remove(m_previewJobsUrls.key(item.url()));
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
    for (const ImagePackage &p : std::as_const(m_packages)) {
        const QUrl wallpaperUrl = QUrl::fromLocalFile(p.path);
        if (m_pendingDeletion.contains(wallpaperUrl.toLocalFile()) && m_pendingDeletion[wallpaperUrl.toLocalFile()]) {
            candidates << wallpaperUrl.toLocalFile();
        }
    }

    return candidates;
}

BackgroundFinder::BackgroundFinder(Image *wallpaper, const QStringList &paths)
    : QThread(wallpaper)
    , m_paths(paths)
    , m_token(QUuid::createUuid().toString())
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
        const auto supportedMimeTypes = QImageReader::supportedMimeTypes();
        for (const QByteArray &mimeType : supportedMimeTypes) {
            QMimeType mime(db.mimeTypeForName(mimeType));
            const QStringList globPatterns = mime.globPatterns();
            for (const QString &pattern : globPatterns) {
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

ImagePackageList BackgroundFinder::parseXmlWallpaper(const QString &xmlPath)
{
    ImagePackageList results;
    QFile file(xmlPath);

    if (!file.open(QIODevice::ReadOnly)) {
        return results;
    }

    QXmlStreamReader xml(&file);
    QString name, path, darkPath, author;
    bool finished = false;

    while (!xml.atEnd() && !finished) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QLatin1String("wallpaper")) {
            while (!xml.atEnd()) {
                xml.readNext();

                if (xml.isStartElement()) {
                    if (xml.name() == QLatin1String("name")) {
                        /* no pictures available for the specified parameters */
                        name = xml.readElementText();
                    } else if (xml.name() == QLatin1String("filename")) {
                        path = xml.readElementText();
                    } else if (xml.name() == QLatin1String("filename-dark")) {
                        darkPath = xml.readElementText();
                    } else if (xml.name() == QLatin1String("author")) {
                        author = xml.readElementText();
                    }
                } else if (xml.isEndElement() && xml.name() == QLatin1String("wallpaper")) {
                    // Read only one wallpaper
                    finished = true;
                    break;
                }
            }
        }
    }

    if (!isAcceptableSuffix(QFileInfo(path).suffix())) {
        // At least one light-mode wallpaper must be available.
        return results;
    }

    if (!isAcceptableSuffix(QFileInfo(darkPath).suffix())) {
        darkPath.clear();
    }

    // Store xml path in packagePath
    results << ImagePackage(path, darkPath, xmlPath, name, author);

    return results;
}

void BackgroundFinder::run()
{
    QElapsedTimer t;
    t.start();

    ImagePackageList packagesFound;
    ImagePackageList xmlImagesFound;
    ImagePackageList imagesFound;

    QDir dir;
    dir.setFilter(QDir::AllDirs | QDir::Files | QDir::Readable);
    QStringList _suffixes = suffixes();
    _suffixes << QStringLiteral("*.xml");
    dir.setNameFilters(_suffixes);
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    int i;
    for (i = 0; i < m_paths.count(); ++i) {
        const QString path = m_paths.at(i);
        dir.setPath(path);
        const QFileInfoList files = dir.entryInfoList();
        for (const QFileInfo &wp : files) {
            if (wp.isDir()) {
                // qCDebug(IMAGEWALLPAPER) << "scanning directory" << wp.fileName();

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
                            packagesFound.append(ImagePackage(QString(), QString(), package.path()));
                        }
                        // qCDebug(IMAGEWALLPAPER) << "adding package" << wp.filePath();
                        continue;
                    }
                }

                // add this to the directories we should be looking at
                m_paths.append(filePath);
            } else if (wp.suffix() == QLatin1String("xml")) {
                xmlImagesFound += parseXmlWallpaper(wp.absoluteFilePath());
            } else {
                // qCDebug(IMAGEWALLPAPER) << "adding image file" << wp.filePath();
                imagesFound.append(ImagePackage(wp.filePath()));
            }
        }
    }

    // Folder first
    Q_EMIT backgroundsFound(packagesFound + xmlImagesFound + imagesFound, m_token);
    deleteLater();
}

#endif // BACKGROUNDLISTMODEL_CPP
