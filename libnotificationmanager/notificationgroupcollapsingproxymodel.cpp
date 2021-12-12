/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "notificationgroupcollapsingproxymodel_p.h"

#include "notifications.h"

#include "debug.h"

using namespace NotificationManager;

NotificationGroupCollapsingProxyModel::NotificationGroupCollapsingProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

NotificationGroupCollapsingProxyModel::~NotificationGroupCollapsingProxyModel() = default;

void NotificationGroupCollapsingProxyModel::setSourceModel(QAbstractItemModel *source)
{
    if (source == QAbstractProxyModel::sourceModel()) {
        return;
    }

    if (QAbstractProxyModel::sourceModel()) {
        disconnect(QAbstractProxyModel::sourceModel(), nullptr, this, nullptr);
    }

    QSortFilterProxyModel::setSourceModel(source);

    if (source) {
        connect(source, &QAbstractItemModel::rowsInserted, this, &NotificationGroupCollapsingProxyModel::invalidateFilter);
        connect(source, &QAbstractItemModel::rowsRemoved, this, &NotificationGroupCollapsingProxyModel::invalidateFilter);

        // When a group is removed, there is no item that's being removed, instead the item morphs back into a single notification
        connect(source,
                &QAbstractItemModel::dataChanged,
                this,
                [this, source](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
                    if (roles.isEmpty() || roles.contains(Notifications::IsGroupRole)) {
                        for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
                            const QModelIndex sourceIdx = source->index(i, 0);

                            if (!sourceIdx.data(Notifications::IsGroupRole).toBool()) {
                                if (m_expandedGroups.contains(sourceIdx)) {
                                    setGroupExpanded(topLeft, false);
                                }
                            }
                        }
                    }
                });
    }
}

QVariant NotificationGroupCollapsingProxyModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Notifications::IsGroupExpandedRole: {
        if (m_limit > 0) {
            // so each item in a group knows whether the group is expanded
            const QModelIndex sourceIdx = mapToSource(index);
            return m_expandedGroups.contains(sourceIdx.parent().isValid() ? sourceIdx.parent() : sourceIdx);
        }
        return true;
    }
    case Notifications::ExpandedGroupChildrenCountRole:
        return rowCount(index.parent().isValid() ? index.parent() : index);
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

    return QSortFilterProxyModel::setData(index, value, role);
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
        invalidateGroupRoles();
        Q_EMIT limitChanged();
    }
}

QDateTime NotificationGroupCollapsingProxyModel::lastRead() const
{
    return m_lastRead;
}

void NotificationGroupCollapsingProxyModel::setLastRead(const QDateTime &lastRead)
{
    if (m_lastRead != lastRead) {
        m_lastRead = lastRead;
        invalidateFilter();
        invalidateGroupRoles();
        Q_EMIT lastReadChanged();
    }
}

bool NotificationGroupCollapsingProxyModel::expandUnread() const
{
    return m_expandUnread;
}

void NotificationGroupCollapsingProxyModel::setExpandUnread(bool expand)
{
    if (m_expandUnread != expand) {
        m_expandUnread = expand;
        invalidateFilter();
        invalidateGroupRoles();
        Q_EMIT expandUnreadChanged();
    }
}

void NotificationGroupCollapsingProxyModel::collapseAll()
{
    m_expandedGroups.clear();

    invalidateFilter();
    invalidateGroupRoles();
}

bool NotificationGroupCollapsingProxyModel::setGroupExpanded(const QModelIndex &idx, bool expanded)
{
    if (idx.data(Notifications::IsGroupExpandedRole).toBool() == expanded) {
        return false;
    }

    QPersistentModelIndex persistentIdx(mapToSource(idx));
    if (expanded) {
        m_expandedGroups.append(persistentIdx);
    } else {
        m_expandedGroups.removeOne(persistentIdx);
    }

    invalidateFilter();

    const QVector<int> dirtyRoles = {Notifications::ExpandedGroupChildrenCountRole, Notifications::IsGroupExpandedRole};

    Q_EMIT dataChanged(idx, idx, dirtyRoles);
    Q_EMIT dataChanged(index(0, 0, idx), index(rowCount(idx) - 1, 0, idx), dirtyRoles);

    return true;
}

void NotificationGroupCollapsingProxyModel::invalidateGroupRoles()
{
    const QVector<int> dirtyRoles = {Notifications::ExpandedGroupChildrenCountRole, Notifications::IsGroupExpandedRole};

    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0), dirtyRoles);

    for (int row = 0; row < rowCount(); ++row) {
        const QModelIndex groupIdx = index(row, 0);
        Q_EMIT dataChanged(index(0, 0, groupIdx), index(rowCount(groupIdx) - 1, 0, groupIdx), dirtyRoles);
    }
}

bool NotificationGroupCollapsingProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (m_limit > 0 && source_parent.isValid()) {
        if (!m_expandedGroups.isEmpty() && m_expandedGroups.contains(source_parent)) {
            return true;
        }

        if (m_expandUnread && m_lastRead.isValid()) {
            const QModelIndex sourceIdx = sourceModel()->index(source_row, 0, source_parent);

            if (!sourceIdx.data(Notifications::ReadRole).toBool()) {
                QDateTime time = sourceIdx.data(Notifications::UpdatedRole).toDateTime();
                if (!time.isValid()) {
                    time = sourceIdx.data(Notifications::CreatedRole).toDateTime();
                }

                if (time.isValid() && m_lastRead < time) {
                    return true;
                }
            }
        }

        // should we raise the limit when there's just one group?

        // FIXME why is this reversed?
        // grouping proxy model seems to reverse the order?
        return source_row >= sourceModel()->rowCount(source_parent) - m_limit;
    }

    return true;
}
