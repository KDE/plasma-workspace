/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "daynightwallpaper.h"

#include <KHolidays/SunEvents>

#include <QDebug>
#include <QUrlQuery>

DayNightPhase::DayNightPhase()
    : m_kind(Night)
{
}

DayNightPhase::DayNightPhase(Kind kind)
    : m_kind(kind)
{
}

DayNightPhase::Kind DayNightPhase::kind() const
{
    return m_kind;
}

static int positiveMod(int m, int n)
{
    return (n + (m % n)) % n;
}

DayNightPhase DayNightPhase::previous() const
{
    return DayNightPhase(Kind(positiveMod(int(m_kind) - 1, 4)));
}

DayNightPhase DayNightPhase::next() const
{
    return DayNightPhase(Kind(positiveMod(int(m_kind) + 1, 4)));
}

int DayNightTransition::tolerance()
{
    return 60;
}

bool DayNightTransition::isValid() const
{
    return start.isValid() && end.isValid();
}

DayNightTransition::Relation DayNightTransition::test(const QDateTime &dateTime) const
{
    if (dateTime.secsTo(start) > tolerance()) {
        return Relation::Before;
    } else if (dateTime.secsTo(end) > tolerance()) {
        return Relation::Inside;
    } else {
        return Relation::After;
    }
}

qreal DayNightTransition::progress(const QDateTime &dateTime) const
{
    const qreal elapsed = start.secsTo(dateTime);
    const qreal total = start.secsTo(end);
    return std::clamp<qreal>(elapsed / total, 0.0, 1.0);
}

DayNightCycle DayNightCycle::create(const QDateTime &dateTime)
{
    const QDateTime todayMorning(dateTime.date(), QTime(6, 0));
    const QDateTime todayEvening(dateTime.date(), QTime(18, 0));

    const QTime morning = QTime(6, 0);
    const QTime evening = QTime(18, 0);
    const int transitionDuration = 30 * 60;

    const bool passedMorning = dateTime.time().secsTo(morning) <= DayNightTransition::tolerance();
    const bool passedEvening = dateTime.time().secsTo(evening) <= DayNightTransition::tolerance();

    const QDateTime nextEarlyMorning = QDateTime(dateTime.date().addDays(passedMorning), morning);
    const QDateTime nextLateMorning = nextEarlyMorning.addSecs(transitionDuration);
    const QDateTime nextEarlyEvening = QDateTime(dateTime.date().addDays(passedEvening), evening);
    const QDateTime nextLateEvening = nextEarlyEvening.addSecs(transitionDuration);

    if (nextEarlyEvening < nextEarlyMorning) {
        return DayNightCycle{
            .previous = DayNightTransition{
                .phase = DayNightPhase::Sunrise,
                .start = nextEarlyMorning.addDays(-1),
                .end = nextLateMorning.addDays(-1),
            },
            .next = DayNightTransition{
                .phase = DayNightPhase::Sunset,
                .start = nextEarlyEvening,
                .end = nextLateEvening,
            },
        };
    } else {
        return DayNightCycle{
            .previous = DayNightTransition{
                .phase = DayNightPhase::Sunset,
                .start = nextEarlyEvening.addDays(-1),
                .end = nextLateEvening.addDays(-1),
            },
            .next = DayNightTransition{
                .phase = DayNightPhase::Sunrise,
                .start = nextEarlyMorning,
                .end = nextLateMorning,
            },
        };
    }
}

DayNightCycle DayNightCycle::create(const QDateTime &dateTime, const QGeoCoordinate &coordinate)
{
    const KHolidays::SunEvents todayEvents(dateTime, coordinate.latitude(), coordinate.longitude());

    const auto todaySunrise = DayNightTransition{
        .phase = DayNightPhase::Sunrise,
        .start = todayEvents.civilDawn(),
        .end = todayEvents.sunrise(),
    };

    const auto todaySunset = DayNightTransition{
        .phase = DayNightPhase::Sunset,
        .start = todayEvents.sunset(),
        .end = todayEvents.civilDusk(),
    };

    if (!todaySunrise.isValid() || !todaySunset.isValid()) {
        return DayNightCycle();
    }

    switch (todaySunrise.test(dateTime)) {
    case DayNightTransition::Before: {
        const KHolidays::SunEvents yesterdayEvents(dateTime.addDays(-1), coordinate.latitude(), coordinate.longitude());

        const auto yesterdaySunset = DayNightTransition{
            .phase = DayNightPhase::Sunset,
            .start = yesterdayEvents.sunset(),
            .end = yesterdayEvents.civilDusk(),
        };

        if (!yesterdaySunset.isValid()) {
            return DayNightCycle();
        }

        return DayNightCycle{
            .previous = yesterdaySunset,
            .next = todaySunrise,
        };
    }
    case DayNightTransition::Inside:
        return DayNightCycle{
            .previous = todaySunrise,
            .next = todaySunset,
        };
    case DayNightTransition::After:
        break;
    }

    switch (todaySunset.test(dateTime)) {
    case DayNightTransition::Before:
        return DayNightCycle{
            .previous = todaySunrise,
            .next = todaySunset,
        };
    case DayNightTransition::Inside:
    case DayNightTransition::After: {
        const KHolidays::SunEvents tomorrowEvents(dateTime.addDays(1), coordinate.latitude(), coordinate.longitude());

        const auto tomorrowSunrise = DayNightTransition{
            .phase = DayNightPhase::Sunrise,
            .start = tomorrowEvents.civilDawn(),
            .end = tomorrowEvents.sunrise(),
        };

        if (!tomorrowSunrise.isValid()) {
            return DayNightCycle();
        }

        return DayNightCycle{
            .previous = todaySunset,
            .next = tomorrowSunrise,
        };
    }
    }

    Q_UNREACHABLE();
}

