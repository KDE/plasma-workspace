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

    // Connect source model changes to sourceCountChanged signal
    connect(m_model.get(), &HistoryModel::rowsInserted, this, &DeclarativeHistoryModel::sourceCountChanged);
    connect(m_model.get(), &HistoryModel::rowsRemoved, this, &DeclarativeHistoryModel::sourceCountChanged);
    connect(m_model.get(), &HistoryModel::modelReset, this, &DeclarativeHistoryModel::sourceCountChanged);

    m_starredCountNotifier = m_model->bindableStarredCount().addNotifier([this] {
        if (m_starredOnly && m_model->bindableStarredCount().value() == 0) {
            // If there are no starred items and we're in starred-only mode, switch to all history
            setStarredOnly(false);
        }
        Q_EMIT starredCountChanged();
    });

    connect(m_model.get(), &HistoryModel::changed, this, &DeclarativeHistoryModel::currentTextChanged);
}

DeclarativeHistoryModel::~DeclarativeHistoryModel() = default;

QString DeclarativeHistoryModel::currentText() const
{
    return m_model->rowCount() == 0 ? QString() : m_model->index(0).data(Qt::DisplayRole).toString();
}

int DeclarativeHistoryModel::sourceCount() const
{
    return m_model->rowCount();
}

int DeclarativeHistoryModel::starredCount() const
{
    return m_model->bindableStarredCount().value();
}

bool DeclarativeHistoryModel::starredOnly() const
{
    return m_starredOnly;
}

void DeclarativeHistoryModel::setStarredOnly(bool value)
{
    if (m_starredOnly == value) {
        return;
    }
    m_starredOnly = value;
    invalidateRowsFilter();
    Q_EMIT starredOnlyChanged();
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

bool DeclarativeHistoryModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_starredOnly) {
        return m_model->index(sourceRow, 0, sourceParent).data(HistoryModel::StarredRole).toBool();
    }

    return true;
}

#include "moc_declarativehistorymodel.cpp"
