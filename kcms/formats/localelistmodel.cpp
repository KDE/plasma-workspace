/*
 *  localelistmodel.cpp
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
#include "exampleutility.cpp"
#include "kcmformats.h"
#include <KLocalizedString>
#include <QTextCodec>
LocaleListModel::LocaleListModel()
{
    QList<QLocale> m_locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    m_localeTuples.reserve(m_locales.size() + 1);
    m_localeTuples.push_back(std::tuple<QString, QString, QString, QLocale>(i18n("Default"), i18n("Default"), i18n("Default"), QLocale()));
    for (auto &locale : m_locales) {
        m_localeTuples.push_back(
            std::tuple<QString, QString, QString, QLocale>(locale.nativeLanguageName(), locale.nativeCountryName(), locale.name(), locale));
    }
}
int LocaleListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (m_noFilter) {
        return m_localeTuples.size();
    } else {
        return m_filteredLocales.size();
    }
}
QVariant LocaleListModel::data(const QModelIndex &index, int role) const
{
    int tupleIndex;
    if (m_noFilter) {
        tupleIndex = index.row();
    } else {
        tupleIndex = m_filteredLocales.at(index.row());
    }

    const auto &[lang, country, name, locale] = m_localeTuples.at(tupleIndex);
    switch (role) {
    case FlagIcon: {
        QString flagCode;
        const QStringList split = name.split(QLatin1Char('_'));
        if (split.count() > 1) {
            flagCode = split[1].toLower();
        }
        auto flagIconPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                   QStringLiteral("kf" QT_STRINGIFY(QT_VERSION_MAJOR) "/locale/countries/%1/flag.png").arg(flagCode));
        return flagIconPath;
    }
    case DisplayName: {
        const QString clabel = !country.isEmpty() ? country : QLocale::countryToString(locale.country());
        if (!lang.isEmpty()) {
            return lang + QStringLiteral(" (") + clabel + QStringLiteral(")");
        } else {
            return name + QStringLiteral(" (") + clabel + QStringLiteral(")");
        }
    }
    case LocaleName: {
        QString cvalue = name;
        if (!cvalue.contains(QLatin1Char('.')) && cvalue != QLatin1Char('C')) {
            // explicitly add the encoding,
            // otherwise Qt doesn't accept dead keys and garbles the output as well
            cvalue.append(QLatin1Char('.') + QTextCodec::codecForLocale()->name());
        }
        return cvalue;
    }
    case Example: {
        switch (m_configType) {
        case Lang:
            return QVariant();
        case Numeric:
            return Utility::numericExample(locale);
        case Time:
            return Utility::shortTimeExample(locale);
        case Currency:
            return Utility::monetaryExample(locale);
        case Measurement:
            return Utility::measurementExample(locale);
        case Collate:
            return Utility::collateExample(locale);
        default:
            return QVariant();
        }
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LocaleListModel::roleNames() const
{
    return {{LocaleName, "localeName"}, {DisplayName, "display"}, {FlagIcon, "flag"}, {Example, "example"}};
}

const QString &LocaleListModel::filter() const
{
    return m_filter;
}

void LocaleListModel::setFilter(const QString &filter)
{
    if (m_filter == filter) {
        return;
    }
    m_filter = filter;
    filterLocale();
}

void LocaleListModel::filterLocale()
{
    beginResetModel();
    if (!m_filter.isEmpty()) {
        m_filteredLocales.clear();
        int i{0};
        for (const auto &[lang, country, name, _locale] : m_localeTuples) {
            if (lang.indexOf(m_filter, 0, Qt::CaseInsensitive) != -1) {
                m_filteredLocales.push_back(i);
            } else if (country.indexOf(m_filter, 0, Qt::CaseInsensitive) != -1) {
                m_filteredLocales.push_back(i);
            } else if (name.indexOf(m_filter, 0, Qt::CaseInsensitive) != -1) {
                m_filteredLocales.push_back(i);
            }
            i++;
        }
        m_noFilter = false;
    } else {
        m_noFilter = true;
    }
    endResetModel();
}

QString LocaleListModel::selectedConfig() const
{
    switch (m_configType) {
    case Lang:
        return QStringLiteral("lang");
    case Numeric:
        return QStringLiteral("numeric");
    case Time:
        return QStringLiteral("time");
    case Currency:
        return QStringLiteral("currency");
    case Measurement:
        return QStringLiteral("measurement");
    case Collate:
        return QStringLiteral("collate");
    }
    // won't reach here
    return QString();
}

void LocaleListModel::setSelectedConfig(const QString &config)
{
    if (config == QStringLiteral("lang")) {
        m_configType = Lang;
    } else if (config == QStringLiteral("numeric")) {
        m_configType = Numeric;
    } else if (config == QStringLiteral("time")) {
        m_configType = Time;
    } else if (config == QStringLiteral("measurement")) {
        m_configType = Measurement;
    } else if (config == QStringLiteral("currency")) {
        m_configType = Currency;
    } else {
        m_configType = Collate;
    }
    Q_EMIT selectedConfigChanged();
    Q_EMIT dataChanged(createIndex(0, 0), createIndex(rowCount(), 0), QVector<int>(1, Example));
}
