/*
    regionandlangsettings.cpp
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "regionandlangsettings.h"

using KCM_RegionAndLang::SettingType;

bool RegionAndLangSettings::isDefaultSetting(SettingType setting) const
{
    switch (setting) {
    case SettingType::Lang:
        return lang() == defaultLangValue();
    case SettingType::Language:
        return language() == defaultLanguageValue();
    case SettingType::Numeric:
        return numeric() == defaultNumericValue();
    case SettingType::Time:
        return time() == defaultTimeValue();
    case SettingType::Currency:
        return monetary() == defaultMonetaryValue();
    case SettingType::Measurement:
        return measurement() == defaultMeasurementValue();
    case SettingType::PaperSize:
        return paperSize() == defaultPaperSizeValue();
    case SettingType::Address:
        return address() == defaultAddressValue();
    case SettingType::NameStyle:
        return nameStyle() == defaultNameStyleValue();
    case SettingType::PhoneNumbers:
        return phoneNumbers() == defaultPhoneNumbersValue();
    case KCM_RegionAndLang::BinaryDialect:
        Q_UNREACHABLE();
        break;
    }

    return false;
}

QString RegionAndLangSettings::langWithFallback() const
{
    QString lang = RegionAndLangSettings::lang();
    if (!(isDefaultSetting(SettingType::Lang) && lang.isEmpty())) {
        if (QString envLang = qEnvironmentVariable("LANG"); !envLang.isEmpty()) {
            envLang.replace(QStringLiteral("utf8"), QStringLiteral("UTF-8"));
            return envLang;
        }
        return QLocale::system().name();
    }
    return lang;
}

QString RegionAndLangSettings::LC_LocaleWithLang(SettingType setting) const
{
    if (isDefaultSetting(setting)) {
        return langWithFallback();
    }

    switch (setting) {
    case SettingType::Numeric:
        return numeric();
    case SettingType::Time:
        return time();
    case SettingType::Currency:
        return monetary();
    case SettingType::Measurement:
        return measurement();
    case SettingType::PaperSize:
        return paperSize();
    case SettingType::Address:
        return address();
    case SettingType::NameStyle:
        return nameStyle();
    case SettingType::PhoneNumbers:
        return phoneNumbers();
    case SettingType::Lang:
        return lang();
    case SettingType::Language:
        return language();
    case SettingType::BinaryDialect:
        Q_UNREACHABLE();
        break;
    }

    return langWithFallback();
}

void RegionAndLangSettings::setLC_Vars(const QString &LANG)
{
    setNumeric(LANG);
    setTime(LANG);
    setMonetary(LANG);
    setMeasurement(LANG);
    setPaperSize(LANG);
    setAddress(LANG);
    setNameStyle(LANG);
    setPhoneNumbers(LANG);
}