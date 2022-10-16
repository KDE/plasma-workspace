/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imagelistmodel.h"

#include <QFileInfo>
#include <QPixmap>
#include <QStandardPaths>
#include <QThreadPool>
#include <QUrl>

#include <KIO/PreviewJob>

#include "../finder/imagefinder.h"
#include "../finder/suffixcheck.h"

ImageListModel::ImageListModel(const QSize &targetSize, QObject *parent)
    : AbstractImageListModel(targetSize, parent)
{
}

int ImageListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

QVariant ImageListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const int row = index.row();

    switch (role) {
    case Qt::DisplayRole: {
        const QString *const title = m_backgroundTitleCache.object(m_data.at(row));

        if (title) {
            return title->isEmpty() ? QFileInfo(m_data.at(row)).completeBaseName() : *title;
        }

        asyncGetMediaMetadata(m_data.at(row), QPersistentModelIndex(index));

        return QFileInfo(m_data.at(row)).completeBaseName();
    }

    case ScreenshotRole: {
        QPixmap *cachedPreview = m_imageCache.object({m_data.at(row)});

        if (cachedPreview) {
            return *cachedPreview;
        }

        asyncGetPreview({m_data.at(row)}, QPersistentModelIndex(index));

        return QVariant();
    }

    case AuthorRole: {
        const QString *const author = m_backgroundAuthorCache.object(m_data.at(row));

        if (author) {
            return *author;
        }

        asyncGetMediaMetadata(m_data.at(row), QPersistentModelIndex(index));

        return QString();
    }

    case ResolutionRole: {
        QSize *size = m_imageSizeCache.object(m_data.at(row));

        if (size && size->isValid()) {
            return QStringLiteral("%1x%2").arg(size->width()).arg(size->height());
        }

        asyncGetMediaMetadata(m_data.at(row), QPersistentModelIndex(index));

        return QString();
    }

    case PathRole:
        return QUrl::fromLocalFile(m_data.at(row));

    case PackageNameRole:
        return m_data.at(row);

    case RemovableRole: {
        const QString &path = m_data.at(row);

        return path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/"))
            || m_removableWallpapers.contains(path);
    }

    case PendingDeletionRole:
        return m_pendingDeletion.value(m_data.at(row), false);
    }
    Q_UNREACHABLE();
}

bool ImageListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role == PendingDeletionRole) {
        m_pendingDeletion[m_data.at(index.row())] = value.toBool();

        Q_EMIT dataChanged(index, index, {PendingDeletionRole});
        return true;
    }

    return false;
}

int ImageListModel::indexOf(const QString &_path) const
{
    QString path = _path;

    if (path.startsWith(QLatin1String("file://"))) {
        path.remove(0, 7);
    }

    const auto it = std::find_if(m_data.cbegin(), m_data.cend(), [&path](const QString &p) {
        return path == p;
    });

    if (it == m_data.cend()) {
        return -1;
    }

    return std::distance(m_data.cbegin(), it);
}

void ImageListModel::load(const QStringList &customPaths)
{
    if (m_loading || customPaths.empty()) {
        return;
    }

    m_customPaths = customPaths;
    m_customPaths.removeDuplicates();

    ImageFinder *finder = new ImageFinder(m_customPaths);
    connect(finder, &ImageFinder::imageFound, this, &ImageListModel::slotHandleImageFound);
    QThreadPool::globalInstance()->start(finder);

    m_loading = true;
}

void ImageListModel::slotHandleImageFound(const QStringList &paths)
{
    beginResetModel();

    m_data = paths;
    clearCache();

    endResetModel();

    m_loading = false;
    Q_EMIT loaded(this);
}

QStringList ImageListModel::addBackground(const QString &path)
{
    if (path.isEmpty() || !QFile::exists(path) || m_data.contains(path)) {
        return {};
    }

    if (QFileInfo info(path); info.isHidden() || !isAcceptableSuffix(info.suffix())) {
        // Skip hidden files or Format not supported
        return {};
    }

    beginInsertRows(QModelIndex(), 0, 0);

    m_data.prepend(path);
    m_removableWallpapers.prepend(path);

    endInsertRows();

    return {path};
}

QStringList ImageListModel::removeBackground(const QString &path)
{
    QStringList results;

    if (path.isEmpty()) {
        return results;
    }

    const int idx = indexOf(path);

    if (idx < 0) {
        return results;
    }

    beginRemoveRows(QModelIndex(), idx, idx);

    m_pendingDeletion.remove(m_data.at(idx));
    m_removableWallpapers.removeOne(m_data.at(idx));
    results.append(m_data.takeAt(idx));

    // Remove local wallpaper
    if (path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/"))) {
        QFile f(path);

        if (f.exists()) {
            f.remove();
        }
    }

    endRemoveRows();

    return results;
}
