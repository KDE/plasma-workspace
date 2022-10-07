/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "abstractimagelistmodel.h"

#include <QPainter>
#include <QThreadPool>

#include <KFileItem>
#include <KIO/PreviewJob>

#include "../finder/mediametadatafinder.h"
#include "config-KF5KExiv2.h"

AbstractImageListModel::AbstractImageListModel(const QSize &targetSize, QObject *parent)
    : QAbstractListModel(parent)
    , m_screenshotSize(targetSize / 8)
    , m_targetSize(targetSize)
{
    constexpr int maxCacheSize = 10;
    m_imageCache.setMaxCost(maxCacheSize);
    m_backgroundTitleCache.setMaxCost(maxCacheSize);
    m_backgroundAuthorCache.setMaxCost(maxCacheSize);
    m_imageSizeCache.setMaxCost(maxCacheSize);

    connect(this, &QAbstractListModel::rowsInserted, this, &AbstractImageListModel::countChanged);
    connect(this, &QAbstractListModel::rowsRemoved, this, &AbstractImageListModel::countChanged);
    connect(this, &QAbstractListModel::modelReset, this, &AbstractImageListModel::countChanged);
}

QHash<int, QByteArray> AbstractImageListModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {Qt::DecorationRole, "decoration"},
        {AuthorRole, "author"},
        {ScreenshotRole, "screenshot"},
        {PathRole, "path"},
        {PackageNameRole, "packageName"},
        {RemovableRole, "removable"},
        {PendingDeletionRole, "pendingDeletion"},
        {ToggleRole, "checked"},
    };
}

int AbstractImageListModel::count() const
{
    return rowCount();
}

void AbstractImageListModel::reload()
{
    if (m_loading || m_customPaths.empty()) {
        return;
    }

    load(m_customPaths);
}

void AbstractImageListModel::slotTargetSizeChanged(const QSize &size)
{
    m_targetSize = size;
    reload();
}

void AbstractImageListModel::slotHandlePreview(const KFileItem &item, const QPixmap &preview)
{
    auto job = qobject_cast<KIO::PreviewJob *>(sender());
    if (!job) {
        return;
    }

    const QString urlString = item.url().toLocalFile();
    const QPersistentModelIndex idx = job->property("index").toPersistentModelIndex();

    auto it = m_previewJobsUrls.find(idx);
    Q_ASSERT(it != m_previewJobsUrls.end());
    it->removeOne(urlString);

    const QStringList paths = job->property("paths").toStringList();
    auto cachedPreviewIt = m_imageTempCache.find(paths);

    if (cachedPreviewIt == m_imageTempCache.end() && !it->empty()) {
        m_imageTempCache.insert(paths, preview);
        // it->empty() is handled in the end
        return;
    } else if (cachedPreviewIt != m_imageTempCache.end()) {
        // Show multiple images side by side
        QPainter p(&*cachedPreviewIt);

        const int i = paths.indexOf(urlString);
        const double start = i / static_cast<double>(paths.size());
        const double end = (i + 1) / static_cast<double>(paths.size());
        // Cropped area
        const QPoint topLeft(start * preview.width(), 0);
        const QPoint bottomRight(end * preview.width(), preview.height());
        // Inserted area
        const QPoint topLeft2(start * cachedPreviewIt->width(), 0);
        const QPoint bottomRight2(end * cachedPreviewIt->width(), cachedPreviewIt->height());

        p.drawPixmap(QRect(topLeft2, bottomRight2), preview.copy(QRect(topLeft, bottomRight)));
    }

    if (it->empty()) {
        // All images in the list have been loaded
        m_previewJobsUrls.erase(it);

        QPixmap *finalPreview = nullptr;
        if (cachedPreviewIt == m_imageTempCache.end()) {
            // Single image
            finalPreview = new QPixmap(preview);
        } else {
            // Side-by-side image
            finalPreview = new QPixmap(*cachedPreviewIt);
            m_imageTempCache.erase(cachedPreviewIt);
        }

        if (m_imageCache.insert(paths, finalPreview, 1)) {
            Q_EMIT dataChanged(idx, idx, {ScreenshotRole});
        } else {
            delete finalPreview;
        }
    }
}

void AbstractImageListModel::slotHandlePreviewFailed(const KFileItem &item)
{
    auto job = qobject_cast<KIO::PreviewJob *>(sender());
    if (!job) {
        return;
    }

    auto it = m_previewJobsUrls.find(job->property("index").toPersistentModelIndex());
    Q_ASSERT(it != m_previewJobsUrls.end());

    it->removeOne(item.url().toLocalFile());
    if (it->empty()) {
        m_previewJobsUrls.erase(it);
    }
}

void AbstractImageListModel::asyncGetPreview(const QStringList &paths, const QPersistentModelIndex &index) const
{
    if (m_previewJobsUrls.contains(index) || paths.isEmpty()) {
        return;
    }

    const QStringList availablePlugins = KIO::PreviewJob::availablePlugins();
    KFileItemList list;

    for (const QString &path : paths) {
        list.append(KFileItem(QUrl::fromLocalFile(path), QString(), 0));
    }

    KIO::PreviewJob *const job = KIO::filePreview(list, m_screenshotSize, &availablePlugins);
    job->setIgnoreMaximumSize(true);

    job->setProperty("paths", paths);
    job->setProperty("index", index);

    connect(job, &KIO::PreviewJob::gotPreview, this, &AbstractImageListModel::slotHandlePreview);
    connect(job, &KIO::PreviewJob::failed, this, &AbstractImageListModel::slotHandlePreviewFailed);

    m_previewJobsUrls.insert(index, paths);
}

void AbstractImageListModel::asyncGetMediaMetadata(const QString &path, const QPersistentModelIndex &index) const
{
    if (m_sizeJobsUrls.contains(path) || path.isEmpty()) {
        return;
    }

    MediaMetadataFinder *finder = new MediaMetadataFinder(path);
    connect(finder, &MediaMetadataFinder::metadataFound, this, &AbstractImageListModel::slotMediaMetadataFound);
    QThreadPool::globalInstance()->start(finder);

    m_sizeJobsUrls.insert(path, index);
}

void AbstractImageListModel::clearCache()
{
    m_imageCache.clear();
    m_backgroundTitleCache.clear();
    m_backgroundAuthorCache.clear();
    m_imageSizeCache.clear();
}

void AbstractImageListModel::slotMediaMetadataFound(const QString &path, const MediaMetadata &metadata)
{
    const QPersistentModelIndex index = m_sizeJobsUrls.take(path);

#if HAVE_KF5KExiv2
    if (!metadata.title.isEmpty()) {
        auto title = new QString(metadata.title);
        if (m_backgroundTitleCache.insert(path, title, 1)) {
            Q_EMIT dataChanged(index, index, {Qt::DisplayRole});
        } else {
            delete title;
        }
    }

    if (!metadata.author.isEmpty()) {
        auto author = new QString(metadata.author);
        if (m_backgroundAuthorCache.insert(path, author, 1)) {
            Q_EMIT dataChanged(index, index, {AuthorRole});
        } else {
            delete author;
        }
    }
#endif

    auto resolution = new QSize(metadata.resolution);
    if (m_imageSizeCache.insert(path, resolution, 1)) {
        Q_EMIT dataChanged(index, index, {ResolutionRole});
    } else {
        delete resolution;
    }
}
