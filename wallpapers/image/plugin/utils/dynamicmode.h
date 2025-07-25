/*
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

namespace DynamicMode
{
Q_NAMESPACE

enum class Mode : unsigned int {
    Automatic = 0,
    DayNight,
    AlwaysDark,
    AlwaysLight,
};
Q_ENUM_NS(Mode)
}
