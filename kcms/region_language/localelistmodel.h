/*
 *  localelistmodel.h
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
#pragma once

#include "settingtype.h"

#include <QAbstractListModel>
#include <QLocale>
#include <QSortFilterProxyModel>

struct LocaleData {
    QString nativeName;
    QString englishName;
    QString nativeCountryName;
    QString englishCountryName;
    QString countryCode;
    QLocale locale;
};

class LocaleListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int selectedConfig READ selectedConfig WRITE setSelectedConfig NOTIFY selectedConfigChanged)
public:
    enum RoleName { DisplayName = Qt::DisplayRole, LocaleName, FlagIcon, Example, FilterRole };
    explicit LocaleListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int selectedConfig() const;
    void setSelectedConfig(int config);
    Q_INVOKABLE void setLang(const QString &lang);

Q_SIGNALS:
    void selectedConfigChanged();

private:
    void getExample();
    std::vector<LocaleData> m_localeData;
    KCM_RegionAndLang::SettingType m_configType = KCM_RegionAndLang::Lang;
};
