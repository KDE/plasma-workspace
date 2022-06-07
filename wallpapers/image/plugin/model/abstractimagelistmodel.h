/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ABSTRACTIMAGELISTMODEL_H
#define ABSTRACTIMAGELISTMODEL_H

#include <QAbstractListModel>
#include <QCache>
#include <QPixmap>
#include <QSize>

#include "imageroles.h"

class KFileItem;

/**
 * Base class for image list model.
 */
class AbstractImageListModel : public QAbstractListModel, public ImageRoles
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit AbstractImageListModel(const QSize &targetSize, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    virtual int indexOf(const QString &path) const = 0;

    virtual void load(const QStringList &customPaths = {}) = 0;
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

    void slotTargetSizeChanged(const QSize &size);

Q_SIGNALS:
    void countChanged();
    void loaded(AbstractImageListModel *model);

protected:
    /**
     * Asynchronously generates a preview.
     * Multiple images are displayed side by side following the order in @c paths
     *
     * @note @c paths should have no duplicate urls.
     */
    void asyncGetPreview(const QStringList &paths, const QPersistentModelIndex &index) const;
    void asyncGetImageSize(const QString &path, const QPersistentModelIndex &index) const;

    bool m_loading = false;

    QSize m_screenshotSize;
    QSize m_targetSize;

    QCache<QStringList, QPixmap> m_imageCache;
    QCache<QString, QSize> m_imageSizeCache;

    mutable QHash<QPersistentModelIndex, QStringList> m_previewJobsUrls;
    mutable QHash<QString, QPersistentModelIndex> m_sizeJobsUrls;

    QHash<QString, bool> m_pendingDeletion;
    QStringList m_removableWallpapers;
    QStringList m_customPaths;

    friend class ImageProxyModel; // For m_removableWallpapers

private Q_SLOTS:
    void slotHandleImageSizeFound(const QString &path, const QSize &size);
    void slotHandlePreview(const KFileItem &item, const QPixmap &preview);
    void slotHandlePreviewFailed(const KFileItem &item);
};

#endif // ABSTRACTIMAGELISTMODEL_H
