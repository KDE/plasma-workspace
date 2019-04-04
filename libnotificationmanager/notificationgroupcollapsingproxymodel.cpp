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

#include "notificationgroupcollapsingproxymodel_p.h"

#include  "notifications.h"

#include "debug.h"

using namespace NotificationManager;

NotificationGroupCollapsingProxyModel::NotificationGroupCollapsingProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

NotificationGroupCollapsingProxyModel::~NotificationGroupCollapsingProxyModel() = default;

void NotificationGroupCollapsingProxyModel::setSourceModel(QAbstractItemModel *source)
{
    if (sourceModel()) {
        disconnect(sourceModel(), nullptr, this, nullptr);
    }

    QSortFilterProxyModel::setSourceModel(source);

    if (source) {
        connect(source, &QAbstractItemModel::rowsInserted, this, &NotificationGroupCollapsingProxyModel::invalidateFilter);
        connect(source, &QAbstractItemModel::rowsRemoved, this, &NotificationGroupCollapsingProxyModel::invalidateFilter);

        // When a group is removed, there is no item that's being removed, instead the item morphs back into a single notification
        connect(source, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
            Q_UNUSED(bottomRight); // what about it?
            Q_UNUSED(roles);

            if (roles.isEmpty() || roles.contains(Notifications::IsGroupRole)) {
                if (!topLeft.data(Notifications::IsGroupRole).toBool()) {
                    const QModelIndex proxyIdx = mapFromSource(topLeft);
                    if (m_expandedGroups.contains(proxyIdx)) {
                        setGroupExpanded(proxyIdx, false);
                    }
                }
            }
        });
    }
}

QVariant NotificationGroupCollapsingProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == NotificationManager::Notifications::IsGroupExpandedRole) {
        if (m_limit > 0) {
            // so each item in a group knows whether the group is expanded
            const QModelIndex parentIdx = index.parent();
            if (parentIdx.isValid()) {
                return m_expandedGroups.contains(parentIdx);
            } else {
                return m_expandedGroups.contains(index);
            }
        } else {
            return true;
        }
    }

    return QSortFilterProxyModel::data(index, role);
}

bool NotificationGroupCollapsingProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Notifications::IsGroupExpandedRole && m_limit > 0) {
        QModelIndex groupIdx = index;
        // so an item inside a group can expand/collapse the group
        if (groupIdx.parent().isValid()) {
            groupIdx = groupIdx.parent();
        }

        const bool expanded = value.toBool();
        if (!groupIdx.data(Notifications::IsGroupRole).toBool()) {
            qCWarning(NOTIFICATIONMANAGER) << "Cannot" << (expanded ? "expand" : "collapse") << "an item isn't a group or inside of one";
            return false;
        }

        return setGroupExpanded(groupIdx, expanded);
    }

    return false;
}

int NotificationGroupCollapsingProxyModel::limit() const
{
    return m_limit;
}

void NotificationGroupCollapsingProxyModel::setLimit(int limit)
{
    if (m_limit != limit) {
        m_limit = limit;
        invalidateFilter();
        emit limitChanged();
    }
}

bool NotificationGroupCollapsingProxyModel::setGroupExpanded(const QModelIndex &idx, bool expanded)
{
    if (idx.data(Notifications::IsGroupExpandedRole).toBool() == expanded) {
        return false;
    }

    QPersistentModelIndex persistentIdx(idx);
    if (expanded) {
        m_expandedGroups.append(persistentIdx);
    } else {
        m_expandedGroups.removeOne(persistentIdx);
    }

    invalidateFilter();

    emit dataChanged(idx, idx, {Notifications::IsGroupExpandedRole});

    // also signal the children
    emit dataChanged(idx.child(0, 0), idx.child(rowCount(idx) - 1, 0), {Notifications::IsGroupExpandedRole});

    return true;
}

bool NotificationGroupCollapsingProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (source_parent.isValid() && m_limit > 0) {
        if (!m_expandedGroups.isEmpty() && m_expandedGroups.contains(mapFromSource(source_parent))) {
            return true;
        }

        // should we raise the limit when there's just one group?

        // FIXME why is this reversed?
        // grouping proxy model seems to reverse the order?
        return source_row >= sourceModel()->rowCount(source_parent) - m_limit;
    }

    return true;
}
