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
    const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

    // We remove "Plasma Workspace" in the filter model, so it's not shown in the apps list
    // but still addressable by model index in the root model ("System notifications" settings).
    QString notifyRcName = idx.data(SourcesModel::NotifyRcNameRole).toString();
    // Recursive filtering would still show it because its events never have a notifyRcName.
    if (notifyRcName.isEmpty() && source_parent.isValid()) {
        notifyRcName = source_parent.data(SourcesModel::NotifyRcNameRole).toString();
    }

    if (notifyRcName == KCMNotifications::plasmaWorkspaceNotifyRcName()) {
        return false;
    }

    if (m_query.isEmpty()) {
        return true;
    }

    const QString display = idx.data(Qt::DisplayRole).toString();
    if (display.contains(m_query, Qt::CaseInsensitive)) {
        return true;
    }

    return false;
}
