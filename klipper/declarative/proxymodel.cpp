/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "proxymodel.h"

#include "historymodel.h"

ProxyModel::ProxyModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , m_model(HistoryModel::self())
{
    setSourceModel(m_model);
}

QHash<int, QByteArray> ProxyModel::roleNames() const
{
    return m_model->roleNames();
}

#include "moc_proxymodel.cpp"
