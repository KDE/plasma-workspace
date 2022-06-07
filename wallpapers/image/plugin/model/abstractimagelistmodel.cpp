/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "abstractimagelistmodel.h"

#include <QPainter>
#include <QThreadPool>

#include <KFileItem>
#include <KIO/PreviewJob>

#include "../finder/imagesizefinder.h"

AbstractImageListModel::AbstractImageListModel(const QSize &targetSize, QObject *parent)
    : QAbstractListModel(parent)
    , m_screenshotSize(targetSize / 8)
    , m_targetSize(targetSize)
{
    m_imageCache.setMaxCost(30);
    m_imageSizeCache.setMaxCost(30);

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

void AbstractImageListModel::slotHandleImageSizeFound(const QString &path, const QSize &size)
{
    const QPersistentModelIndex index = m_sizeJobsUrls.take(path);

    if (m_imageSizeCache.insert(path, new QSize(size), 1)) {
        Q_EMIT dataChanged(index, index, {ResolutionRole});
    }
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
    QPixmap *cachedPreview = m_imageCache.object(paths);

    if (!cachedPreview) {
        // Insert full preview
        m_imageCache.insert(paths, new QPixmap(preview), 0);
    } else {
        // Show multiple images side by side
        QPainter p(cachedPreview);

        const int i = paths.indexOf(urlString);
        const double start = i / static_cast<double>(paths.size());
        const double end = (i + 1) / static_cast<double>(paths.size());
        // Cropped area
        const QPoint topLeft(start * preview.width(), 0);
        const QPoint bottomRight(end * preview.width(), preview.height());
        // Inserted area
        const QPoint topLeft2(start * cachedPreview->width(), 0);
        const QPoint bottomRight2(end * cachedPreview->width(), cachedPreview->height());

        p.drawPixmap(QRect(topLeft2, bottomRight2), preview.copy(QRect(topLeft, bottomRight)));
    }

    if (it->empty()) {
        // All images in the list have been loaded
        m_previewJobsUrls.erase(it);

        cachedPreview = m_imageCache.object(paths);
        auto finalPreview = new QPixmap(*cachedPreview);

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

void AbstractImageListModel::asyncGetImageSize(const QString &path, const QPersistentModelIndex &index) const
{
    if (m_sizeJobsUrls.contains(path) || path.isEmpty()) {
        return;
    }

    ImageSizeFinder *finder = new ImageSizeFinder(path);
    connect(finder, &ImageSizeFinder::sizeFound, this, &AbstractImageListModel::slotHandleImageSizeFound);
    QThreadPool::globalInstance()->start(finder);

    m_sizeJobsUrls.insert(path, index);
}
