/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "limitedrowcountproxymodel_p.h"

#include  "notifications.h"

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
        emit limitChanged();
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
