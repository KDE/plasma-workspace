/*
 *  Copyright 2019 David Redondo <kde@david-redondo.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

#ifndef SLIDEFILTERMODEL_H
#define SLIDEFILTERMODEL_H

#include <image.h>

#include <QSortFilterProxyModel>
#include <QVector>

class SlideFilterModel : public QSortFilterProxyModel {

    Q_OBJECT

    Q_PROPERTY(bool usedInConfig MEMBER m_usedInConfig NOTIFY usedInConfigChanged);

public:
    SlideFilterModel(QObject* parent);
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;
    void setSortingMode(Image::SlideshowMode mode);
    void invalidate();
    void invalidateFilter();

    Q_INVOKABLE int indexOf(const QString& path);
    Q_INVOKABLE void openContainingFolder(int rowIndex);

Q_SIGNALS:
    void usedInConfigChanged();

private:
    void buildRandomOrder();

    QVector<int> m_randomOrder;
    Image::SlideshowMode m_SortingMode;
    bool m_usedInConfig;
};
#endif
