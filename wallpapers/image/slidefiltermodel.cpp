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

#include "slidefiltermodel.h"

#include "backgroundlistmodel.h"
#include "slidemodel.h"

#include <QRandomGenerator>
#include <QFileInfo>

SlideFilterModel::SlideFilterModel(QObject* parent)
    : QSortFilterProxyModel{parent}
    , m_SortingMode{Image::Random}
    , m_usedInConfig{false}
{
    setSortCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    connect(this, &SlideFilterModel::usedInConfigChanged, this, &SlideFilterModel::invalidateFilter);
}

bool SlideFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    auto index = sourceModel()->index(source_row, 0, source_parent);
    return m_usedInConfig || index.data(BackgroundListModel::ToggleRole).toBool();
}

bool SlideFilterModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
        switch (m_SortingMode) {
        case Image::Random:
            return (*QRandomGenerator::system())() % 2 == 0;
        case Image::Alphabetical:
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        case Image::AlphabeticalReversed:
            return !QSortFilterProxyModel::lessThan(source_left, source_right);
        case Image::Modified: // oldest first
        {
            QFileInfo f1(source_left.data(BackgroundListModel::PathRole).toUrl().toLocalFile());
            QFileInfo f2(source_right.data(BackgroundListModel::PathRole).toUrl().toLocalFile());
            return f1.lastModified() < f2.lastModified();
        }
        case Image::ModifiedReversed: // newest first
        {
            QFileInfo f1(source_left.data(BackgroundListModel::PathRole).toUrl().toLocalFile());
            QFileInfo f2(source_right.data(BackgroundListModel::PathRole).toUrl().toLocalFile());
            return !(f1.lastModified() < f2.lastModified());
        }
        }
        Q_UNREACHABLE();
}

void SlideFilterModel::setSortingMode(Image::SlideshowMode mode)
{
    m_SortingMode = mode;
    if (!(m_usedInConfig && mode == Image::Random)) {
        QSortFilterProxyModel::invalidate();
    }
}

void SlideFilterModel::invalidateFilter()
{
    QSortFilterProxyModel::invalidateFilter();
}

int SlideFilterModel::indexOf(const QString& path)
{
    auto sourceIndex = sourceModel()->index(static_cast<SlideModel*>(sourceModel())->indexOf(path), 0);
    return mapFromSource(sourceIndex).row();
}

void SlideFilterModel::openContainingFolder(int rowIndex)
{
    auto sourceIndex = mapToSource(index(rowIndex, 0));
    static_cast<SlideModel*>(sourceModel())->openContainingFolder(sourceIndex.row());
}
