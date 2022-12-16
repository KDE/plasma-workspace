/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractimagelistmodel.h"

/**
 * List images that do not belong to any KPackages.
 */
class ImageListModel : public AbstractImageListModel
{
    Q_OBJECT

public:
    explicit ImageListModel(const QSize &targetSize, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    int indexOf(const QString &path) const override;

    void load(const QStringList &customPaths = {}) override;

public Q_SLOTS:
    QStringList addBackground(const QString &path) override;
    QStringList removeBackground(const QString &path) override;

protected Q_SLOTS:
    void slotHandleImageFound(const QStringList &paths);

private:
    QStringList m_data;

    friend class ImageListModelTest;
};
