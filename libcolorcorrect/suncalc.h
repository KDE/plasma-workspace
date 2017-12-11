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
#ifndef COLORCORRECT_SUNCALC_H
#define COLORCORRECT_SUNCALC_H

#include <QObject>
#include <QDate>
#include <QTime>
#include <QPair>
#include <QVariant>

#include "colorcorrect_export.h"

namespace ColorCorrect
{

/*
 * The purpose of this class is solely to enable workspace to present the
 * resulting timings of the current or requested configuration to the user.
 * The final calculations happen in similar functions in the compositor.
 *
 * The functions provided by this class here therefore needs to be hold in
 * sync with the functions in the compositor.
 */

class COLORCORRECT_EXPORT SunCalc : public QObject
{
    Q_OBJECT

public:
    /**
     * Calculates for a given location and date two of the
     * following sun timings in their temporal order:
     * - Nautical dawn and sunrise for the morning
     * - Sunset and nautical dusk for the evening
     * @since 5.12
     **/
    Q_INVOKABLE QVariantMap getMorningTimings(double latitude, double longitude);
    Q_INVOKABLE QVariantMap getEveningTimings(double latitude, double longitude);

};

}

#endif // COLORCORRECT_SUNCALC_H
