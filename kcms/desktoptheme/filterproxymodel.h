/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QSortFilterProxyModel>

#include "kcm.h"

class FilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum ThemeFilter {
        AllThemes,
        LightThemes,
        DarkThemes,
        ThemesFollowingColors,
    };
    Q_ENUM(ThemeFilter)

    Q_PROPERTY(QString selectedTheme READ selectedTheme WRITE setSelectedTheme NOTIFY selectedThemeChanged)
    Q_PROPERTY(int selectedThemeIndex READ selectedThemeIndex NOTIFY selectedThemeIndexChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(ThemeFilter filter READ filter WRITE setFilter NOTIFY filterChanged)

    FilterProxyModel(QObject *parent = nullptr);
    ~FilterProxyModel() override;

    QString selectedTheme() const;
    void setSelectedTheme(const QString &pluginName);

    int selectedThemeIndex() const;

    QString query() const;
    void setQuery(const QString &query);

    ThemeFilter filter() const;
    void setFilter(ThemeFilter filter);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

Q_SIGNALS:
    void filterChanged();
    void queryChanged();

    void selectedThemeChanged();
    void selectedThemeIndexChanged();

private:
    QString m_selectedTheme;
    QString m_query;
    ThemeFilter m_filter = AllThemes;
};
