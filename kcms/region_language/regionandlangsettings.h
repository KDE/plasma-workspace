/*
    regionandlangsettings.h
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "regionandlangsettingsbase.h"
#include "settingtype.h"

class RegionAndLangSettings : public RegionAndLangSettingsBase
{
    Q_OBJECT
public:
    using RegionAndLangSettingsBase::RegionAndLangSettingsBase;
    bool isDefaultSetting(KCM_RegionAndLang::SettingType setting) const;
    QString langWithFallback() const;
    QString LC_LocaleWithLang(KCM_RegionAndLang::SettingType setting) const;
};
