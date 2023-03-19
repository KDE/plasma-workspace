/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "clock.h"

#include <QDBusConnection>
#include <QDebug>

#include "alignedtimer.h"

class SystemTimeZone : public QObject
{
    Q_OBJECT
public:
    SystemTimeZone();
    QTimeZone timeZone() const
    {
        return QTimeZone::systemTimeZone();
    }
Q_SIGNALS:
    void timeZoneChanged();
};

Q_GLOBAL_STATIC(SystemTimeZone, s_systemTimezone)

SystemTimeZone::SystemTimeZone()
{
    auto sessionBus = QDBusConnection::sessionBus();
    sessionBus.connect(QString(), QString(), QStringLiteral("org.kde.KTimeZoned"), QStringLiteral("timeZoneChanged"), this, SIGNAL(timeZoneChanged()));
}

Clock::Clock(QObject *parent)
    : QObject{parent}
{
    connect(s_systemTimezone, &SystemTimeZone::timeZoneChanged, this, [this]() {
        if (!m_timeZoneExplicitlySet) {
            setupTimeZone(s_systemTimezone->timeZone());
            update();
            Q_EMIT isSystemTimeZoneChanged();
        }
    });
    connect(this, &Clock::timeZoneChanged, this, &Clock::isSystemTimeZoneChanged);
}

bool Clock::isValid() const
{
    return m_timeZone.isValid();
}
 QByteArray Clock::timeZone() const
{
    return m_timeZone.id();
}

void Clock::setTimeZone(const QByteArray &ianaId)
{
    if (m_timeZone.id() == ianaId) {
        return;
    }

    const QTimeZone timeZone = QTimeZone(ianaId);
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
    return m_timeZone == QTimeZone(QTimeZone::systemTimeZone());
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

bool Clock::trackSeconds() const
{
    return m_trackSeconds;
}

void Clock::setTrackSeconds(bool trackSeconds)
{
    m_trackSeconds = trackSeconds;
    setupTickConnections();
}

QDateTime Clock::jsDateTime() const
{
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    auto systemTime = nowUtc.toTimeZone(QTimeZone::systemTimeZone());
    auto tzTime = nowUtc.toTimeZone(m_timeZone);

    // Javascript doesn't have a concept of timezones, only a value from UTC in the system timezone
    return nowUtc.addSecs(-systemTime.offsetFromUtc() + tzTime.offsetFromUtc());
}

QDateTime Clock::now() const
{
    QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    return nowUtc.toTimeZone(m_timeZone);
}

void Clock::classBegin()
{
    m_deferInit = true;
}

void Clock::componentComplete()
{
    m_deferInit = false;
    if (!m_timeZoneExplicitlySet) {
        setupTimeZone(QTimeZone(QTimeZone::systemTimeZoneId()));
    }
    setupTickConnections();
}

void Clock::update()
{
    // Even though we may be on Europe/London, we still change between GMT and BST
    // Refresh each time we pass a transition point
    if (!timezoneMetadataValid()) {
        setupTimeZone(m_timeZone);
    }

    Q_EMIT timeChanged();
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
        return false;
    }
    if (m_prevTimezoneTransition.isValid() && nowUtc < m_prevTimezoneTransition) {
        return false;
    }
    return true;
}

#include "clock.moc"
