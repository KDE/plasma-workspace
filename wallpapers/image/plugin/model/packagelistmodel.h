/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractimagelistmodel.h"
#include "finder/packagefinder.h"

/**
 * List KPackage wallpapers, usually in a folder.
 */
class PackageListModel : public AbstractImageListModel
{
    Q_OBJECT

public:
    explicit PackageListModel(const QBindable<QSize> &bindableTargetSize, const QBindable<bool> &bindableUsedInConfig, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    /**
     * @path Package folder path
     */
    int indexOf(const QUrl &url) const override;

    void load(const QStringList &customPaths = {}) override;

public Q_SLOTS:
    QStringList addBackground(const QUrl &url) override;
    QStringList removeBackground(const QUrl &url) override;

private:
    QList<WallpaperPackage> m_packages;

    friend class PackageListModelTest;
};
