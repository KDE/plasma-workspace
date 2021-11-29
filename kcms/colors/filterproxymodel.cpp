/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "filterproxymodel.h"

#include "colorsmodel.h"

FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

FilterProxyModel::~FilterProxyModel() = default;

QString FilterProxyModel::selectedScheme() const
{
    return m_selectedScheme;
}

void FilterProxyModel::setSelectedScheme(const QString &scheme)
{
    if (m_selectedScheme == scheme) {
        return;
    }

    m_selectedScheme = scheme;

    Q_EMIT selectedSchemeChanged();
    Q_EMIT selectedSchemeIndexChanged();
}

int FilterProxyModel::selectedSchemeIndex() const
{
    // We must search in the source model and then map the index to our proxy model.
    const auto results = sourceModel()->match(sourceModel()->index(0, 0), ColorsModel::SchemeNameRole, m_selectedScheme, 1, Qt::MatchExactly);

    if (results.count() == 1) {
        const QModelIndex result = mapFromSource(results.first());
        if (result.isValid()) {
            return result.row();
        }
    }

    return -1;
}

QString FilterProxyModel::query() const
{
    return m_query;
}

void FilterProxyModel::setQuery(const QString &query)
{
    if (m_query != query) {
        const int oldIndex = selectedSchemeIndex();

        m_query = query;
        invalidateFilter();

        Q_EMIT queryChanged();

        if (selectedSchemeIndex() != oldIndex) {
            Q_EMIT selectedSchemeIndexChanged();
        }
    }
}

KCMColors::SchemeFilter FilterProxyModel::filter() const
{
    return m_filter;
}

void FilterProxyModel::setFilter(KCMColors::SchemeFilter filter)
{
    if (m_filter != filter) {
        const int oldIndex = selectedSchemeIndex();

        m_filter = filter;
        invalidateFilter();

        Q_EMIT filterChanged();

        if (selectedSchemeIndex() != oldIndex) {
            Q_EMIT selectedSchemeIndexChanged();
        }
    }
}

bool FilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

    if (!m_query.isEmpty()) {
        if (!idx.data(Qt::DisplayRole).toString().contains(m_query, Qt::CaseInsensitive)
            && !idx.data(ColorsModel::SchemeNameRole).toString().contains(m_query, Qt::CaseInsensitive)) {
            return false;
        }
    }

    if (m_filter != KCMColors::AllSchemes) {
        const QPalette palette = idx.data(ColorsModel::PaletteRole).value<QPalette>();

        const int windowBackgroundGray = qGray(palette.window().color().rgb());

        if (m_filter == KCMColors::DarkSchemes) {
            return windowBackgroundGray < 192;
        } else if (m_filter == KCMColors::LightSchemes) {
            return windowBackgroundGray >= 192;
        }
    }

    return true;
}
