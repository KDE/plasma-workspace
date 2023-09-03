/*
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <random>

#include <QBindable>
#include <QSortFilterProxyModel>
#include <QVector>

#include "sortingmode.h"

class SlideFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit SlideFilterModel(const QBindable<bool> &usedInConfig,
                              const QBindable<SortingMode::Mode> &sortingMode,
                              const QBindable<bool> &slideshowFoldersFirst,
                              QObject *parent);

    QHash<int, QByteArray> roleNames() const override;

    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;
    void invalidate();
    void invalidateFilter();

    Q_INVOKABLE int indexOf(const QString &path);
    Q_INVOKABLE void openContainingFolder(int rowIndex);

private:
    void buildRandomOrder();

    QVector<int> m_randomOrder;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SlideFilterModel, SortingMode::Mode, m_SortingMode, SortingMode::Random)
    QPropertyNotifier m_SortingModeNotifier;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SlideFilterModel, bool, m_SortingFoldersFirst, false)
    QPropertyNotifier m_slideshowFoldersFirstNotifier;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SlideFilterModel, bool, m_usedInConfig, true)
    QPropertyNotifier m_usedInConfigNotifier;
    std::random_device m_randomDevice;
    std::mt19937 m_random;
};
