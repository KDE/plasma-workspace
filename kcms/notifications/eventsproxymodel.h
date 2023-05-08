/*
 *    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>
 *
 *    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractProxyModel>

class EventsProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)

public:
    explicit EventsProxyModel(QObject *parent);

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &index) const override;

    QModelIndex rootIndex() const;
    void setRootIndex(const QModelIndex &index);
    Q_SIGNAL void rootIndexChanged();

private Q_SLOTS:
    void updateDataConnection();

private:
    QModelIndex m_rootIndex;
};
