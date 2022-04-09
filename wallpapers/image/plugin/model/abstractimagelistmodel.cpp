/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "abstractimagelistmodel.h"

#include <QPixmap>
#include <QThreadPool>

#include <KFileItem>
#include <KIO/PreviewJob>

#include "../finder/imagesizefinder.h"

AbstractImageListModel::AbstractImageListModel(const QSize &targetSize, QObject *parent)
    : QAbstractListModel(parent)
    , m_screenshotSize(targetSize / 8)
    , m_targetSize(targetSize)
{
    m_imageCache.setMaxCost(m_screenshotSize.width() * m_screenshotSize.height() * 20);
    m_imageSizeCache.setMaxCost(20);

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
    const QString urlString = item.url().toLocalFile();
    const QPersistentModelIndex pidx = m_previewJobsUrls.take(urlString);
    QModelIndex idx;

    if (!pidx.isValid()) {
        if (int row = indexOf(urlString); row >= 0) {
            idx = index(row, 0);
        } else {
            return;
        }
    } else {
        idx = pidx;
    }

    const int cost = preview.width() * preview.height() * preview.depth() / 8;

    if (m_imageCache.insert(urlString, new QPixmap(preview), cost)) {
        Q_EMIT dataChanged(idx, idx, {ScreenshotRole});
    }
}

void AbstractImageListModel::slotHandlePreviewFailed(const KFileItem &item)
{
    m_previewJobsUrls.remove(item.url().toLocalFile());
}

void AbstractImageListModel::asyncGetPreview(const QString &path, const QPersistentModelIndex &index) const
{
    if (m_previewJobsUrls.contains(path) || path.isEmpty()) {
        return;
    }

    const QUrl url = QUrl::fromLocalFile(path);
    const QStringList availablePlugins = KIO::PreviewJob::availablePlugins();

    KIO::PreviewJob *const job = KIO::filePreview(KFileItemList{KFileItem(url, QString(), 0)}, m_screenshotSize, &availablePlugins);
    job->setIgnoreMaximumSize(true);

    connect(job, &KIO::PreviewJob::gotPreview, this, &AbstractImageListModel::slotHandlePreview);
    connect(job, &KIO::PreviewJob::failed, this, &AbstractImageListModel::slotHandlePreviewFailed);

    m_previewJobsUrls.insert(path, index);
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
