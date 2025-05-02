/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "suncalc.h"
#include "suntransit.h"

using namespace Qt::StringLiterals;

namespace ColorCorrect
{

inline constexpr int DEFAULT_TRANSITION_DURATION = 1800000; /* 30 minutes */

static QVariantMap makeTransitionMap(const QDateTime &start, const QDateTime &end)
{
    QVariantMap map;
    map.insert(u"begin"_s, start);
    map.insert(u"end"_s, end);
    return map;
}

QVariantMap SunCalc::getMorningTimings(double latitude, double longitude)
{
    const QDateTime now = QDateTime::currentDateTime();
    const SunTransit transit(now, latitude, longitude);

    QDateTime civilDawn = transit.dateTime(SunTransit::CivilDawn);
    QDateTime sunrise = transit.dateTime(SunTransit::Sunrise);
    if (civilDawn.isNull() || sunrise.isNull()) {
        sunrise = QDateTime(now.date(), QTime(6, 0));
        civilDawn = sunrise.addMSecs(-DEFAULT_TRANSITION_DURATION);
    }

    return makeTransitionMap(civilDawn, sunrise);
}

QVariantMap SunCalc::getEveningTimings(double latitude, double longitude)
{
    const QDateTime now = QDateTime::currentDateTime();
    const SunTransit transit(now, latitude, longitude);

    QDateTime sunset = transit.dateTime(SunTransit::Sunset);
    QDateTime civilDusk = transit.dateTime(SunTransit::CivilDusk);
    if (sunset.isNull() || civilDusk.isNull()) {
        sunset = QDateTime(now.date(), QTime(18, 0));
        civilDusk = sunset.addMSecs(DEFAULT_TRANSITION_DURATION);
    }

    return makeTransitionMap(sunset, civilDusk);
}

}

#include "moc_suncalc.cpp"
