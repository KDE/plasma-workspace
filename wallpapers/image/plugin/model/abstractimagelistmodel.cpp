/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "abstractimagelistmodel.h"

#include <QtConcurrent>

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
        {SourceRole, QByteArrayLiteral("source")},
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
#ifdef HAVE_KExiv2
    if (m_sizeJobsUrls.contains(path) || path.isEmpty()) {
        return;
    }

    // An ugly way to get mutable "this". asyncGetMediaMetadata() gets called from `data() const` so
    // there's no beating around the bush unfortunately, const_cast is only one of the few options we have left.
    auto self = const_cast<AbstractImageListModel *>(this);

    QtConcurrent::run(MediaMetadata::read, path).then(self, [self, path](const MediaMetadata &metadata) {
        const QPersistentModelIndex index = self->m_sizeJobsUrls.take(path);

        QList<int> dirtyRoles;

        self->m_backgroundTitleCache.insert(path, metadata.title);
        if (!metadata.title.isEmpty()) {
            dirtyRoles.append(Qt::DisplayRole);
        }

        self->m_backgroundAuthorCache.insert(path, metadata.author);
        if (!metadata.author.isEmpty()) {
            dirtyRoles.append(AuthorRole);
        }

        if (!dirtyRoles.isEmpty()) {
            Q_EMIT self->dataChanged(index, index, dirtyRoles);
        }
    });

    self->m_sizeJobsUrls.insert(path, index);
#else
    Q_UNUSED(path)
    Q_UNUSED(index)
#endif
}
