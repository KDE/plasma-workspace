/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imageproxymodel.h"

#include <QDir>

#include <KConfigGroup>
#include <KIO/OpenFileManagerWindowJob>
#include <KSharedConfig>

#include "../finder/suffixcheck.h"
#include "imagelistmodel.h"
#include "packagelistmodel.h"

namespace
{
inline bool isChildItem(const QStringList &customPathsInKDirWatch, const QString &childPath)
{
    return std::any_of(customPathsInKDirWatch.cbegin(), customPathsInKDirWatch.cend(), [&childPath](const QString &customPath) {
        if (customPath.endsWith(QDir::separator())) {
            return childPath.startsWith(customPath);
        } else {
            return childPath.startsWith(customPath + QDir::separator());
        }
    });
}
}

ImageProxyModel::ImageProxyModel(const QStringList &customPaths, const QSize &targetSize, QObject *parent)
    : QConcatenateTablesProxyModel(parent)
    , m_imageModel(new ImageListModel(targetSize, this))
    , m_packageModel(new PackageListModel(targetSize, this))
{
    connect(this, &ImageProxyModel::rowsInserted, this, &ImageProxyModel::countChanged);
    connect(this, &ImageProxyModel::rowsRemoved, this, &ImageProxyModel::countChanged);
    connect(this, &ImageProxyModel::modelReset, this, &ImageProxyModel::countChanged);

    m_customPaths = customPaths;
    if (customPaths.empty()) {
        KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Wallpapers"));
        m_customPaths = cfg.readEntry("usersWallpapers", QStringList{});
        m_imageModel->m_removableWallpapers = m_customPaths;
        m_packageModel->m_removableWallpapers = m_customPaths;

        m_customPaths += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/"), QStandardPaths::LocateDirectory);
    }

    connect(m_imageModel, &AbstractImageListModel::loaded, this, &ImageProxyModel::slotHandleLoaded);
    connect(m_packageModel, &AbstractImageListModel::loaded, this, &ImageProxyModel::slotHandleLoaded);

    m_loaded = 0;
    Q_EMIT loadingChanged();

    m_imageModel->load(m_customPaths);
    m_packageModel->load(m_customPaths);
}

QHash<int, QByteArray> ImageProxyModel::roleNames() const
{
    const auto models = sourceModels();

    if (!models.empty()) {
        return models.at(0)->roleNames();
    }

    return QConcatenateTablesProxyModel::roleNames();
}

int ImageProxyModel::count() const
{
    return rowCount();
}

int ImageProxyModel::indexOf(const QString &packagePath) const
{
    int idx = -1;

    const auto models = sourceModels();

    for (const auto &m : models) {
        idx = static_cast<const AbstractImageListModel *>(m)->indexOf(packagePath);

        if (idx >= 0) {
            return mapFromSource(m->index(idx, 0)).row();
        }
    }

    return idx;
}

bool ImageProxyModel::loading() const
{
    return m_loaded != 2;
}

void ImageProxyModel::reload()
{
    const auto models = sourceModels();

    for (const auto &m : models) {
        static_cast<AbstractImageListModel *>(m)->reload();
    }

    m_loaded = 0;
    Q_EMIT loadingChanged();
}

QStringList ImageProxyModel::addBackground(const QString &_path)
{
    QString path = _path;

    if (path.startsWith(QLatin1String("file://"))) {
        path.remove(0, 7);
    }

    const QFileInfo info(path);

    QStringList results;

    if (info.isDir()) {
        if (!path.endsWith(QDir::separator())) {
            path += QDir::separator();
        }

        results = m_packageModel->addBackground(path);
    } else if (info.isFile()) {
        results = m_imageModel->addBackground(path);
    }

    if (!results.empty()) {
        m_pendingAddition.append(results);

        for (const QString &path : std::as_const(results)) {
            if (m_dirWatch.contains(path) || isChildItem(m_customPaths, path) /* KDirWatch already monitors the parent folder */) {
                continue;
            }

            const QFileInfo info(path);
            if (info.isFile()) {
                m_dirWatch.addFile(path);
            } else if (info.isDir()) {
                m_dirWatch.addDir(path);
            }
        }
    }

    return results;
}

