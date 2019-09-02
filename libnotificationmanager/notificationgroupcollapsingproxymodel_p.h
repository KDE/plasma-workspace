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

#pragma once

#include <QSortFilterProxyModel>
#include <QDateTime>

namespace NotificationManager {

class NotificationGroupCollapsingProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit NotificationGroupCollapsingProxyModel(QObject *parent = nullptr);
    ~NotificationGroupCollapsingProxyModel() override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    int limit() const;
    void setLimit(int limit);

    QDateTime lastRead() const;
    void setLastRead(const QDateTime &lastRead);

    bool expandUnread() const;
    void setExpandUnread(bool expand);

    void collapseAll();

signals:
    void limitChanged();
    void lastReadChanged();
    void expandUnreadChanged();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    bool setGroupExpanded(const QModelIndex &idx, bool expanded);

    void invalidateGroupRoles();

    int m_limit = 0;
    QDateTime m_lastRead;
    bool m_expandUnread = false;

    QList<QPersistentModelIndex> m_expandedGroups;

};

} // namespace NotificationManager
