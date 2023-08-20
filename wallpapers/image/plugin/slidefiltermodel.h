/*
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QBindable>
#include <QFileInfo>
#include <QSortFilterProxyModel>
#include <QVector>

#include <random>

#include "sortingmode.h"

class SlideFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit SlideFilterModel(const QBindable<bool> &usedInConfig, QObject *parent);

    QHash<int, QByteArray> roleNames() const override;

    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;
    void setSortingMode(SortingMode::Mode slideshowMode, bool slideshowFoldersFirst);
    void invalidate();
    void invalidateFilter();

    Q_INVOKABLE int indexOf(const QString &path);
    Q_INVOKABLE void openContainingFolder(int rowIndex);

private:
    void buildRandomOrder();

    QString getLocalFilePath(const QModelIndex &modelIndex) const;
    QString getFilePathWithDir(const QFileInfo &fileInfo) const;

    QVector<int> m_randomOrder;
    SortingMode::Mode m_SortingMode = SortingMode::Random;
    bool m_SortingFoldersFirst = false;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SlideFilterModel, bool, m_usedInConfig, true)
    QPropertyNotifier m_usedInConfigNotifier;
    std::random_device m_randomDevice;
    std::mt19937 m_random;
};
