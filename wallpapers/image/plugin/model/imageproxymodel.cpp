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

ImageProxyModel::ImageProxyModel(const QStringList &_customPaths, const QSize &targetSize, QObject *parent)
    : QConcatenateTablesProxyModel(parent)
    , m_imageModel(new ImageListModel(targetSize, this))
    , m_packageModel(new PackageListModel(targetSize, this))
{
    connect(this, &ImageProxyModel::rowsInserted, this, &ImageProxyModel::countChanged);
    connect(this, &ImageProxyModel::rowsRemoved, this, &ImageProxyModel::countChanged);
    connect(this, &ImageProxyModel::modelReset, this, &ImageProxyModel::countChanged);

    /**
     * Add files to KDirWatch.
     * Files or dirs should be already added to KDirWatch when
     * rowsInserted or rowsRemoved is emitted.
     */
    connect(m_imageModel, &QAbstractItemModel::modelAboutToBeReset, this, &ImageProxyModel::slotSourceModelAboutToBeReset);
    connect(m_packageModel, &QAbstractItemModel::modelAboutToBeReset, this, &ImageProxyModel::slotSourceModelAboutToBeReset);
    connect(m_imageModel, &QAbstractItemModel::modelReset, this, &ImageProxyModel::slotSourceModelReset);
    connect(m_packageModel, &QAbstractItemModel::modelReset, this, &ImageProxyModel::slotSourceModelReset);

    // Monitor file changes in the custom directories for the slideshow backend.
    QStringList customPaths = _customPaths;

    if (customPaths.empty()) {
        KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Wallpapers"));
        customPaths = cfg.readEntry("usersWallpapers", QStringList{});
        m_imageModel->m_removableWallpapers = customPaths;
        m_packageModel->m_removableWallpapers = customPaths;

        customPaths += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/"), QStandardPaths::LocateDirectory);
    }

    for (const QString &path : std::as_const(customPaths)) {
        if (QFileInfo(path).isDir()) {
            m_dirWatch.addDir(path, KDirWatch::WatchFiles | KDirWatch::WatchSubDirs);
        }
    }

    connect(&m_dirWatch, &KDirWatch::created, this, &ImageProxyModel::slotDirWatchCreated);
    connect(&m_dirWatch, &KDirWatch::deleted, this, &ImageProxyModel::slotDirWatchDeleted);

    connect(m_imageModel, &AbstractImageListModel::loaded, this, &ImageProxyModel::slotHandleLoaded);
    connect(m_packageModel, &AbstractImageListModel::loaded, this, &ImageProxyModel::slotHandleLoaded);

    m_loaded = 0;
    Q_EMIT loadingChanged();

    m_imageModel->load(customPaths);
    m_packageModel->load(customPaths);
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

        std::for_each(results.cbegin(), results.cend(), [this](const QString &path) {
            if (m_dirWatch.contains(path)) {
                return;
            }

            const QFileInfo info(path);

            if (info.isFile()) {
                m_dirWatch.addFile(path);
            } else if (info.isDir()) {
                m_dirWatch.addDir(path);
            }
        });
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
    if (isAcceptableSuffix(QFileInfo(packagePath).suffix())) {
        results = m_imageModel->removeBackground(packagePath);

        if (!results.empty()) {
            m_dirWatch.removeFile(results.at(0));
        }
    } else {
        results = m_packageModel->removeBackground(packagePath);

        if (!results.empty()) {
            m_dirWatch.removeFile(results.at(0));
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

        Q_EMIT loadingChanged();
    }
}

void ImageProxyModel::slotSourceModelAboutToBeReset()
{
    AbstractImageListModel *model = qobject_cast<AbstractImageListModel *>(this->sender());

    if (!model) {
        return;
    }

    // Delete all items in KDirWatch
    for (int i = 0; i < model->rowCount(); i++) {
        const QString packageName = model->index(i, 0).data(ImageRoles::PackageNameRole).toString();
        const QFileInfo info(packageName);

        if (info.isFile()) {
            m_dirWatch.removeFile(packageName);
        } else if (info.isDir()) {
            m_dirWatch.removeDir(packageName);
        }
    }
}

void ImageProxyModel::slotSourceModelReset()
{
    AbstractImageListModel *model = qobject_cast<AbstractImageListModel *>(this->sender());

    if (!model) {
        return;
    }

    // Add all items to KDirWatch
    for (int i = 0; i < model->rowCount(); i++) {
        const QString packageName = model->index(i, 0).data(ImageRoles::PackageNameRole).toString();
        const QFileInfo info(packageName);

        if (info.isFile()) {
            m_dirWatch.addFile(packageName);
        } else if (info.isDir()) {
            m_dirWatch.addDir(packageName);
        }
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
