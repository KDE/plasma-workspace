/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QSortFilterProxyModel>

class LimitedRowCountProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit LimitedRowCountProxyModel(QObject *parent = nullptr);
    ~LimitedRowCountProxyModel() override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    int limit() const;
    void setLimit(int limit);

Q_SIGNALS:
    void limitChanged();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    int m_limit = 0;
};
