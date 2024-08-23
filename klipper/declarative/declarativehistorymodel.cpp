/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "declarativehistorymodel.h"
#include "historyitem.h"
#include "historymodel.h"

DeclarativeHistoryModel::DeclarativeHistoryModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , m_model(HistoryModel::self())
{
    setSourceModel(m_model.get());

    connect(this, &QIdentityProxyModel::rowsInserted, this, &DeclarativeHistoryModel::countChanged);
    connect(this, &QIdentityProxyModel::rowsRemoved, this, &DeclarativeHistoryModel::countChanged);
    connect(this, &QIdentityProxyModel::modelReset, this, &DeclarativeHistoryModel::countChanged);
    connect(m_model.get(), &HistoryModel::changed, this, &DeclarativeHistoryModel::currentTextChanged);
}

DeclarativeHistoryModel::~DeclarativeHistoryModel()
{
}

QString DeclarativeHistoryModel::currentText() const
{
    return m_model->rowCount() == 0 ? QString() : m_model->index(0).data(Qt::DisplayRole).toString();
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

#include "moc_declarativehistorymodel.cpp"
