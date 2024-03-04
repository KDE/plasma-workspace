/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "filterproxymodel.h"

#include "kcm.h"
#include "sourcesmodel.h"

FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setRecursiveFilteringEnabled(true);
}

FilterProxyModel::~FilterProxyModel() = default;

QString FilterProxyModel::query() const
{
    return m_query;
}

void FilterProxyModel::setQuery(const QString &query)
{
    if (m_query != query) {
        m_query = query;
        invalidateFilter();
        Q_EMIT queryChanged();
    }
}

bool FilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (m_query.isEmpty()) {
        return true;
    }

    const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

    // We remove "Plasma Workspace" in the filter model, so it's not shown in the apps list
    // but still addressable by model index in the root model ("System notificatons" settings).
    if (idx.data(SourcesModel::NotifyRcNameRole).toString() == KCMNotifications::plasmaWorkspaceNotifyRcName()) {
        return false;
    }

    const QString display = idx.data(Qt::DisplayRole).toString();
    if (display.contains(m_query, Qt::CaseInsensitive)) {
        return true;
    }

    return false;
}
