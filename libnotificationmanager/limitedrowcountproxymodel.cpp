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
    if (this->sourceModel()) {
        disconnect(this->sourceModel(), nullptr, this, nullptr);
    }

    QSortFilterProxyModel::setSourceModel(sourceModel);

    if (sourceModel) {
        connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &LimitedRowCountProxyModel::invalidateFilter);
        connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, &LimitedRowCountProxyModel::invalidateFilter);
    }
}

QVariant LimitedRowCountProxyModel::data(const QModelIndex &index, int role) const
{
    // FIXME split all of this out into a group limiter proxy model thing
    // so this one stays super generic
    if (m_childLimit > 0 && /*!index.parent().isValid() && */role == NotificationManager::Notifications::IsGroupExpandedRole) {
        return m_unlimitedChildren.contains(index);
    }

    return QSortFilterProxyModel::data(index, role);
}

int LimitedRowCountProxyModel::rootLimit() const
{
    return m_rootLimit;
}

void LimitedRowCountProxyModel::setRootLimit(int limit)
{
    if (m_rootLimit != limit) {
        m_rootLimit = limit;
        invalidateFilter();
        emit rootLimitChanged();
    }
}

int LimitedRowCountProxyModel::childLimit() const
{
    return m_childLimit;
}

void LimitedRowCountProxyModel::setChildLimit(int limit)
{
    if (m_childLimit != limit) {
        m_childLimit = limit;
        invalidateFilter();
        emit childLimitChanged();
    }
}

void LimitedRowCountProxyModel::setNodeLimited(const QModelIndex &idx, bool limited)
{
    if (limited) {
        // remove unlimited
        m_unlimitedChildren.removeAll(QPersistentModelIndex(idx));
    } else {
        m_unlimitedChildren.append(QPersistentModelIndex(idx));
    }

    invalidateFilter();
    emit dataChanged(idx, idx, {NotificationManager::Notifications::IsGroupExpandedRole});
}

bool LimitedRowCountProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (source_parent.isValid()) {
        if (m_childLimit > 0) {
            if (!m_unlimitedChildren.isEmpty()) {
                const QModelIndex parent = mapFromSource(source_parent);
                return m_unlimitedChildren.contains(parent);
            }

            return source_row < m_childLimit;
        }

        return true;
    }

    if (m_rootLimit > 0) {
        return source_row < m_rootLimit;
    }

    return true;
}
