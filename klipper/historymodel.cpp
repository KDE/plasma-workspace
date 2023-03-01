/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "historymodel.h"
#include "historyimageitem.h"
#include "historystringitem.h"
#include "historyurlitem.h"

HistoryModel::HistoryModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_maxSize(0)
    , m_displayImages(true)
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

    switch (role) {
    case Qt::DisplayRole:
        return item->text();
    case Qt::DecorationRole:
        return item->image();
    case HistoryItemConstPtrRole:
        return QVariant::fromValue<HistoryItemConstPtr>(qSharedPointerConstCast<const HistoryItem>(item));
    case UuidRole:
        return item->uuid();
    case TypeRole:
        return QVariant::fromValue<HistoryItemType>(item->type());
    case Base64UuidRole:
        return item->uuid().toBase64();
    case TypeIntRole:
        return int(item->type());
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

    if (m_maxSize == 0) {
        // special case - cannot insert any items
        return;
    }

    QMutexLocker lock(&m_mutex);

    const QModelIndex existingItem = indexOf(item.get());
    if (existingItem.isValid()) {
        // move to top
        moveToTop(existingItem.row());
        return;
    }

    beginInsertRows(QModelIndex(), 0, 0);
    item->setModel(this);
    m_items.prepend(item);
    endInsertRows();

    if (m_items.count() > m_maxSize) {
        beginRemoveRows(QModelIndex(), m_items.count() - 1, m_items.count() - 1);
        m_items.removeLast();
        endRemoveRows();
    }
}

void HistoryModel::clearAndBatchInsert(const QVector<HistoryItemPtr> &items)
{
    if (m_maxSize == 0) {
        // special case - cannot insert any items
        return;
    }

    if (items.empty()) {
        // special case - nothing to insert, so just clear.
        clear();
        return;
    }

    QMutexLocker lock(&m_mutex);

    beginResetModel();
    m_items.clear();

    // The last row is either items.size() - 1 or m_maxSize - 1.
    const int numOfItemsToBeInserted = std::min(static_cast<int>(items.size()), m_maxSize);
    m_items.reserve(numOfItemsToBeInserted);

    for (int i = 0; i < numOfItemsToBeInserted; i++) {
        if (items[i].isNull()) {
            continue;
        }

        items[i]->setModel(this);
        m_items.append(items[i]);
    }

    endResetModel();
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
    hash.insert(Base64UuidRole, QByteArrayLiteral("UuidRole"));
    hash.insert(TypeIntRole, QByteArrayLiteral("TypeRole"));
    return hash;
}
