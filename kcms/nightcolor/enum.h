/*
SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <qobjectdefs.h>

namespace ColorCorrect
{
Q_NAMESPACE
enum NightColorMode {
    /**
     * Color temperature is computed based on the current position of the Sun.
     *
     * Location of the user is provided by Plasma.
     */
    Automatic,
    /**
     * Color temperature is computed based on the current position of the Sun.
     *
     * Location of the user is provided by themselves.
     */
    Location,
    /**
     * Color temperature is computed based on the current time.
     *
     * Sunrise and sunset times have to be specified by the user.
     */
    Timings,
    /**
     * Color temperature is constant thoughout the day.
     */
    Constant,
};

Q_ENUM_NS(NightColorMode)
}
