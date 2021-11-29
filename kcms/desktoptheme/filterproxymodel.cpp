/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "filterproxymodel.h"

#include "themesmodel.h"

FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

FilterProxyModel::~FilterProxyModel() = default;

QString FilterProxyModel::selectedTheme() const
{
    return m_selectedTheme;
}

void FilterProxyModel::setSelectedTheme(const QString &pluginName)
{
    if (m_selectedTheme == pluginName) {
        return;
    }

    const bool firstTime = m_selectedTheme.isNull();
    m_selectedTheme = pluginName;

    if (!firstTime) {
        Q_EMIT selectedThemeChanged();
    }
    Q_EMIT selectedThemeIndexChanged();
}

int FilterProxyModel::selectedThemeIndex() const
{
    // We must search in the source model and then map the index to our proxy model.
    const auto results = sourceModel()->match(sourceModel()->index(0, 0), ThemesModel::PluginNameRole, m_selectedTheme, 1, Qt::MatchExactly);

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
        const int oldIndex = selectedThemeIndex();

        m_query = query;
        invalidateFilter();

        Q_EMIT queryChanged();

        if (selectedThemeIndex() != oldIndex) {
            Q_EMIT selectedThemeIndexChanged();
        }
    }
}

FilterProxyModel::ThemeFilter FilterProxyModel::filter() const
{
    return m_filter;
}

void FilterProxyModel::setFilter(ThemeFilter filter)
{
    if (m_filter != filter) {
        const int oldIndex = selectedThemeIndex();

        m_filter = filter;
        invalidateFilter();

        Q_EMIT filterChanged();

        if (selectedThemeIndex() != oldIndex) {
            Q_EMIT selectedThemeIndexChanged();
        }
    }
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

    if (!m_query.isEmpty()) {
        if (!idx.data(Qt::DisplayRole).toString().contains(m_query, Qt::CaseInsensitive)
            && !idx.data(ThemesModel::PluginNameRole).toString().contains(m_query, Qt::CaseInsensitive)) {
            return false;
        }
    }

    const auto type = idx.data(ThemesModel::ColorTypeRole).value<ThemesModel::ColorType>();
    switch (m_filter) {
    case AllThemes:
        return true;
    case LightThemes:
        return type == ThemesModel::LightTheme;
    case DarkThemes:
        return type == ThemesModel::DarkTheme;
    case ThemesFollowingColors:
        return type == ThemesModel::FollowsColorTheme;
    }

    return true;
}
