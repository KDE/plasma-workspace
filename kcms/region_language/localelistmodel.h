/*
 *  localelistmodel.h
 *  SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
 *  SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
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
