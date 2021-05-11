/*
 *  kcmformats.cpp
 *  Copyright 2014 Sebastian Kügler <sebas@kde.org>
 *  Copyright 2021 Han Young <hanyoung@protonmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */
#ifndef LOCALELISTMODEL_H
#define LOCALELISTMODEL_H

#include <QAbstractListModel>
#include <QLocale>
class LocaleListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
public:
    enum RoleName { DisplayName = Qt::DisplayRole, LocaleName, FlagIcon };
    LocaleListModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QString &filter() const;
    void setFilter(const QString &filter);

Q_SIGNALS:
    void filterChanged();

private:
    void filterLocale();

    QString m_filter;
    std::vector<std::tuple<QString, QString, QString, QLocale>> m_localeTuples; // lang, country, name
    std::vector<int> m_filteredLocales;
    bool m_noFilter = true;
};

#endif // LOCALELISTMODEL_H
