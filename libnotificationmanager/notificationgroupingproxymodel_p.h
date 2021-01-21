/*
 * Copyright 2016 Eike Hein <hein@kde.org>
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

#include <QAbstractProxyModel>

namespace NotificationManager
{
class NotificationGroupingProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    explicit NotificationGroupingProxyModel(QObject *parent = nullptr);
    ~NotificationGroupingProxyModel() override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &proxyIndex, int role) const override;

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

protected:
    // bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

private:
    bool appsMatch(const QModelIndex &a, const QModelIndex &b) const;
    bool isGroup(int row) const;
    bool tryToGroup(const QModelIndex &sourceIndex, bool silent = false);
    void adjustMap(int anchor, int delta);
    void rebuildMap();
    void checkGrouping(bool silent = false);
    void formGroupFor(const QModelIndex &index);

    QVector<QVector<int> *> rowMap;
};

} // namespace NotificationManager