void ImageProxyModel::removeBackground(const QString &_packagePath)
{
    QString packagePath = _packagePath;

    if (packagePath.startsWith(QLatin1String("file://"))) {
        packagePath.remove(0, 7);
    }

    QStringList results;

    // The file may be already deleted, so isFile/isDir won't work.
    if (const QFileInfo info(packagePath); isAcceptableSuffix(info.suffix())) {
        results = m_imageModel->removeBackground(packagePath);

        if (!results.empty() && !isChildItem(m_customPaths, results.at(0))) {
            // Don't remove the file if its parent folder is in KDirWatch, otherwise KDirWatchPrivate::removeEntry will also remove the parent folder
            m_dirWatch.removeFile(results.at(0));
        }
    } else {
        results = m_packageModel->removeBackground(packagePath);

        if (!results.empty()) {
            // Because of KDirWatch::WatchSubDirs, some folders will still be added to KDirWatch
            m_dirWatch.removeDir(results.at(0));
        }
    }

    // The user may add a wallpaper and delete it later.
    for (const QString &path : std::as_const(results)) {
        m_pendingAddition.removeOne(path);
    }
}

void ImageProxyModel::commitAddition()
{
    if (m_pendingAddition.empty()) {
        return;
    }

    KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Wallpapers"));
    QStringList list = cfg.readEntry("usersWallpapers", QStringList{});

    list += m_pendingAddition;
    list.removeDuplicates();

    cfg.writeEntry("usersWallpapers", list);

    m_pendingAddition.clear();
}

void ImageProxyModel::commitDeletion()
{
    QStringList pendingList;

    for (int row = 0; row < rowCount(); row++) {
        QModelIndex idx = index(row, 0);

        if (idx.data(PendingDeletionRole).toBool()) {
            pendingList.append(idx.data(PackageNameRole).toString());
        }
    }

    for (const QString &p : std::as_const(pendingList)) {
        removeBackground(p);
    }

    // Update the config
    KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Wallpapers"));
    const QStringList list = cfg.readEntry("usersWallpapers", QStringList{});
    QStringList updatedList;

    // Check if the file still exists
    std::copy_if(list.cbegin(), list.cend(), std::back_inserter(updatedList), [&pendingList](const QString &_p) {
        QString p = _p;
        if (p.startsWith(QLatin1String("file://"))) {
            p.remove(0, 7);
        }

        return !pendingList.contains(p) && QFileInfo(p).exists();
    });

    cfg.writeEntry("usersWallpapers", updatedList);
    cfg.sync();
}

void ImageProxyModel::openContainingFolder(int row) const
{
    KIO::highlightInFileManager({index(row, 0).data(PathRole).toUrl()});
}

void ImageProxyModel::slotHandleLoaded(AbstractImageListModel *model)
{
    disconnect(model, &AbstractImageListModel::loaded, this, 0);

    if (++m_loaded == 2) {
        // All models are loaded, now add them.
        addSourceModel(m_imageModel);
        addSourceModel(m_packageModel);

        connect(this, &ImageProxyModel::targetSizeChanged, m_imageModel, &AbstractImageListModel::slotTargetSizeChanged);
        connect(this, &ImageProxyModel::targetSizeChanged, m_packageModel, &AbstractImageListModel::slotTargetSizeChanged);
        setupDirWatch();

        Q_EMIT loadingChanged();
    }
}

void ImageProxyModel::slotDirWatchCreated(const QString &_path)
{
    QString path = _path;

    if (int idx = path.indexOf(QLatin1String("contents/images/")); idx > 0) {
        path = path.mid(0, idx);
    }

    addBackground(path);
}

void ImageProxyModel::slotDirWatchDeleted(const QString &path)
{
    removeBackground(path);
}

void ImageProxyModel::setupDirWatch()
{
    // Monitor file changes in the custom directories for the slideshow backend.
    for (const QString &path : std::as_const(m_customPaths)) {
        if (QFileInfo(path).isDir()) {
            m_dirWatch.addDir(path, KDirWatch::WatchFiles | KDirWatch::WatchSubDirs);
        }
    }

    connect(&m_dirWatch, &KDirWatch::created, this, &ImageProxyModel::slotDirWatchCreated);
    connect(&m_dirWatch, &KDirWatch::deleted, this, &ImageProxyModel::slotDirWatchDeleted);
}