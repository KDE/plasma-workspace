/*
    regionandlangsettings.cpp
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "regionandlangsettings.h"

#include "kcm_regionandlang_debug.h"

using KCM_RegionAndLang::SettingType;

bool RegionAndLangSettings::isDefaultSetting(SettingType setting) const
{
    switch (setting) {
    case SettingType::Lang:
        return lang() == defaultLangValue();
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
    }

    return false;
}

QString RegionAndLangSettings::langWithFallback() const
{
    const QString lang = RegionAndLangSettings::lang();
    if (!(isDefaultSetting(SettingType::Lang) && lang.isEmpty())) {
        if (const QString envLang = qEnvironmentVariable("LANG"); !envLang.isEmpty()) {
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
        Q_UNREACHABLE();
    }

    return langWithFallback();
}
