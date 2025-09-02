/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "darklightschedulepreview.h"

#include <KDarkLightSchedule>
#include <KHolidays/SunEvents>

DarkLightSchedulePreview::DarkLightSchedulePreview(QObject *parent)
    : QObject(parent)
{
}

void DarkLightSchedulePreview::classBegin()
{
}

void DarkLightSchedulePreview::componentComplete()
{
    recalculate();
    m_complete = true;
}

QGeoCoordinate DarkLightSchedulePreview::coordinate() const
{
    return m_coordinate;
}

void DarkLightSchedulePreview::setCoordinate(const QGeoCoordinate &coordinate)
{
    if (m_coordinate != coordinate) {
        m_coordinate = coordinate;
        if (m_complete) {
            recalculate();
        }
        Q_EMIT coordinateChanged();
    }
}

void DarkLightSchedulePreview::resetCoordinate()
{
    if (m_coordinate.isValid()) {
        m_coordinate = QGeoCoordinate();
        if (m_complete) {
            recalculate();
        }
        Q_EMIT coordinateChanged();
    }
}

QTime DarkLightSchedulePreview::sunsetStart() const
{
    return m_sunsetStart;
}

void DarkLightSchedulePreview::setSunsetStart(const QTime &start)
{
    if (m_sunsetStart != start) {
        m_sunsetStart = start;
        if (m_complete) {
            recalculate();
        }
        Q_EMIT sunsetStartChanged();
    }
}

QTime DarkLightSchedulePreview::sunriseStart() const
{
    return m_sunriseStart;
}

void DarkLightSchedulePreview::setSunriseStart(const QTime &start)
{
    if (m_sunriseStart != start) {
        m_sunriseStart = start;
        if (m_complete) {
            recalculate();
        }
        Q_EMIT sunriseStartChanged();
    }
}

int DarkLightSchedulePreview::transitionDuration() const
{
    return m_transitionDuration.count();
}

void DarkLightSchedulePreview::setTransitionDuration(int duration)
{
    if (m_transitionDuration.count() != duration) {
        m_transitionDuration = std::chrono::milliseconds(duration);
        if (m_complete) {
            recalculate();
        }
        Q_EMIT transitionDurationChanged();
    }
}

QDateTime DarkLightSchedulePreview::startSunriseDateTime() const
{
    return m_startSunriseDateTime;
}

void DarkLightSchedulePreview::setStartSunriseDateTime(const QDateTime &dateTime)
{
    if (m_startSunriseDateTime != dateTime) {
        m_startSunriseDateTime = dateTime;
        Q_EMIT startSunriseDateTimeChanged();
    }
}

QDateTime DarkLightSchedulePreview::endSunriseDateTime() const
{
    return m_endSunriseDateTime;
}

void DarkLightSchedulePreview::setEndSunriseDateTime(const QDateTime &dateTime)
{
    if (m_endSunriseDateTime != dateTime) {
        m_endSunriseDateTime = dateTime;
        Q_EMIT endSunriseDateTimeChanged();
    }
}

QDateTime DarkLightSchedulePreview::startSunsetDateTime() const
{
    return m_startSunsetDateTime;
}

void DarkLightSchedulePreview::setStartSunsetDateTime(const QDateTime &dateTime)
{
    if (m_startSunsetDateTime != dateTime) {
        m_startSunsetDateTime = dateTime;
        Q_EMIT startSunsetDateTimeChanged();
    }
}

QDateTime DarkLightSchedulePreview::endSunsetDateTime() const
{
    return m_endSunsetDateTime;
}

void DarkLightSchedulePreview::setEndSunsetDateTime(const QDateTime &dateTime)
{
    if (m_endSunsetDateTime != dateTime) {
        m_endSunsetDateTime = dateTime;
        Q_EMIT endSunsetDateTimeChanged();
    }
}

DarkLightSchedulePreview::FallbackReason DarkLightSchedulePreview::fallbackReason() const
{
    return m_fallbackReason;
}

void DarkLightSchedulePreview::setFallbackReason(FallbackReason reason)
{
    if (m_fallbackReason != reason) {
        m_fallbackReason = reason;
        Q_EMIT fallbackReasonChanged();
    }
}

void DarkLightSchedulePreview::recalculate()
{
    const QDateTime now = QDateTime::currentDateTime();

    FallbackReason fallbackReason = FallbackReason::None;
    std::optional<KDarkLightSchedule> schedule;
    if (m_coordinate.isValid()) {
        schedule = KDarkLightSchedule::forecast(now, m_coordinate.latitude(), m_coordinate.longitude());
        if (!schedule) {
            const KHolidays::SunEvents sunEvents(now, m_coordinate.latitude(), m_coordinate.longitude());
            if (sunEvents.isPolarDay()) {
                fallbackReason = FallbackReason::PolarDay;
            } else if (sunEvents.isPolarNight()) {
                fallbackReason = FallbackReason::PolarNight;
            } else {
                fallbackReason = FallbackReason::Other;
            }
        }
    }
    if (!schedule) {
        schedule = KDarkLightSchedule::forecast(now, m_sunriseStart, m_sunsetStart, m_transitionDuration);
    }

    apply(*schedule->previousTransition(now));
    apply(*schedule->nextTransition(now));
    setFallbackReason(fallbackReason);
}

void DarkLightSchedulePreview::apply(const KDarkLightTransition &transition)
{
    switch (transition.type()) {
    case KDarkLightTransition::Morning:
        setStartSunriseDateTime(transition.startDateTime());
        setEndSunriseDateTime(transition.endDateTime());
        break;
    case KDarkLightTransition::Evening:
        setStartSunsetDateTime(transition.startDateTime());
        setEndSunsetDateTime(transition.endDateTime());
        break;
    }
}
