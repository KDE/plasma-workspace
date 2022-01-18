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
    case HistoryItemConstPtrRole:
        return QVariant::fromValue<HistoryItemConstPtr>(qSharedPointerConstCast<const HistoryItem>(item));
    case UuidRole:
        return item->uuid();
    case TypeRole:
        return QVariant::fromValue<HistoryItemType>(type);
    case Base64UuidRole:
        return item->uuid().toBase64();
    case TypeIntRole:
        return int(type);
    case FullTextRole:
        return item->mimeData()->text();
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
    hash.insert(Base64UuidRole, QByteArrayLiteral("UuidRole"));
    hash.insert(TypeIntRole, QByteArrayLiteral("TypeRole"));
    hash.insert(FullTextRole, QByteArrayLiteral("FullTextRole"));
    return hash;
}
