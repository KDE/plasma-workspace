/*
 * SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "eventsproxymodel.h"

EventsProxyModel::EventsProxyModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
    connect(this, &QAbstractProxyModel::sourceModelChanged, this, &EventsProxyModel::updateDataConnection);
}

void EventsProxyModel::updateDataConnection()
{
    connect(sourceModel(), &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
        if (topLeft.parent() == m_rootIndex && bottomRight.parent() == m_rootIndex) {
            Q_EMIT dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
        }
    });
    connect(sourceModel(), &QAbstractItemModel::modelAboutToBeReset, this, &EventsProxyModel::beginResetModel);
    connect(sourceModel(), &QAbstractItemModel::modelReset, this, &EventsProxyModel::endResetModel);
}

int EventsProxyModel::rowCount(const QModelIndex &index) const
{
    return sourceModel()->rowCount(mapToSource(index));
}

int EventsProxyModel::columnCount(const QModelIndex &index) const
{
    return sourceModel()->columnCount(mapToSource(index));
}

QModelIndex EventsProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column, parent.internalId());
}

QModelIndex EventsProxyModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

QModelIndex EventsProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid() || !m_rootIndex.isValid() || sourceIndex.parent() != m_rootIndex) {
        return QModelIndex();
    }

    return index(sourceIndex.row(), sourceIndex.column(), QModelIndex());
}

QModelIndex EventsProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!m_rootIndex.isValid()) {
        return QModelIndex();
    }
    if (!proxyIndex.isValid()) { // root index, we want to return the parent's leaf
        return m_rootIndex;
    }
    return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), m_rootIndex);
}

QModelIndex EventsProxyModel::rootIndex() const
{
    return m_rootIndex;
}

void EventsProxyModel::setRootIndex(const QModelIndex &index)
{
    if (m_rootIndex == index) {
        return;
    }
    beginResetModel();
    m_rootIndex = index;
    Q_EMIT rootIndexChanged();
    endResetModel();
}
