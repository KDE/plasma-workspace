/*
    settingtype.h
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <qobjectdefs.h>

namespace KCM_RegionAndLang
{
Q_NAMESPACE_EXPORT()
enum SettingType { Lang, Numeric, Time, Currency, Measurement, PaperSize, Address, NameStyle, PhoneNumbers };
Q_ENUM_NS(SettingType)
} // namespace KCM_RegionAndLang
