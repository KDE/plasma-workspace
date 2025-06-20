/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "daynighttimings.h"

DayNightTimings::DayNightTimings(QObject *parent)
    : QObject(parent)
    , m_scheduleProvider(new KDarkLightScheduleProvider(QString(), this))
{
    connect(m_scheduleProvider, &KDarkLightScheduleProvider::scheduleChanged, this, &DayNightTimings::refresh);
}

QDateTime DayNightTimings::dateTime() const
{
    return m_dateTime;
}

void DayNightTimings::setDateTime(const QDateTime &dateTime)
{
    if (m_dateTime != dateTime) {
        m_dateTime = dateTime;
        refresh();
        Q_EMIT dateTimeChanged();
    }
}

QDateTime DayNightTimings::morningStart() const
{
    return m_morningStart;
}

void DayNightTimings::setMorningStart(const QDateTime &dateTime)
{
    if (m_morningStart != dateTime) {
        m_morningStart = dateTime;
        Q_EMIT morningStartChanged();
    }
}

QDateTime DayNightTimings::morningEnd() const
{
    return m_morningEnd;
}

void DayNightTimings::setMorningEnd(const QDateTime &dateTime)
{
    if (m_morningEnd != dateTime) {
        m_morningEnd = dateTime;
        Q_EMIT morningEndChanged();
    }
}

QDateTime DayNightTimings::eveningStart() const
{
    return m_eveningStart;
}

void DayNightTimings::setEveningStart(const QDateTime &dateTime)
{
    if (m_eveningStart != dateTime) {
        m_eveningStart = dateTime;
        Q_EMIT eveningStartChanged();
    }
}

QDateTime DayNightTimings::eveningEnd() const
{
    return m_eveningEnd;
}

void DayNightTimings::setEveningEnd(const QDateTime &dateTime)
{
    if (m_eveningEnd != dateTime) {
        m_eveningEnd = dateTime;
        Q_EMIT eveningEndChanged();
    }
}

void DayNightTimings::setTransition(const KDarkLightTransition &transition)
{
    if (transition.type() == KDarkLightTransition::Morning) {
        setMorningStart(transition.startDateTime());
        setMorningEnd(transition.endDateTime());
    } else {
        setEveningStart(transition.startDateTime());
        setEveningEnd(transition.endDateTime());
    }
}

void DayNightTimings::refresh()
{
    const KDarkLightSchedule schedule = m_scheduleProvider->schedule();

    setTransition(*schedule.nextTransition(m_dateTime));
    setTransition(*schedule.previousTransition(m_dateTime));
}

#include "moc_daynighttimings.cpp"
