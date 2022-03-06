/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    Moon Phase:
    SPDX-FileCopyrightText: 1998, 2000 Stephan Kulow <coolo@kde.org>
    SPDX-FileCopyrightText: 2009 Davide Bettio <davide.bettio@kdemail.net>

    Solar position:
    SPDX-FileCopyrightText: 2009 Petri Damsten <damu@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "timesource.h"

#include <QDateTime>

#include <KLazyLocalizedString>
#include <KLocalizedString>

#include "solarsystem.h"

// timezone is defined in msvc
#ifdef timezone
#undef timezone
#endif

TimeSource::TimeSource(const QString &name, QObject *parent)
    : Plasma::DataContainer(parent)
    , m_offset(0)
    , m_latitude(0)
    , m_longitude(0)
    , m_sun(nullptr)
    , m_moon(nullptr)
    , m_moonPosition(false)
    , m_solarPosition(false)
    , m_local(false)
{
    setObjectName(name);
    setTimeZone(parseName(name));
}

void TimeSource::setTimeZone(const QString &tz)
{
    m_tzName = tz;
    m_local = m_tzName == kli18n("Local").untranslatedText();
    if (m_local) {
        m_tzName = QString::fromUtf8(QTimeZone::systemTimeZoneId());
    }

    if (m_local) {
        m_tz = QTimeZone(QTimeZone::systemTimeZoneId());
    } else {
        m_tz = QTimeZone(m_tzName.toUtf8());
        if (!m_tz.isValid()) {
            m_tz = QTimeZone(QTimeZone::systemTimeZoneId());
        }
    }

    const QString trTimezone = i18n(m_tzName.toUtf8());
    setData(kli18n("Timezone").untranslatedText(), trTimezone);

    const QStringList tzParts = trTimezone.split('/', Qt::SkipEmptyParts);
    if (tzParts.count() == 1) {
        // no '/' so just set it as the city
        setData(kli18n("Timezone City").untranslatedText(), trTimezone);
    } else if (tzParts.count() == 2) {
        setData(kli18n("Timezone Continent").untranslatedText(), tzParts.value(0));
        setData(kli18n("Timezone City").untranslatedText(), tzParts.value(1));
    } else { // for zones like America/Argentina/Buenos_Aires
        setData(kli18n("Timezone Continent").untranslatedText(), tzParts.value(0));
        setData(kli18n("Timezone Country").untranslatedText(), tzParts.value(1));
        setData(kli18n("Timezone City").untranslatedText(), tzParts.value(2));
    }

    updateTime();
}

TimeSource::~TimeSource()
{
    // First delete the moon, that does not delete the Sun, and then the Sun
    // If the Sun is deleted before the moon, the moon has a invalid pointer
    // to where the Sun was pointing.
    delete m_moon;
    delete m_sun;
}

void TimeSource::updateTime()
{
    QDateTime timeZoneDateTime = QDateTime::currentDateTime().toTimeZone(m_tz);

    int offset = m_tz.offsetFromUtc(timeZoneDateTime);
    if (m_offset != offset) {
        m_offset = offset;
    }

    setData(kli18n("Offset").untranslatedText(), m_offset);

    QString abbreviation = m_tz.abbreviation(timeZoneDateTime);
    setData(kli18n("Timezone Abbreviation").untranslatedText(), abbreviation);

    QDateTime dt;
    if (m_userDateTime) {
        dt = data()[QStringLiteral("DateTime")].toDateTime();
    } else {
        dt = timeZoneDateTime;
    }

    if (m_solarPosition || m_moonPosition) {
        const QDate prev = data()[QStringLiteral("DateTime")].toDate();
        const bool updateDailies = prev != dt.date();

        if (m_solarPosition) {
            if (updateDailies) {
                addDailySolarPositionData(dt);
            }

            addSolarPositionData(dt);
        }

        if (m_moonPosition) {
            if (updateDailies) {
                addDailyMoonPositionData(dt);
            }

            addMoonPositionData(dt);
        }
    }

    if (!m_userDateTime) {
        setData(kli18n("DateTime").untranslatedText(), dt);

        forceImmediateUpdate();
    }
}

