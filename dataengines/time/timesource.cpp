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
#include <QTimeZone>

#include <KLocalizedString>

#include "solarsystem.h"

//timezone is defined in msvc
#ifdef timezone
#undef timezone
#endif

TimeSource::TimeSource(const QString &name, QObject *parent)
    : Plasma::DataContainer(parent),
      m_offset(0),
      m_latitude(0),
      m_longitude(0),
      m_sun(0),
      m_moon(0),
      m_moonPosition(false),
      m_solarPosition(false),
      m_local(false)
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

    const QString trTimezone = i18n(m_tzName.toUtf8());
    setData(I18N_NOOP("Timezone"), trTimezone);

    const QStringList tzParts = trTimezone.split('/', QString::SkipEmptyParts);
    if (tzParts.count() == 1) {
        // no '/' so just set it as the city
        setData(I18N_NOOP("Timezone City"), trTimezone);
    } else {
        setData(I18N_NOOP("Timezone Continent"), tzParts.value(0));
        setData(I18N_NOOP("Timezone City"), tzParts.value(1));
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
    QTimeZone tz;
    if (m_local) {
        tz = QTimeZone(QTimeZone::systemTimeZoneId());
    } else {
        tz = QTimeZone(m_tzName.toUtf8());
        if (!tz.isValid()) {
            tz = QTimeZone(QTimeZone::systemTimeZoneId());
        }
    }

    int offset = tz.offsetFromUtc(QDateTime::currentDateTime());
    if (m_offset != offset) {
        m_offset = offset;
        setData(I18N_NOOP("Offset"), m_offset);
    }

    QDateTime dt;
    if (m_userDateTime) {
        dt = data()["DateTime"].toDateTime();
    } else {
        dt = QDateTime::currentDateTime().toTimeZone(tz);
    }

    if (m_solarPosition || m_moonPosition) {
        const QDate prev = data()["Date"].toDate();
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
        setData(I18N_NOOP("Time"), dt.time());
        setData(I18N_NOOP("Date"), dt.date());
        setData(I18N_NOOP("DateTime"), dt);
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
                    setData(I18N_NOOP("Date"), dt.date());
                    setData(I18N_NOOP("Time"), dt.time());
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

Sun* TimeSource::sun()
{
    if (!m_sun) {
        m_sun = new Sun();
    }
    m_sun->setPosition(m_latitude, m_longitude);
    return m_sun;
}

Moon* TimeSource::moon()
{
    if (!m_moon) {
        m_moon = new Moon(sun());
    }
    m_moon->setPosition(m_latitude, m_longitude);
    return m_moon;
}

void TimeSource::addMoonPositionData(const QDateTime &dt)
{
    Moon* m = moon();
    m->calcForDateTime(dt, m_offset);
    setData("Moon Azimuth", m->azimuth());
    setData("Moon Zenith", 90 - m->altitude());
    setData("Moon Corrected Elevation", m->calcElevation());
    setData("MoonPhaseAngle", m->phase());
}

void TimeSource::addDailyMoonPositionData(const QDateTime &dt)
{
    Moon* m = moon();
    QList< QPair<QDateTime, QDateTime> > times = m->timesForAngles(
            QList<double>() << -0.833, dt, m_offset);
    setData("Moonrise", times[0].first);
    setData("Moonset", times[0].second);
    m->calcForDateTime(QDateTime(dt.date(), QTime(12,0)), m_offset);
    setData("MoonPhase",  int(m->phase() / 360.0 * 29.0));
}

void TimeSource::addSolarPositionData(const QDateTime &dt)
{
    Sun* s = sun();
    s->calcForDateTime(dt, m_offset);
    setData("Azimuth", s->azimuth());
    setData("Zenith", 90.0 - s->altitude());
    setData("Corrected Elevation", s->calcElevation());
}

void TimeSource::addDailySolarPositionData(const QDateTime &dt)
{
    Sun* s = sun();
    QList< QPair<QDateTime, QDateTime> > times = s->timesForAngles(
            QList<double>() << -0.833 << -6.0 << -12.0 << -18.0, dt, m_offset);

    setData("Sunrise", times[0].first);
    setData("Sunset", times[0].second);
    setData("Civil Dawn", times[1].first);
    setData("Civil Dusk", times[1].second);
    setData("Nautical Dawn", times[2].first);
    setData("Nautical Dusk", times[2].second);
    setData("Astronomical Dawn", times[3].first);
    setData("Astronomical Dusk", times[3].second);
}

