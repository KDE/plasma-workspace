/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "declarativehistorymodel.h"
#include "historyitem.h"
#include "historymodel.h"

DeclarativeHistoryModel::DeclarativeHistoryModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_model(HistoryModel::self())
{
    setSourceModel(m_model.get());

    connect(this, &QSortFilterProxyModel::rowsInserted, this, &DeclarativeHistoryModel::countChanged);
    connect(this, &QSortFilterProxyModel::rowsRemoved, this, &DeclarativeHistoryModel::countChanged);
    connect(this, &QSortFilterProxyModel::modelReset, this, &DeclarativeHistoryModel::countChanged);
    connect(m_model.get(), &HistoryModel::changed, this, &DeclarativeHistoryModel::currentTextChanged);
}

DeclarativeHistoryModel::~DeclarativeHistoryModel()
{
}

QString DeclarativeHistoryModel::currentText() const
{
    return m_model->rowCount() == 0 ? QString() : m_model->index(0).data(Qt::DisplayRole).toString();
}

bool DeclarativeHistoryModel::pinnedOnly() const
{
    return m_pinnedOnly;
}

void DeclarativeHistoryModel::setPinnedOnly(bool value)
{
    if (m_pinnedOnly == value) {
        return;
    }

    m_pinnedOnly = value;
    invalidateRowsFilter();
    Q_EMIT pinnedOnlyChanged();
}

bool DeclarativeHistoryModel::pinnedPrioritized() const
{
    return m_pinnedOnly;
}

void DeclarativeHistoryModel::setPinnedPrioritized(bool value)
{
    if (m_pinnedPrioritized == value) {
        return;
    }

    m_pinnedPrioritized = value;
    invalidateRowsFilter();
    Q_EMIT pinnedPrioritizedChanged();
}

void DeclarativeHistoryModel::moveToTop(const QString &uuid)
{
    m_model->moveToTop(uuid);
}

void DeclarativeHistoryModel::remove(const QString &uuid)
{
    m_model->remove(uuid);
}

void DeclarativeHistoryModel::clearHistory()
{
    m_model->clearHistory();
}

void DeclarativeHistoryModel::invokeAction(const QString &uuid)
{
    if (const qsizetype row = m_model->indexOf(uuid); row >= 0) {
        Q_EMIT m_model->actionInvoked(m_model->m_items[row]);
    }
}

bool DeclarativeHistoryModel::filterAcceptsRow(int sourceRow, const QModelIndex & /*sourceParent*/) const
{
    if (m_pinnedOnly) {
        return index(sourceRow, 0).data(HistoryModel::PinnedRole).toBool();
    }

    return true;
}

bool DeclarativeHistoryModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    const int leftRow = source_left.row();
    const int rightRow = source_right.row();

    // Always prioritize the current clipboard item
    if (leftRow == 0) {
        return true;
    } else if (rightRow == 0) {
        return false;
    }

    if (m_pinnedPrioritized) {
        const bool leftPinned = source_left.data(HistoryModel::PinnedRole).toBool();
        const bool rightPinned = source_right.data(HistoryModel::PinnedRole).toBool();

        if (leftPinned && !rightPinned) {
            return true;
        }

        if (rightPinned && !leftPinned) {
            return false;
        }
    }

    return leftRow < rightRow;
}

#include "moc_declarativehistorymodel.cpp"
