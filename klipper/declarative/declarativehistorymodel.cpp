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
}

DeclarativeHistoryModel::~DeclarativeHistoryModel()
{
}

#include "moc_declarativehistorymodel.cpp"
