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
#include "exampleutility.h"
#include "kcmregionandlang.h"

#include <QTextCodec>

using namespace KCM_RegionAndLang;

LocaleListModel::LocaleListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    const QList<QLocale> m_locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    m_localeData.reserve(m_locales.size() + 1);
    // we use first QString for title in Unset option
    m_localeData.push_back(LocaleData{i18n("Default for System"), QString(), QString(), QString(), i18n("Default"), QLocale()});
    for (auto &locale : m_locales) {
        m_localeData.push_back(LocaleData{.nativeName = locale.nativeLanguageName(),
                                          .englishName = QLocale::languageToString(locale.language()),
                                          .nativeCountryName = locale.nativeCountryName(),
                                          .englishCountryName = QLocale::countryToString(locale.country()),
                                          .countryCode = locale.name(),
                                          .locale = locale});
    }
}

int LocaleListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_localeData.size();
}

QVariant LocaleListModel::data(const QModelIndex &index, int role) const
{
    int dataIndex = index.row();
    const auto &data = m_localeData.at(dataIndex);
    switch (role) {
    case FlagIcon: {
        QString countryCode;
        const QStringList split = data.countryCode.split(QLatin1Char('_'));
        if (split.size() > 1) {
            countryCode = split[1].toLower();
        }
        auto flagIconPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                   QStringLiteral("kf" QT_STRINGIFY(QT_VERSION_MAJOR) "/locale/countries/%1/flag.png").arg(countryCode));
        return flagIconPath;
    }
    case DisplayName: {
        // 0 is unset option, 1 is locale C
        if (dataIndex == 1) {
            return data.countryCode;
        }
        const QString clabel = !data.nativeCountryName.isEmpty() ? data.nativeCountryName : data.englishCountryName;
        QString languageName;
        if (!data.nativeName.isEmpty()) {
            languageName = data.nativeName;
        } else {
            languageName = data.englishName;
        }
        if (dataIndex == 0) {
            return languageName;
        }
        return i18nc("the first is language name, the second is the country name, like 'English (America)'", "%1 (%2)", languageName, clabel);
    }
    case LocaleName: {
        QString cvalue = data.countryCode;
        if (!cvalue.contains(QLatin1Char('.')) && cvalue != QLatin1Char('C') && cvalue != i18n("Default")) {
            // explicitly add the encoding,
            // otherwise Qt doesn't accept dead keys and garbles the output as well
            cvalue.append(QLatin1Char('.') + QTextCodec::codecForLocale()->name());
        }
        return cvalue;
    }
    case Example: {
        switch (m_configType) {
        case Lang:
            return {};
        case Numeric:
            return Utility::numericExample(data.locale);
        case Time:
            return Utility::timeExample(data.locale);
        case Currency:
            return Utility::monetaryExample(data.locale);
        case Measurement:
            return Utility::measurementExample(data.locale);
        case PaperSize:
            return Utility::paperSizeExample(data.locale);
#ifdef LC_ADDRESS
        case Address:
            return Utility::addressExample(data.locale);
        case NameStyle:
            return Utility::nameStyleExample(data.locale);
        case PhoneNumbers:
            return Utility::phoneNumbersExample(data.locale);
#endif
        }
        return {};
    }
    case FilterRole: {
        return data.englishCountryName.toLower() + data.nativeCountryName.toLower() + data.nativeName.toLower() + data.englishName.toLower()
            + data.countryCode.toLower();
    }
    }
    Q_UNREACHABLE();
    return {};
}

QHash<int, QByteArray> LocaleListModel::roleNames() const
{
    return {{LocaleName, QByteArrayLiteral("localeName")},
            {DisplayName, QByteArrayLiteral("display")},
            {FlagIcon, QByteArrayLiteral("flag")},
            {Example, QByteArrayLiteral("example")},
            {FilterRole, QByteArrayLiteral("filter")}};
}

int LocaleListModel::selectedConfig() const
{
    return m_configType;
}

void LocaleListModel::setSelectedConfig(int config)
{
    if (config != m_configType) {
        m_configType = static_cast<SettingType>(config);
    }
    Q_EMIT selectedConfigChanged();
    Q_EMIT dataChanged(createIndex(0, 0), createIndex(rowCount(), 0), QVector<int>(1, Example));
}

void LocaleListModel::setLang(const QString &lang)
{
    QString tmpLang = lang;
    bool isC = false;
    if (lang.isEmpty()) {
        tmpLang = qgetenv("LANG");
        if (tmpLang.isEmpty()) {
            tmpLang = QStringLiteral("C");
            isC = true;
        }
    }

    LocaleData &data = m_localeData.front();
    if (isC) {
        data.nativeName = i18nc("@info:title, meaning the current locale is system default(which is `C`)", "System Default C");
    } else {
        data.nativeName =
            i18nc("@info:title the current locale is the default for %1, %1 is the country name", "Default for %1", QLocale(tmpLang).nativeLanguageName());
    }
    data.locale = QLocale(tmpLang);

    Q_EMIT dataChanged(createIndex(0, 0), createIndex(0, 0));
}
