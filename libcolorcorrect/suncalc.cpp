/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "suncalc.h"
#include "colorcorrectconstants.h"

#include <cmath>
#include <numbers>

#include <QDateTime>
#include <QTimeZone>

using namespace Qt::StringLiterals;

namespace ColorCorrect
{
inline constexpr double TWILIGHT_NAUT = -12.0;
inline constexpr double TWILIGHT_CIVIL = -6.0;
inline constexpr double SUN_RISE_SET = -0.833;
inline constexpr double SUN_HIGH = 2.0;
inline constexpr int DEFAULT_TRANSITION_DURATION = 1800000; /* 30 minutes */

static QTime convertToLocalTime(const QDateTime &when, const QTime &utcTime)
{
    const QTimeZone timeZone = QTimeZone::systemTimeZone();
    const int utcOffset = timeZone.offsetFromUtc(when);
    return utcTime.addSecs(utcOffset);
}

typedef QPair<QDateTime, QDateTime> DateTimes;

DateTimes calculateSunTimings(const QDateTime &dateTime, double latitude, double longitude, bool morning)
{
    // calculations based on https://aa.quae.nl/en/reken/zonpositie.html
    // accuracy: +/- 5min

    // positioning
    constexpr double rad = std::numbers::pi / 180.;
    constexpr double earthObliquity = 23.4397; // epsilon

    const double lat = latitude; // phi
    const double lng = -longitude; // lw

    // times
    const QDateTime utcDateTime = dateTime.toUTC();
    const double juPrompt = utcDateTime.date().toJulianDay(); // J
    constexpr double ju2000 = 2451545.; // J2000

    // geometry
    auto mod360 = [](double number) -> double {
        return std::fmod(number, 360.);
    };

    auto sin = [](double angle) -> double {
        return std::sin(angle * rad);
    };
    auto cos = [](double angle) -> double {
        return std::cos(angle * rad);
    };
    auto asin = [](double val) -> double {
        return std::asin(val) / rad;
    };
    auto acos = [](double val) -> double {
        return std::acos(val) / rad;
    };

    auto anomaly = [&mod360](const double date) -> double { // M
        return mod360(357.5291 + 0.98560028 * (date - ju2000));
    };

    auto center = [&sin](double anomaly) -> double { // C
        return 1.9148 * sin(anomaly) + 0.02 * sin(2 * anomaly) + 0.0003 * sin(3 * anomaly);
    };

    auto ecliptLngMean = [](double anom) -> double { // Mean ecliptical longitude L_sun = Mean Anomaly + Perihelion + 180°
        return anom + 282.9372; // anom + 102.9372 + 180°
    };

    auto ecliptLng = [&](double anom) -> double { // lambda = L_sun + C
        return ecliptLngMean(anom) + center(anom);
    };

    auto declination = [&](const double date) -> double { // delta
        const double anom = anomaly(date);
        const double eclLng = ecliptLng(anom);

        return mod360(asin(sin(earthObliquity) * sin(eclLng)));
    };

    // sun hour angle at specific angle
    auto hourAngle = [&](const double date, double angle) -> double { // H_t
        const double decl = declination(date);
        const double ret0 = (sin(angle) - sin(lat) * sin(decl)) / (cos(lat) * cos(decl));

        double ret = mod360(acos(ret0));
        if (180. < ret) {
            ret = ret - 360.;
        }
        return ret;
    };

    /*
     * Sun positions
     */

    // transit is at noon
    auto getTransit = [&](const double date) -> double { // Jtransit
        const double juMeanSolTime = juPrompt - ju2000 - 0.0009 - lng / 360.; // n_x = J - J_2000 - J_0 - l_w / 360°
        const double juTrEstimate = date + qRound64(juMeanSolTime) - juMeanSolTime; // J_x = J + n - n_x
        const double anom = anomaly(juTrEstimate); // M
        const double eclLngM = ecliptLngMean(anom); // L_sun

        return juTrEstimate + 0.0053 * sin(anom) - 0.0068 * sin(2 * eclLngM);
    };

    auto getSunMorning = [&hourAngle](const double angle, const double transit) -> double {
        return transit - hourAngle(transit, angle) / 360.;
    };

    auto getSunEvening = [&hourAngle](const double angle, const double transit) -> double {
        return transit + hourAngle(transit, angle) / 360.;
    };

    /*
     * Begin calculations
     */

    // noon - sun at the highest point
    const double juNoon = getTransit(juPrompt);

    double begin, end;
    if (morning) {
        begin = getSunMorning(TWILIGHT_CIVIL, juNoon);
        end = getSunMorning(SUN_HIGH, juNoon);
    } else {
        begin = getSunEvening(SUN_HIGH, juNoon);
        end = getSunEvening(TWILIGHT_CIVIL, juNoon);
    }
    // transform to QDateTime
    begin += 0.5;
    end += 0.5;

    QDateTime dateTimeBegin;
    QDateTime dateTimeEnd;

    if (!std::isnan(begin)) {
        const double dayFraction = begin - int(begin);
        const QTime utcTime = QTime::fromMSecsSinceStartOfDay(dayFraction * MSC_DAY);
        const QTime localTime = convertToLocalTime(dateTime, utcTime);
        dateTimeBegin = QDateTime(dateTime.date(), localTime);
    }

    if (!std::isnan(end)) {
        const double dayFraction = end - int(end);
        const QTime utcTime = QTime::fromMSecsSinceStartOfDay(dayFraction * MSC_DAY);
        const QTime localTime = convertToLocalTime(dateTime, utcTime);
        dateTimeEnd = QDateTime(dateTime.date(), localTime);
    }

    return {dateTimeBegin, dateTimeEnd};
}

QVariantMap getSunTimings(double latitude, double longitude, bool morning)
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    DateTimes dateTimes = calculateSunTimings(currentDateTime, latitude, longitude, morning);
    // At locations near the poles it is possible, that we can't
    // calculate some or all sun timings (midnight sun).
    // In this case try to fallback to sensible default values.
    const bool beginDefined = !dateTimes.first.isNull();
    const bool endDefined = !dateTimes.second.isNull();
    if (!beginDefined || !endDefined) {
        if (beginDefined) {
            dateTimes.second = dateTimes.first.addMSecs(DEFAULT_TRANSITION_DURATION);
        } else if (endDefined) {
            dateTimes.first = dateTimes.second.addMSecs(-DEFAULT_TRANSITION_DURATION);
        } else {
            // Just use default values for morning and evening, but the user
            // will probably deactivate Night Light anyway if he is living
            // in a region without clear sun rise and set.
            const QTime referenceTime = morning ? QTime(6, 0) : QTime(18, 0);
            dateTimes.first = QDateTime(currentDateTime.date(), referenceTime);
            dateTimes.second = dateTimes.first.addMSecs(DEFAULT_TRANSITION_DURATION);
        }
    }

    QVariantMap map;
    map.insert(u"begin"_s, dateTimes.first);
    map.insert(u"end"_s, dateTimes.second);
    return map;
}

QVariantMap SunCalc::getMorningTimings(double latitude, double longitude)
{
    return getSunTimings(latitude, longitude, true);
}

QVariantMap SunCalc::getEveningTimings(double latitude, double longitude)
{
    return getSunTimings(latitude, longitude, false);
}

}

#include "moc_suncalc.cpp"
