/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "limitedrowcountproxymodel_p.h"

#include "notifications.h"

using namespace NotificationManager;

LimitedRowCountProxyModel::LimitedRowCountProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

LimitedRowCountProxyModel::~LimitedRowCountProxyModel() = default;

void LimitedRowCountProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (sourceModel == QAbstractProxyModel::sourceModel()) {
        return;
    }

    if (QAbstractProxyModel::sourceModel()) {
        disconnect(QAbstractProxyModel::sourceModel(), nullptr, this, nullptr);
    }

    QSortFilterProxyModel::setSourceModel(sourceModel);

    if (sourceModel) {
        connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &LimitedRowCountProxyModel::invalidateFilter);
        connect(sourceModel, &QAbstractItemModel::rowsMoved, this, &LimitedRowCountProxyModel::invalidateFilter);
        connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, &LimitedRowCountProxyModel::invalidateFilter);
    }
}

int LimitedRowCountProxyModel::limit() const
{
    return m_limit;
}

void LimitedRowCountProxyModel::setLimit(int limit)
{
    if (m_limit != limit) {
        m_limit = limit;
        invalidateFilter();
        Q_EMIT limitChanged();
    }
}

bool LimitedRowCountProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (source_parent.isValid()) {
        return true;
    }

    if (m_limit > 0) {
        return source_row < m_limit;
    }

    return true;
}
