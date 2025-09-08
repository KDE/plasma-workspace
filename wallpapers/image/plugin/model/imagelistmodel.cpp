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
#include <QUrlQuery>

#include <KIO/PreviewJob>
#include <algorithm>

#include "../finder/imagefinder.h"
#include "../finder/suffixcheck.h"

ImageListModel::ImageListModel(const QBindable<QSize> &bindableTargetSize, const QBindable<bool> &bindableUsedInConfig, QObject *parent)
    : AbstractImageListModel(bindableTargetSize, bindableUsedInConfig, parent)
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

    case PreviewRole: {
        QUrl previewUrl(QStringLiteral("image://wallpaper-preview"));
        previewUrl.setQuery({
            std::make_pair(QStringLiteral("image"), m_data.at(row)),
        });
        return previewUrl;
    }

    case AuthorRole: {
        const QString *const author = m_backgroundAuthorCache.object(m_data.at(row));

        if (author) {
            return *author;
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

    case SelectorsRole:
        return QStringList();
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

int ImageListModel::indexOf(const QUrl &url) const
{
    const auto it = std::ranges::find(m_data, url.toLocalFile());
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

    AbstractImageListModel::load(customPaths);

    auto *finder = new ImageFinder(m_customPaths);
    connect(finder, &ImageFinder::imageFound, this, &ImageListModel::slotHandleImageFound);
    QThreadPool::globalInstance()->start(finder);
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

QStringList ImageListModel::addBackground(const QUrl &url)
{
    const QString path = url.toLocalFile();
    if (path.isEmpty() || !QFile::exists(path) || m_data.contains(path)) {
        return {};
    }

    if (QFileInfo info(path); info.isHidden() || !isAcceptableSuffix(info.suffix())) {
        // Skip hidden files or Format not supported
        return {};
    }

    if (m_usedInConfig) {
        beginInsertRows(QModelIndex(), 0, 0);

        m_data.prepend(path);
        m_removableWallpapers.prepend(path);

        endInsertRows();
    } else {
        // In a slideshow, append to the last so the random order can be kept
        const int count = rowCount();
        beginInsertRows(QModelIndex(), count, count);

        m_data.append(path);
        m_removableWallpapers.append(path);

        endInsertRows();
    }

    return {path};
}

QStringList ImageListModel::removeBackground(const QUrl &url)
{
    QStringList results;

    const int idx = indexOf(url);
    if (idx < 0) {
        return results;
    }

    beginRemoveRows(QModelIndex(), idx, idx);

    m_pendingDeletion.remove(m_data.at(idx));
    m_removableWallpapers.removeOne(m_data.at(idx));
    results.append(m_data.takeAt(idx));

    // Remove local wallpaper
    const QString path = url.toLocalFile();
    if (path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wallpapers/"))) {
        QFile f(path);

        if (f.exists()) {
            f.remove();
        }
    }

    endRemoveRows();

    return results;
}
