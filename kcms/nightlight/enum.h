/*
SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <qobjectdefs.h>

namespace ColorCorrect
{
Q_NAMESPACE
enum NightLightMode {
    /**
     * Color temperature is constant thoughout the day.
     */
    Constant,
    /**
     * Colore temperature is synchronized to the dark-light cycle.
     */
    DarkLight,
};

Q_ENUM_NS(NightLightMode)
}
