/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "historymodel.h"
#include "historyimageitem.h"
#include "historystringitem.h"
#include "historyurlitem.h"

HistoryModel::HistoryModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_maxSize(0)
    , m_displayImages(true)
    , m_mutex(QMutex::Recursive)
{
}

HistoryModel::~HistoryModel()
{
    clear();
}

void HistoryModel::clear()
{
    QMutexLocker lock(&m_mutex);
    beginResetModel();
    m_items.clear();
    endResetModel();
}

void HistoryModel::setMaxSize(int size)
{
    if (m_maxSize == size) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    m_maxSize = size;
    if (m_items.count() > m_maxSize) {
        removeRows(m_maxSize, m_items.count() - m_maxSize);
    }
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_items.count();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.count() || index.column() != 0) {
        return QVariant();
    }

    QSharedPointer<HistoryItem> item = m_items.at(index.row());
    HistoryItemType type = HistoryItemType::Text;
    if (dynamic_cast<HistoryStringItem *>(item.data())) {
        type = HistoryItemType::Text;
    } else if (dynamic_cast<HistoryImageItem *>(item.data())) {
        type = HistoryItemType::Image;
    } else if (dynamic_cast<HistoryURLItem *>(item.data())) {
        type = HistoryItemType::Url;
    }

    switch (role) {
    case Qt::DisplayRole:
        return item->text();
    case Qt::DecorationRole:
        return item->image();
    case Qt::UserRole:
        return QVariant::fromValue<HistoryItemConstPtr>(qSharedPointerConstCast<const HistoryItem>(item));
    case Qt::UserRole + 1:
        return item->uuid();
    case Qt::UserRole + 2:
        return QVariant::fromValue<HistoryItemType>(type);
    case Qt::UserRole + 3:
        return item->uuid().toBase64();
    case Qt::UserRole + 4:
        return int(type);
    }
    return QVariant();
}

bool HistoryModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid()) {
        return false;
    }
    if ((row + count) > m_items.count()) {
        return false;
    }
    QMutexLocker lock(&m_mutex);
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        m_items.removeAt(row);
    }
    endRemoveRows();
    return true;
}

bool HistoryModel::remove(const QByteArray &uuid)
{
    QModelIndex index = indexOf(uuid);
    if (!index.isValid()) {
        return false;
    }
    return removeRow(index.row(), QModelIndex());
}

QModelIndex HistoryModel::indexOf(const QByteArray &uuid) const
{
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items.at(i)->uuid() == uuid) {
            return index(i);
        }
    }
    return QModelIndex();
}

QModelIndex HistoryModel::indexOf(const HistoryItem *item) const
{
    if (!item) {
        return QModelIndex();
    }
    return indexOf(item->uuid());
}

void HistoryModel::insert(QSharedPointer<HistoryItem> item)
{
    if (item.isNull()) {
        return;
    }
    const QModelIndex existingItem = indexOf(item.data());
    if (existingItem.isValid()) {
        // move to top
        moveToTop(existingItem.row());
        return;
    }

    QMutexLocker lock(&m_mutex);
    if (m_items.count() == m_maxSize) {
        // remove last item
        if (m_maxSize == 0) {
            // special case - cannot insert any items
            return;
        }
        beginRemoveRows(QModelIndex(), m_items.count() - 1, m_items.count() - 1);
        m_items.removeLast();
        endRemoveRows();
    }

    beginInsertRows(QModelIndex(), 0, 0);
    item->setModel(this);
    m_items.prepend(item);
    endInsertRows();
}

void HistoryModel::moveToTop(const QByteArray &uuid)
{
    const QModelIndex existingItem = indexOf(uuid);
    if (!existingItem.isValid()) {
        return;
    }
    moveToTop(existingItem.row());
}

void HistoryModel::moveToTop(int row)
{
    if (row == 0 || row >= m_items.count()) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0);
    m_items.move(row, 0);
    endMoveRows();
}

void HistoryModel::moveTopToBack()
{
    if (m_items.count() < 2) {
        return;
    }
    QMutexLocker lock(&m_mutex);
    beginMoveRows(QModelIndex(), 0, 0, QModelIndex(), m_items.count());
    auto item = m_items.takeFirst();
    m_items.append(item);
    endMoveRows();
}

void HistoryModel::moveBackToTop()
{
    moveToTop(m_items.count() - 1);
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    QHash<int, QByteArray> hash;
    hash.insert(Qt::DisplayRole, QByteArrayLiteral("DisplayRole"));
    hash.insert(Qt::DecorationRole, QByteArrayLiteral("DecorationRole"));
    hash.insert(Qt::UserRole + 3, QByteArrayLiteral("UuidRole"));
    hash.insert(Qt::UserRole + 4, QByteArrayLiteral("TypeRole"));
    return hash;
}
