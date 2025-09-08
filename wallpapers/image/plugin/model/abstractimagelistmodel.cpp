/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "abstractimagelistmodel.h"

#include <QThreadPool>

#include "../finder/mediametadatafinder.h"
#include "config-KExiv2.h"

AbstractImageListModel::AbstractImageListModel(const QBindable<QSize> &bindableTargetSize, const QBindable<bool> &bindableUsedInConfig, QObject *parent)
    : QAbstractListModel(parent)
{
    m_targetSize.setBinding(bindableTargetSize.makeBinding());
    m_screenshotSize.setBinding([this] {
        return m_targetSize.value() / 8;
    });
    m_targetSizeChangeNotifier = m_screenshotSize.addNotifier([this] {
        reload();
    });
    m_usedInConfig.setBinding(bindableUsedInConfig.makeBinding());

    constexpr int maxCacheSize = 10;
    m_backgroundTitleCache.setMaxCost(maxCacheSize);
    m_backgroundAuthorCache.setMaxCost(maxCacheSize);

    connect(this, &QAbstractListModel::rowsInserted, this, &AbstractImageListModel::countChanged);
    connect(this, &QAbstractListModel::rowsRemoved, this, &AbstractImageListModel::countChanged);
    connect(this, &QAbstractListModel::modelReset, this, &AbstractImageListModel::countChanged);
}

QHash<int, QByteArray> AbstractImageListModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {Qt::DecorationRole, QByteArrayLiteral("decoration")},
        {AuthorRole, QByteArrayLiteral("author")},
        {PreviewRole, QByteArrayLiteral("preview")},
        {PathRole, QByteArrayLiteral("path")},
        {PackageNameRole, QByteArrayLiteral("packageName")},
        {RemovableRole, QByteArrayLiteral("removable")},
        {PendingDeletionRole, QByteArrayLiteral("pendingDeletion")},
        {ToggleRole, QByteArrayLiteral("checked")},
        {SelectorsRole, QByteArrayLiteral("selectors")},
    };
}

int AbstractImageListModel::count() const
{
    return rowCount();
}

void AbstractImageListModel::load(const QStringList &customPaths)
{
    Q_ASSERT(!m_loading && !customPaths.empty());
    m_customPaths = customPaths;
    m_customPaths.removeDuplicates();
    m_loading = true;
}

void AbstractImageListModel::reload()
{
    if (m_loading || m_customPaths.empty()) {
        return;
    }

    load(m_customPaths);
}

void AbstractImageListModel::asyncGetMediaMetadata(const QString &path, const QPersistentModelIndex &index) const
{
    if (m_sizeJobsUrls.contains(path) || path.isEmpty()) {
        return;
    }

    auto *finder = new MediaMetadataFinder(path);
    connect(finder, &MediaMetadataFinder::metadataFound, this, &AbstractImageListModel::slotMediaMetadataFound);
    QThreadPool::globalInstance()->start(finder);

    m_sizeJobsUrls.insert(path, index);
}

void AbstractImageListModel::clearCache()
{
    m_backgroundTitleCache.clear();
    m_backgroundAuthorCache.clear();
}

void AbstractImageListModel::slotMediaMetadataFound(const QString &path, const MediaMetadata &metadata)
{
    const QPersistentModelIndex index = m_sizeJobsUrls.take(path);

#if HAVE_KExiv2
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
}
