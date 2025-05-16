/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>
#include <QBindable>
#include <QCache>
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
    explicit AbstractImageListModel(const QBindable<QSize> &bindableTargetSize, const QBindable<bool> &bindableUsedInConfig, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    virtual int indexOf(const QString &path) const = 0;

    virtual void load(const QStringList &customPaths = {});
    /**
     * Reload when target size changes or a new package is installed
     */
    void reload();

public Q_SLOTS:
    virtual QStringList addBackground(const QString &path) = 0;
    /**
     * @return removed files that should be removed from \KDirWatch
     */
    virtual QStringList removeBackground(const QString &path) = 0;

Q_SIGNALS:
    void countChanged();
    void loaded(AbstractImageListModel *model);

protected:
    /**
     * Asynchronously extracts metadata from an image or a video file.
     */
    void asyncGetMediaMetadata(const QString &path, const QPersistentModelIndex &index) const;

    /**
     * Clears all cached records.
     */
    void clearCache();

    bool m_loading = false;

    Q_OBJECT_BINDABLE_PROPERTY(AbstractImageListModel, QSize, m_screenshotSize)
    Q_OBJECT_BINDABLE_PROPERTY(AbstractImageListModel, QSize, m_targetSize)
    QPropertyNotifier m_targetSizeChangeNotifier;

    QCache<QString, QString /* title */> m_backgroundTitleCache;
    QCache<QString, QString /* author */> m_backgroundAuthorCache;
    QCache<QString, QSize> m_imageSizeCache;

    mutable QHash<QString, QPersistentModelIndex> m_sizeJobsUrls;

    QHash<QString, bool> m_pendingDeletion;
    QStringList m_removableWallpapers;
    QStringList m_customPaths;

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(AbstractImageListModel, bool, m_usedInConfig, true)

    friend class ImageProxyModel; // For m_removableWallpapers

private Q_SLOTS:
    void slotMediaMetadataFound(const QString &path, const MediaMetadata &metadata);
};
