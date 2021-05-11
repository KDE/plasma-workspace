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
#include "localelistmodel.h"
#include "kcmformats.h"
#include <KLocalizedString>
#include <QTextCodec>
LocaleListModel::LocaleListModel()
{
    auto m_locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    m_localeTuples.reserve(m_locales.size());
    for (auto &locale : m_locales) {
        m_localeTuples.push_back(
            std::tuple<QString, QString, QString, QLocale>(locale.nativeLanguageName(), locale.nativeCountryName(), locale.name(), locale));
    }
}
int LocaleListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (m_noFilter)
        return m_localeTuples.size();
    else
        return m_filteredLocales.size();
}
QVariant LocaleListModel::data(const QModelIndex &index, int role) const
{
    int tupleIndex;
    if (m_noFilter)
        tupleIndex = index.row();
    else
        tupleIndex = m_filteredLocales.at(index.row());

    const auto &tuple = m_localeTuples.at(tupleIndex);
    switch (role) {
    case FlagIcon: {
        QString flagCode;
        const QStringList split = std::get<2>(tuple).split(QLatin1Char('_'));
        if (split.count() > 1) {
            flagCode = split[1].toLower();
        }
        auto flagIconPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kf5/locale/countries/%1/flag.png").arg(flagCode));
        return flagIconPath.isEmpty() ? QStringLiteral("unknown") : flagIconPath;
    }
    case DisplayName: {
        const QString clabel = !std::get<1>(tuple).isEmpty() ? std::get<1>(tuple) : QLocale::countryToString(std::get<3>(tuple).country());
        if (!std::get<0>(tuple).isEmpty()) {
            return std::get<0>(tuple) + QStringLiteral(" (") + clabel + QStringLiteral(")");
        } else {
            return std::get<2>(tuple) + QStringLiteral(" (") + clabel + QStringLiteral(")");
        }
    }
    case LocaleName: {
        return std::get<2>(tuple);
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> LocaleListModel::roleNames() const
{
    return {{LocaleName, "localeName"}, {DisplayName, "display"}, {FlagIcon, "flag"}};
}

const QString &LocaleListModel::filter() const
{
    return m_filter;
}

void LocaleListModel::setFilter(const QString &filter)
{
    if (m_filter == filter)
        return;
    m_filter = filter;
    filterLocale();
}

void LocaleListModel::filterLocale()
{
    beginResetModel();
    if (!m_filter.isEmpty()) {
        m_filteredLocales.clear();
        auto i{0};
        for (const auto &tuple : m_localeTuples) {
            if (std::get<0>(tuple).indexOf(m_filter, 0, Qt::CaseInsensitive) != -1) {
                m_filteredLocales.push_back(i);
            } else if (std::get<1>(tuple).indexOf(m_filter, 0, Qt::CaseInsensitive) != -1) {
                m_filteredLocales.push_back(i);
            } else if (std::get<2>(tuple).indexOf(m_filter, 0, Qt::CaseInsensitive) != -1) {
                m_filteredLocales.push_back(i);
            }
            i++;
        }
        m_noFilter = false;
    } else
        m_noFilter = true;
    endResetModel();
}
