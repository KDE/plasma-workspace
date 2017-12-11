/********************************************************************
Copyright 2017 Roman Gilg <subdiff@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#ifndef COLORCORRECT_CONSTANTS_H
#define COLORCORRECT_CONSTANTS_H

namespace ColorCorrect
{

// these values needs to be hold in sync with the compositor
static const int MSC_DAY = 86400000;
static const int MIN_TEMPERATURE = 1000;
static const int NEUTRAL_TEMPERATURE = 6500;
static const int DEFAULT_NIGHT_TEMPERATURE = 4500;
static const int FALLBACK_SLOW_UPDATE_TIME = 30;    // in minutes

}

#endif // COLORCORRECT_CONSTANTS_H
