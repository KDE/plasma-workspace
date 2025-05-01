/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QBindable>
#include <QConcatenateTablesProxyModel>
#include <QSize>

#include <KDirWatch>

#include "imageroles.h"

class AbstractImageListModel;
class ImageListModel;
class PackageListModel;

/**
 * A proxy model that aggregates data from ImageListModel and PackageListModel.
 */
class ImageProxyModel : public QConcatenateTablesProxyModel, public ImageRoles
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit ImageProxyModel(const QStringList &customPaths,
                             const QBindable<QSize> &bindableTargetSize,
                             const QBindable<bool> &bindableUsedInConfig,
                             QObject *parent);

    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    Q_INVOKABLE int indexOf(const QString &packagePath) const;

    QBindable<bool> loading() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE QStringList addBackground(const QString &_path);
    void removeBackground(const QString &packagePath);

    Q_INVOKABLE void commitAddition();
    Q_INVOKABLE void commitDeletion();

    Q_INVOKABLE void openContainingFolder(int row) const;

Q_SIGNALS:
    void countChanged();
    void loadingChanged();

private Q_SLOTS:
    void slotHandleLoaded(AbstractImageListModel *model);

    /**
     * Slots to handle file change signals from KDirWatch
     */
    void slotDirWatchCreated(const QString &path);
    void slotDirWatchDeleted(const QString &path);

private:
    /**
     * Add files to KDirWatch.
     * Files or dirs should be already added to KDirWatch when
     * rowsInserted or rowsRemoved is emitted.
     */
    void setupDirWatch();

    ImageListModel *m_imageModel;
    PackageListModel *m_packageModel;

    KDirWatch m_dirWatch;
    QStringList m_customPaths;

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ImageProxyModel, int, m_loaded, 0)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ImageProxyModel, bool, m_loading, false, &ImageProxyModel::loadingChanged)

    QStringList m_pendingAddition;

    friend class ImageProxyModelTest;
};
