/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDateTime>
#include <QSortFilterProxyModel>

namespace NotificationManager
{
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

Q_SIGNALS:
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
