/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "xmlimagelistmodel.h"

#include <QPixmap>
#include <QThreadPool>

#include "../finder/xmlfinder.h"
#include "xmlpreviewgenerator.h"

XmlImageListModel::XmlImageListModel(const QSize &targetSize, QObject *parent)
    : AbstractImageListModel(targetSize, parent)
{
    qRegisterMetaType<WallpaperItem>();
    qRegisterMetaType<QList<WallpaperItem>>();
}

int XmlImageListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

QVariant XmlImageListModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();

    if (!index.isValid() || row >= m_data.size() || row < 0) {
        return QVariant();
    }

    const auto &item = m_data.at(row);

    switch (role) {
    case Qt::DisplayRole:
        return item.name;

    case ScreenshotRole: {
        QPixmap *cachedPreview = m_imageCache.object(item.path.toString());

        if (cachedPreview) {
            return *cachedPreview;
        }

        asyncGetXmlPreview(item, QPersistentModelIndex(index));

        return QVariant();
    }

    case AuthorRole:
        return item.author;

    case ResolutionRole: {
        const QString realPath = getRealPath(item);
        QSize *size = m_imageSizeCache.object(realPath);

        if (size && size->isValid()) {
            return QStringLiteral("%1x%2").arg(size->width()).arg(size->height());
        }

        asyncGetImageSize(realPath, QPersistentModelIndex(index));

        return QString();
    }

    case PathRole:
        return QUrl::fromLocalFile(item.filename);

    case PackageNameRole:
        return item.path.toString();

    case RemovableRole:
        return m_removableWallpapers.contains(item.path.toString());

    case PendingDeletionRole:
        return m_pendingDeletion.value(item.path.toString(), false);

    default:
        return QVariant();
    }
}

bool XmlImageListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role == PendingDeletionRole) {
        m_pendingDeletion[m_data.at(index.row()).path.toString()] = value.toBool();

        Q_EMIT dataChanged(index, index, {PendingDeletionRole});
        return true;
    }

    return false;
}

int XmlImageListModel::indexOf(const QString &path) const
{
    const auto it = std::find_if(m_data.cbegin(), m_data.cend(), [&path](const WallpaperItem &p) {
        return path == p.path.toString();
    });

    if (it == m_data.cend()) {
        return -1;
    }

    return std::distance(m_data.cbegin(), it);
}

void XmlImageListModel::load(const QStringList &customPaths)
{
    if (m_loading || customPaths.empty()) {
        return;
    }

    m_customPaths = customPaths;
    m_customPaths.removeDuplicates();

    XmlFinder *finder = new XmlFinder(m_customPaths, m_targetSize);
    connect(finder, &XmlFinder::xmlFound, this, &XmlImageListModel::slotXmlFound);
    QThreadPool::globalInstance()->start(finder);

    m_loading = true;
}

QStringList XmlImageListModel::addBackground(const QString &path)
{
    QStringList results;

    if (!path.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
        return results;
    }

    const auto items = XmlFinder::parseXml(path, m_targetSize);

    if (items.empty()) {
        return results;
    }

    // Remove duplicates
    std::vector<WallpaperItem> pendingList;
    std::copy_if(items.cbegin(), items.cend(), std::back_inserter(pendingList), [this](const WallpaperItem &item) {
        return indexOf(item.path.toString()) < 0;
    });

    if (pendingList.empty()) {
        return results;
    }

    beginInsertRows(QModelIndex(), 0, pendingList.size() - 1);

    for (const auto &p : std::as_const(pendingList)) {
        m_data.prepend(p);
        m_removableWallpapers.prepend(p.path.toString());
        results.prepend(p.path.toString());
    }

    endInsertRows();

    return results;
}

QStringList XmlImageListModel::removeBackground(const QString &path)
{
    QStringList results;

    if (path.isEmpty()) {
        return results;
    }

    int idx = indexOf(path);

    // For KDirWatch
    if (idx < 0) {
        // Check root path
        const auto it = std::find_if(m_data.cbegin(), m_data.cend(), [&path](const WallpaperItem &p) {
            return p._root == path;
        });

        if (it != m_data.cend()) {
            idx = std::distance(m_data.cbegin(), it);
        }
    }

    if (idx < 0) {
        // Check filename
        const auto it2 = std::find_if(m_data.cbegin(), m_data.cend(), [&path](const WallpaperItem &p) {
            return p.filename == path;
        });

        if (it2 != m_data.cend()) {
            idx = std::distance(m_data.cbegin(), it2);
        }
    }

    if (idx < 0) {
        return results;
    }

    beginRemoveRows(QModelIndex(), idx, idx);

    const auto p = m_data.takeAt(idx);

    m_pendingDeletion.remove(p.path.toString());
    m_removableWallpapers.removeOne(p.path.toString());
    results.append(p.path.toString());

    endRemoveRows();

    return results;
}

void XmlImageListModel::slotXmlFound(const QList<WallpaperItem> &packages)
{
    beginResetModel();

    m_data = packages;

    endResetModel();

    m_loading = false;
    Q_EMIT loaded(this);
}

void XmlImageListModel::slotXmlFinderGotPreview(const WallpaperItem &item, const QPixmap &_preview)
{
    const QPersistentModelIndex pIdx = m_previewJobsUrls.take(item.path.toString());
    QModelIndex idx;

    if (!pIdx.isValid()) {
        if (int row = indexOf(item.path.toString()); row >= 0) {
            idx = index(row, 0);
        } else {
            return;
        }
    } else {
        idx = pIdx;
    }

    const int cost = _preview.width() * _preview.height() * _preview.depth() / 8;
    auto preview = new QPixmap(_preview);

    if (m_imageCache.insert(item.path.toString(), preview, cost)) {
        Q_EMIT dataChanged(idx, idx, {ScreenshotRole});
    } else {
        delete preview;
    }
}

void XmlImageListModel::slotXmlFinderFailed(const WallpaperItem &item)
{
    m_previewJobsUrls.remove(item.path.toString());
}

void XmlImageListModel::asyncGetXmlPreview(const WallpaperItem &item, const QPersistentModelIndex &index) const
{
    if (m_previewJobsUrls.contains(item.path.toString()) || item.path.isEmpty()) {
        return;
    }

    XmlPreviewGenerator *finder = new XmlPreviewGenerator(item, m_screenshotSize);
    connect(finder, &XmlPreviewGenerator::gotPreview, this, &XmlImageListModel::slotXmlFinderGotPreview);
    connect(finder, &XmlPreviewGenerator::failed, this, &XmlImageListModel::slotXmlFinderFailed);
    QThreadPool::globalInstance()->start(finder);

    m_previewJobsUrls.insert(item.path.toString(), index);
}

QString XmlImageListModel::getRealPath(const WallpaperItem &item) const
{
    QString path = item.filename;

    const auto it = std::find_if(item.slideshow.data.cbegin(), item.slideshow.data.cend(), [](const SlideshowItemData &d) {
        return d.dataType == 0 && !d.file.isEmpty();
    });

    if (it != item.slideshow.data.cend()) {
        path = it->file;
    }

    return path;
}
