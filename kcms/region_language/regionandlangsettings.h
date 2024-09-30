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

    /* If the LC_* is not set with Plasma (~/.config/plasma-localerc),
     * but inherited from system-wide setting. Explicitly override LC_*
     * system-wide setting when user is setting $LANG.
     * For example:
     * # /etc/locale.conf
     * LANG=pt_PT.UTF-8
     * LC_TIME=pt_PT.UTF-8
     *
     * # ~/.config/plasma-localerc
     * [Formats]
     * LANG=en_US.UTF-8
     *
     * The end result is LANG=en_US.UTF-8, LC_TIME=pt_PT.UTF-8
     * But we will display LC_TIME as "en_US.UTF-8 inherit from LANG",
     * because it is NOT SET BY US.
     *
     * Explicit set LC_TIME in ~/.config/plasma-localerc in this case
     * should fix the issue.
     *
     * see https://bugs.kde.org/show_bug.cgi?id=491305
     */
    void setLC_Vars(const QString &LANG);
};