bool DayNightCycle::isValid() const
{
    return previous.isValid() && next.isValid();
}

DayNightPhase DayNightCycle::phase(const QDateTime &dateTime) const
{
    switch (previous.test(dateTime)) {
    case DayNightTransition::Before:
        return previous.phase.previous();
    case DayNightTransition::Inside:
        return previous.phase;
    case DayNightTransition::After:
        break;
    }

    switch (next.test(dateTime)) {
    case DayNightTransition::Before:
        return next.phase.previous();
    case DayNightTransition::Inside:
        return next.phase;
    case DayNightTransition::After:
        return next.phase.next();
    }

    Q_UNREACHABLE();
}

DayNightWallpaper::DayNightWallpaper(QObject *parent)
    : QObject(parent)
    , m_systemClockMonitor(new KSystemClockSkewNotifier(this))
    , m_blendTimer(new QTimer(this))
    , m_scheduleTimer(new QTimer(this))
{
    m_blendTimer->setSingleShot(true);
    connect(m_blendTimer, &QTimer::timeout, this, &DayNightWallpaper::update);

    m_systemClockMonitor->setActive(true);
    connect(m_systemClockMonitor, &KSystemClockSkewNotifier::skewed, this, &DayNightWallpaper::schedule);

    m_scheduleTimer->setSingleShot(true);
    connect(m_scheduleTimer, &QTimer::timeout, this, &DayNightWallpaper::schedule);
}

void DayNightWallpaper::classBegin()
{
}

void DayNightWallpaper::componentComplete()
{
    load();
    m_complete = true;
}

QUrl DayNightWallpaper::source() const
{
    return m_source;
}

void DayNightWallpaper::setSource(const QUrl &source)
{
    if (m_source != source) {
        m_source = source;
        if (m_complete) {
            load();
        }
        Q_EMIT sourceChanged();
    }
}

QGeoCoordinate DayNightWallpaper::location() const
{
    return m_location;
}

void DayNightWallpaper::setLocation(const QGeoCoordinate &location)
{
    const int minDistance = 50 * 1000;
    if (m_location.isValid() && m_location.distanceTo(location) < minDistance) {
        return;
    }

    m_location = location;
    if (m_complete) {
        schedule();
    }

    Q_EMIT locationChanged();
}

void DayNightWallpaper::resetLocation()
{
    if (m_location.isValid()) {
        m_location = QGeoCoordinate();
        if (m_complete) {
            schedule();
        }
        Q_EMIT locationChanged();
    }
}

QUrl DayNightWallpaper::current() const
{
    return m_current;
}

void DayNightWallpaper::setCurrent(const QUrl &current)
{
    if (m_current != current) {
        m_current = current;
        Q_EMIT currentChanged();
    }
}

QUrl DayNightWallpaper::next() const
{
    return m_next;
}

void DayNightWallpaper::setNext(const QUrl &next)
{
    if (m_next != next) {
        m_next = next;
        Q_EMIT nextChanged();
    }
}

qreal DayNightWallpaper::blendFactor() const
{
    return m_blendFactor;
}

void DayNightWallpaper::setBlendFactor(qreal blendFactor)
{
    if (m_blendFactor != blendFactor) {
        m_blendFactor = blendFactor;
        Q_EMIT blendFactorChanged();
    }
}

void DayNightWallpaper::load()
{
    m_day = QUrl(QStringLiteral("image://package/get"));
    m_day.setQuery({
        std::make_pair(QStringLiteral("dir"), m_source.toLocalFile()),
        std::make_pair(QStringLiteral("darkMode"), QStringLiteral("0")),
    });

    m_night = QUrl(QStringLiteral("image://package/get"));
    m_night.setQuery({
        std::make_pair(QStringLiteral("dir"), m_source.toLocalFile()),
        std::make_pair(QStringLiteral("darkMode"), QStringLiteral("1")),
    });

    schedule();
}

void DayNightWallpaper::schedule()
{
    const QDateTime now = QDateTime::currentDateTime();

    m_cycle = DayNightCycle();
    if (m_location.isValid()) {
        m_cycle = DayNightCycle::create(now, m_location);
    }
    if (!m_cycle.isValid()) {
        m_cycle = DayNightCycle::create(now);
    }

    m_scheduleTimer->start(now.msecsTo(m_cycle.next.start));
    update();
}

void DayNightWallpaper::update()
{
    const QDateTime now = QDateTime::currentDateTime();
    const auto phase = m_cycle.phase(now);

    switch (phase.kind()) {
    case DayNightPhase::Night:
        setCurrent(m_night);
        setNext(QUrl());
        break;
    case DayNightPhase::Sunrise:
        setCurrent(m_night);
        setNext(m_day);
        break;
    case DayNightPhase::Day:
        setCurrent(m_day);
        setNext(QUrl());
        break;
    case DayNightPhase::Sunset:
        setCurrent(m_day);
        setNext(m_night);
        break;
    }

    switch (phase.kind()) {
    case DayNightPhase::Night:
    case DayNightPhase::Day:
        setBlendFactor(0.0);
        m_blendTimer->stop();
        break;
    case DayNightPhase::Sunrise:
    case DayNightPhase::Sunset:
        setBlendFactor(m_cycle.previous.progress(now));
        m_blendTimer->start(60000);
        break;
    }
}
