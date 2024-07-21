/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "declarativehistorymodel.h"
#include "historymodel.h"

DeclarativeHistoryModel::DeclarativeHistoryModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , m_model(HistoryModel::self())
{
    setSourceModel(m_model.get());

    connect(this, &QIdentityProxyModel::rowsInserted, this, &DeclarativeHistoryModel::countChanged);
    connect(this, &QIdentityProxyModel::rowsRemoved, this, &DeclarativeHistoryModel::countChanged);
    connect(this, &QIdentityProxyModel::modelReset, this, &DeclarativeHistoryModel::countChanged);
}

DeclarativeHistoryModel::~DeclarativeHistoryModel()
{
}

void DeclarativeHistoryModel::clearHistory()
{
    m_model->clearHistory();
}

#include "moc_declarativehistorymodel.cpp"
