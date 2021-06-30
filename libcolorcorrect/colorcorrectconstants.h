/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

namespace ColorCorrect
{
// these values needs to be hold in sync with the compositor
static const int MSC_DAY = 86400000;
static const int MIN_TEMPERATURE = 1000;
static const int NEUTRAL_TEMPERATURE = 6500;
static const int DEFAULT_NIGHT_TEMPERATURE = 4500;
static const int FALLBACK_SLOW_UPDATE_TIME = 30; // in minutes

}
