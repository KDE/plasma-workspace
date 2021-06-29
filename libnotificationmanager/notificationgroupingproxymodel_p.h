/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