QString TimeSource::parseName(const QString &name)
{
    m_userDateTime = false;
    if (!name.contains('|')) {
        // the simple case where it's just a timezone request
        return name;
    }

    // the various keys we recognize
    constexpr const auto latitude = kli18n("Latitude");
    constexpr const auto longitude = kli18n("Longitude");
    constexpr const auto solar = kli18n("Solar");
    constexpr const auto moon = kli18n("Moon");
    constexpr const auto datetime = kli18n("DateTime");

    // now parse out what we got handed in
    const QStringList list = name.split('|', Qt::SkipEmptyParts);

    const int listSize = list.size();
    for (int i = 1; i < listSize; ++i) {
        const QString arg = list[i];
        const int n = arg.indexOf('=');

        if (n != -1) {
            const QString key = arg.mid(0, n);
            const QString value = arg.mid(n + 1);

            if (key == latitude.untranslatedText()) {
                m_latitude = value.toDouble();
            } else if (key == longitude.untranslatedText()) {
                m_longitude = value.toDouble();
            } else if (key == datetime.untranslatedText()) {
                QDateTime dt = QDateTime::fromString(value, Qt::ISODate);
                if (dt.isValid()) {
                    setData(kli18n("DateTime").untranslatedText(), dt);
                    m_userDateTime = true;
                }
            }
        } else if (arg == solar.untranslatedText()) {
            m_solarPosition = true;
        } else if (arg == moon.untranslatedText()) {
            m_moonPosition = true;
        }
    }

    // timezone is first item ...
    return list.at(0);
}

Sun *TimeSource::sun()
{
    if (!m_sun) {
        m_sun = new Sun();
    }
    m_sun->setPosition(m_latitude, m_longitude);
    return m_sun;
}

Moon *TimeSource::moon()
{
    if (!m_moon) {
        m_moon = new Moon(sun());
    }
    m_moon->setPosition(m_latitude, m_longitude);
    return m_moon;
}

void TimeSource::addMoonPositionData(const QDateTime &dt)
{
    Moon *m = moon();
    m->calcForDateTime(dt, m_offset);
    setData(QStringLiteral("Moon Azimuth"), m->azimuth());
    setData(QStringLiteral("Moon Zenith"), 90 - m->altitude());
    setData(QStringLiteral("Moon Corrected Elevation"), m->calcElevation());
    setData(QStringLiteral("MoonPhaseAngle"), m->phase());
}

void TimeSource::addDailyMoonPositionData(const QDateTime &dt)
{
    Moon *m = moon();
    QList<QPair<QDateTime, QDateTime>> times = m->timesForAngles(QList<double>() << -0.833, dt, m_offset);
    setData(QStringLiteral("Moonrise"), times[0].first);
    setData(QStringLiteral("Moonset"), times[0].second);
    m->calcForDateTime(QDateTime(dt.date(), QTime(12, 0)), m_offset);
    setData(QStringLiteral("MoonPhase"), int(m->phase() / 360.0 * 29.0));
}

void TimeSource::addSolarPositionData(const QDateTime &dt)
{
    Sun *s = sun();
    s->calcForDateTime(dt, m_offset);
    setData(QStringLiteral("Azimuth"), s->azimuth());
    setData(QStringLiteral("Zenith"), 90.0 - s->altitude());
    setData(QStringLiteral("Corrected Elevation"), s->calcElevation());
}

void TimeSource::addDailySolarPositionData(const QDateTime &dt)
{
    Sun *s = sun();
    QList<QPair<QDateTime, QDateTime>> times = s->timesForAngles(QList<double>() << -0.833 << -6.0 << -12.0 << -18.0, dt, m_offset);

    setData(QStringLiteral("Sunrise"), times[0].first);
    setData(QStringLiteral("Sunset"), times[0].second);
    setData(QStringLiteral("Civil Dawn"), times[1].first);
    setData(QStringLiteral("Civil Dusk"), times[1].second);
    setData(QStringLiteral("Nautical Dawn"), times[2].first);
    setData(QStringLiteral("Nautical Dusk"), times[2].second);
    setData(QStringLiteral("Astronomical Dawn"), times[3].first);
    setData(QStringLiteral("Astronomical Dusk"), times[3].second);
}
