/*
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SLIDEFILTERMODEL_H
#define SLIDEFILTERMODEL_H

#include <image.h>

#include <QSortFilterProxyModel>
#include <QVector>

#include <random>

class SlideFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(bool usedInConfig MEMBER m_usedInConfig NOTIFY usedInConfigChanged);

public:
    SlideFilterModel(QObject *parent);
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;
    void setSortingMode(Image::SlideshowMode mode);
    void invalidate();
    void invalidateFilter();

    Q_INVOKABLE int indexOf(const QString &path);
    Q_INVOKABLE void openContainingFolder(int rowIndex);

Q_SIGNALS:
    void usedInConfigChanged();

private:
    void buildRandomOrder();

    QVector<int> m_randomOrder;
    Image::SlideshowMode m_SortingMode;
    bool m_usedInConfig;
    std::random_device m_randomDevice;
    std::mt19937 m_random;
};
#endif
