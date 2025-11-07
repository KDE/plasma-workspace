/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>
#include <QBindable>
#include <QPixmap>
#include <QSize>

#include "imageroles.h"

class KFileItem;

struct MediaMetadata;

/**
 * Base class for image list model.
 */
class AbstractImageListModel : public QAbstractListModel, public ImageRoles
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit AbstractImageListModel(const QBindable<bool> &bindableUsedInConfig, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    virtual int indexOf(const QUrl &url) const = 0;

    virtual void load(const QStringList &customPaths = {});
    /**
     * Reload when a new package is installed
     */
    void reload();

    virtual QUrl effectiveSource(const QModelIndex &index, const QSize &targetSize) const = 0;

public Q_SLOTS:
    virtual QStringList addBackground(const QUrl &url) = 0;
    /**
     * @return removed files that should be removed from \KDirWatch
     */
    virtual QStringList removeBackground(const QUrl &url) = 0;

Q_SIGNALS:
    void countChanged();
    void loaded(AbstractImageListModel *model);

protected:
    /**
     * Asynchronously extracts metadata from an image or a video file.
     */
    void asyncGetMediaMetadata(const QString &path, const QPersistentModelIndex &index) const;

    bool m_loading = false;

    QHash<QString, QString /* title */> m_backgroundTitleCache;
    QHash<QString, QString /* author */> m_backgroundAuthorCache;
    QHash<QString, QPersistentModelIndex> m_sizeJobsUrls;

    QHash<QString, bool> m_pendingDeletion;
    QStringList m_removableWallpapers;
    QStringList m_customPaths;

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(AbstractImageListModel, bool, m_usedInConfig, true)

    friend class ImageProxyModel; // For m_removableWallpapers
};
