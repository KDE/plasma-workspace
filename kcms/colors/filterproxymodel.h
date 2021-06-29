/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QSortFilterProxyModel>

#include "colors.h"

class FilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QString selectedScheme READ selectedScheme WRITE setSelectedScheme NOTIFY selectedSchemeChanged)
    Q_PROPERTY(int selectedSchemeIndex READ selectedSchemeIndex NOTIFY selectedSchemeIndexChanged)

    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(KCMColors::SchemeFilter filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    FilterProxyModel(QObject *parent = nullptr);
    ~FilterProxyModel() override;

    QString selectedScheme() const;
    void setSelectedScheme(const QString &scheme);

    int selectedSchemeIndex() const;

    QString query() const;
    void setQuery(const QString &query);

    KCMColors::SchemeFilter filter() const;
    void setFilter(KCMColors::SchemeFilter filter);

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

Q_SIGNALS:
    void queryChanged();
    void filterChanged();

    void selectedSchemeChanged();
    void selectedSchemeIndexChanged();

private:
    void emitSelectedSchemeIndexChange();

    QString m_selectedScheme;

    QString m_query;
    KCMColors::SchemeFilter m_filter = KCMColors::AllSchemes;
};
