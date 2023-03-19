/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "clock.h"

#include <QDBusConnection>
#include <QDebug>

#include "alignedtimer.h"

Clock::Clock(QObject *parent)
    : QObject{parent}
{
    auto sessionBus = QDBusConnection::sessionBus();
    sessionBus
        .connect(QString(), QString(), QStringLiteral("org.kde.KTimeZoned"), QStringLiteral("timeZoneChanged"), this, SLOT(handleSystemTimeZoneChanged()));
    resetTimeZone();
}

const QByteArray Clock::timeZone() const
{
    return m_timeZone.id();
}

void Clock::setTimeZone(const QByteArray &ianaId)
{
    if (m_timeZone.id() == ianaId)
        return;

    const QTimeZone timeZone = QTimeZone(ianaId);
    if (!timeZone.isValid()) {
        resetTimeZone();
        return;
    }
    m_timeZoneExplicitlySet = true;
    setupTimeZone(timeZone);
    update();
}

void Clock::resetTimeZone()
{
    m_timeZoneExplicitlySet = false;
    setupTimeZone(QTimeZone(QTimeZone::systemTimeZoneId()));
    update();
}

bool Clock::isSystemTimeZone() const
{
    return !m_timeZoneExplicitlySet;
}

QString Clock::timeZoneCode() const
{
    return m_timeZone.displayName(now(), QTimeZone::ShortName);
}

QString Clock::timeZoneName() const
{
    return m_timeZone.displayName(now(), QTimeZone::LongName);
}

QString Clock::timeZoneOffset() const
{
    return m_timeZone.displayName(now(), QTimeZone::OffsetName);
}

const QString Clock::formattedTime() const
{
    return now().toString(m_timeFormat);
}

const QString Clock::formattedDate() const
{
    return now().toString(m_dateFormat);
}

bool Clock::isTrackSeconds() const
{
    return m_trackSeconds;
}

void Clock::setTrackSeconds(bool trackSeconds)
{
    m_trackSeconds = trackSeconds;
    setupTickConnections();
}

const QDateTime Clock::now() const
{
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    // we don't cache the system timezone so it can update
    const QTimeZone timeZone = m_timeZone.isValid() ? m_timeZone : QTimeZone::systemTimeZone();
    return nowUtc.toTimeZone(timeZone);
}

void Clock::classBegin()
{
    m_deferInit = true;
}

void Clock::componentComplete()
{
    m_deferInit = false;
    setupTickConnections();
}

void Clock::handleSystemTimezoneChanged()
{
    if (m_timeZoneExplicitlySet) {
        return;
    }
    resetTimeZone();
}

void Clock::update()
{
    if (!timezoneMetadataValid()) {
        setupTimeZone(m_timeZone);
    }
    emit timeChanged();
}

void Clock::setupTickConnections()
{
    if (m_deferInit) {
        return;
    }
    if (m_timer) {
        disconnect(m_timer.get(), nullptr, this, nullptr);
    }

    if (m_trackSeconds) {
        m_timer = AlignedTimer::getSecondTimer();
    } else {
        m_timer = AlignedTimer::getMinuteTimer();
    }

    connect(m_timer.get(), &AlignedTimer::timeout, this, &Clock::update);
}

void Clock::setupTimeZone(const QTimeZone &timeZone)
{
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    m_nextTimezoneTransition = m_timeZone.nextTransition(nowUtc).atUtc;
    m_prevTimezoneTransition = m_timeZone.previousTransition(nowUtc).atUtc;

    m_timeZone = timeZone;
    Q_EMIT timeZoneChanged();
}

bool Clock::timezoneMetadataValid()
{
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();

    if (m_nextTimezoneTransition.isValid() && nowUtc > m_nextTimezoneTransition) {
        return true;
    }
    if (m_prevTimezoneTransition.isValid() && nowUtc < m_prevTimezoneTransition) {
        return true;
    }
    return false;
}

#include "clock.moc"
