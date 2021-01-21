/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   Moon Phase:
 *   Copyright 1998,2000  Stephan Kulow <coolo@kde.org>
 *   Copyright 2009 by Davide Bettio <davide.bettio@kdemail.net>
 *
 *   Solar position:
 *   Copyright (C) 2009 Petri Damsten <damu@iki.fi>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "timesource.h"

#include <QDateTime>

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
    m_local = m_tzName == I18N_NOOP("Local");
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
    setData(I18N_NOOP("Timezone"), trTimezone);

    const QStringList tzParts = trTimezone.split('/', QString::SkipEmptyParts);
    if (tzParts.count() == 1) {
        // no '/' so just set it as the city
        setData(I18N_NOOP("Timezone City"), trTimezone);
    } else if (tzParts.count() == 2) {
        setData(I18N_NOOP("Timezone Continent"), tzParts.value(0));
        setData(I18N_NOOP("Timezone City"), tzParts.value(1));
    } else { // for zones like America/Argentina/Buenos_Aires
        setData(I18N_NOOP("Timezone Continent"), tzParts.value(0));
        setData(I18N_NOOP("Timezone Country"), tzParts.value(1));
        setData(I18N_NOOP("Timezone City"), tzParts.value(2));
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

    setData(I18N_NOOP("Offset"), m_offset);

    QString abbreviation = m_tz.abbreviation(timeZoneDateTime);
    setData(I18N_NOOP("Timezone Abbreviation"), abbreviation);

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
        setData(I18N_NOOP("DateTime"), dt);

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
    static const QString latitude = I18N_NOOP("Latitude");
    static const QString longitude = I18N_NOOP("Longitude");
    static const QString solar = I18N_NOOP("Solar");
    static const QString moon = I18N_NOOP("Moon");
    static const QString datetime = I18N_NOOP("DateTime");

    // now parse out what we got handed in
    const QStringList list = name.split('|', QString::SkipEmptyParts);

    const int listSize = list.size();
    for (int i = 1; i < listSize; ++i) {
        const QString arg = list[i];
        const int n = arg.indexOf('=');

        if (n != -1) {
            const QString key = arg.mid(0, n);
            const QString value = arg.mid(n + 1);

            if (key == latitude) {
                m_latitude = value.toDouble();
            } else if (key == longitude) {
                m_longitude = value.toDouble();
            } else if (key == datetime) {
                QDateTime dt = QDateTime::fromString(value, Qt::ISODate);
                if (dt.isValid()) {
                    setData(I18N_NOOP("DateTime"), dt);
                    m_userDateTime = true;
                }
            }
        } else if (arg == solar) {
            m_solarPosition = true;
        } else if (arg == moon) {
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
