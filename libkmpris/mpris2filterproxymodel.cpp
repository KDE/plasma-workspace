/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "mpris2filterproxymodel.h"

#include "mpris2sourcemodel.h"

std::shared_ptr<Mpris2FilterProxyModel> Mpris2FilterProxyModel::self()
{
    static std::weak_ptr<Mpris2FilterProxyModel> s_model;
    if (s_model.expired()) {
        std::shared_ptr<Mpris2FilterProxyModel> ptr{new Mpris2FilterProxyModel};
        s_model = ptr;
        return ptr;
    }

    return s_model.lock();
}

Mpris2FilterProxyModel::Mpris2FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_sourceModel(Mpris2SourceModel::self())
{
    // Must update m_proxyPidList before QSortFilterProxyModel receives updates from the source model,
    // so when filterAcceptsRow is called, m_proxyPidList is the latest
    connect(m_sourceModel.get(), &QAbstractListModel::rowsInserted, this, &Mpris2FilterProxyModel::onRowsInserted);
    connect(m_sourceModel.get(), &QAbstractListModel::rowsAboutToBeRemoved, this, &Mpris2FilterProxyModel::onRowsAboutToBeRemoved);
    connect(m_sourceModel.get(), &QAbstractListModel::dataChanged, this, &Mpris2FilterProxyModel::onDataChanged);
    setSourceModel(m_sourceModel.get());
    connect(m_sourceModel.get(), &QAbstractListModel::rowsInserted, this, &Mpris2FilterProxyModel::invalidateRowsFilter);
    connect(m_sourceModel.get(), &QAbstractListModel::rowsRemoved, this, &Mpris2FilterProxyModel::invalidateRowsFilter);
    connect(m_sourceModel.get(), &QAbstractListModel::dataChanged, this, &Mpris2FilterProxyModel::invalidateRowsFilter);
}

Mpris2FilterProxyModel::~Mpris2FilterProxyModel()
{
}

bool Mpris2FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
    if (m_proxyPidList.empty()) {
        return true;
    }

    const unsigned instancePid = m_sourceModel->index(sourceRow).data(Mpris2SourceModel::InstancePidRole).toUInt();
    return !m_proxyPidList.contains(instancePid);
}

void Mpris2FilterProxyModel::onRowsInserted(const QModelIndex &, int first, int)
{
    if (const unsigned proxyPid = m_sourceModel->index(first).data(Mpris2SourceModel::KDEPidRole).toUInt(); proxyPid > 0) {
        m_proxyPidList.emplace(proxyPid);
    }
}

void Mpris2FilterProxyModel::onRowsAboutToBeRemoved(const QModelIndex &, int first, int)
{
    if (const unsigned proxyPid = m_sourceModel->index(first).data(Mpris2SourceModel::KDEPidRole).toUInt(); proxyPid > 0) {
        m_proxyPidList.erase(proxyPid);
        // invalidateRowsFilter is called after rowsRemoved
    }
}

void Mpris2FilterProxyModel::onDataChanged(const QModelIndex &topLeft, const QModelIndex &, const QList<int> &roles)
{
    if (!roles.contains(Mpris2SourceModel::KDEPidRole)) {
        return;
    }

    if (const unsigned proxyPid = topLeft.data(Mpris2SourceModel::KDEPidRole).toUInt(); proxyPid > 0) {
        m_proxyPidList.emplace(proxyPid);
    }
}

#include "moc_mpris2filterproxymodel.cpp"