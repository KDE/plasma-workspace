/*
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "slidefiltermodel.h"

#include "backgroundlistmodel.h"
#include "slidemodel.h"

#include <QFileInfo>
#include <QRandomGenerator>

#include <algorithm>

SlideFilterModel::SlideFilterModel(QObject *parent)
    : QSortFilterProxyModel{parent}
    , m_SortingMode{Image::Random}
    , m_usedInConfig{false}
    , m_random(m_randomDevice())
{
    srand(time(nullptr));
    setSortCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    connect(this, &SlideFilterModel::usedInConfigChanged, this, &SlideFilterModel::invalidateFilter);
}

bool SlideFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    auto index = sourceModel()->index(source_row, 0, source_parent);
    return m_usedInConfig || index.data(BackgroundListModel::ToggleRole).toBool();
}

void SlideFilterModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (this->sourceModel()) {
        disconnect(this->sourceModel(), nullptr, this, nullptr);
    }
    QSortFilterProxyModel::setSourceModel(sourceModel);
    if (m_SortingMode == Image::Random && !m_usedInConfig) {
        buildRandomOrder();
    }
    if (sourceModel) {
        connect(sourceModel, &QAbstractItemModel::modelReset, this, &SlideFilterModel::buildRandomOrder);
        connect(sourceModel, &QAbstractItemModel::rowsInserted, this, [this] {
            if (m_SortingMode != Image::Random || m_usedInConfig) {
                return;
            }
            const int old_count = m_randomOrder.size();
            m_randomOrder.resize(this->sourceModel()->rowCount());
            std::iota(m_randomOrder.begin() + old_count, m_randomOrder.end(), old_count);
            std::shuffle(m_randomOrder.begin() + old_count, m_randomOrder.end(), m_random);
        });
        connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, [this] {
            if (m_SortingMode != Image::Random || m_usedInConfig) {
                return;
            }
            m_randomOrder.erase(std::remove_if(m_randomOrder.begin(),
                                               m_randomOrder.end(),
                                               [this](const int v) {
                                                   return v >= this->sourceModel()->rowCount();
                                               }),
                                m_randomOrder.end());
        });
    }
}

bool SlideFilterModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    switch (m_SortingMode) {
    case Image::Random:
        if (m_usedInConfig) {
            return source_left.row() < source_right.row();
        }
        return m_randomOrder.indexOf(source_left.row()) < m_randomOrder.indexOf(source_right.row());
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
    if (m_SortingMode == Image::Random && !m_usedInConfig) {
        buildRandomOrder();
    }
    QSortFilterProxyModel::invalidate();
}

void SlideFilterModel::invalidate()
{
    if (m_SortingMode == Image::Random && !m_usedInConfig) {
        std::shuffle(m_randomOrder.begin(), m_randomOrder.end(), m_random);
    }
    QSortFilterProxyModel::invalidate();
}

void SlideFilterModel::invalidateFilter()
{
    QSortFilterProxyModel::invalidateFilter();
}

int SlideFilterModel::indexOf(const QString &path)
{
    auto sourceIndex = sourceModel()->index(static_cast<SlideModel *>(sourceModel())->indexOf(path), 0);
    return mapFromSource(sourceIndex).row();
}

void SlideFilterModel::openContainingFolder(int rowIndex)
{
    auto sourceIndex = mapToSource(index(rowIndex, 0));
    static_cast<SlideModel *>(sourceModel())->openContainingFolder(sourceIndex.row());
}

void SlideFilterModel::buildRandomOrder()
{
    if (sourceModel()) {
        m_randomOrder.resize(sourceModel()->rowCount());
        std::iota(m_randomOrder.begin(), m_randomOrder.end(), 0);
        std::shuffle(m_randomOrder.begin(), m_randomOrder.end(), m_random);
    }
}
