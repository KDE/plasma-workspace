/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QDate>
#include <QObject>
#include <QPair>
#include <QTime>
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
