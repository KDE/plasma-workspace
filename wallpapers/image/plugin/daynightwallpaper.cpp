/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "daynightwallpaper.h"

#include <KHolidays/SunEvents>
#include <KPluginMetaData>

#include <QUrlQuery>

DayNightPhase::DayNightPhase()
    : m_kind(Night)
{
}

DayNightPhase::DayNightPhase(Kind kind)
    : m_kind(kind)
{
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

DayNightTransition::DayNightTransition()
{
}

DayNightTransition::DayNightTransition(DayNightPhase phase, const QDateTime &start, const QDateTime &end)
    : m_phase(phase)
    , m_start(start)
    , m_end(end)
{
}

DayNightTransition::Relation DayNightTransition::test(const QDateTime &dateTime) const
{
    // Timers can fire earlier than expected, handle it by relaxing the time checks.
    const int tolerance = 60;
    if (dateTime.secsTo(m_start) > tolerance) {
        return Relation::Before;
    } else if (dateTime.secsTo(m_end) > tolerance) {
        return Relation::Inside;
    } else {
        return Relation::After;
    }
}

qreal DayNightTransition::progress(const QDateTime &dateTime) const
{
    const qreal elapsed = m_start.secsTo(dateTime);
    const qreal total = m_start.secsTo(m_end);
    return std::clamp<qreal>(elapsed / total, 0.0, 1.0);
}

DayNightSchedule DayNightSchedule::create(const QDateTime &dateTime)
{
    const QDateTime todayMorning(dateTime.date(), QTime(6, 0));
    const QDateTime todayEvening(dateTime.date(), QTime(18, 0));

    const QTime morning = QTime(6, 0);
    const QTime evening = QTime(18, 0);
    const int transitionDuration = 30 * 60;

    const int tolerance = 60;
    const bool passedMorning = dateTime.time().secsTo(morning) <= tolerance;
    const bool passedEvening = dateTime.time().secsTo(evening) <= tolerance;

    const QDateTime nextEarlyMorning = QDateTime(dateTime.date().addDays(passedMorning), morning);
    const QDateTime nextLateMorning = nextEarlyMorning.addSecs(transitionDuration);
    const QDateTime nextEarlyEvening = QDateTime(dateTime.date().addDays(passedEvening), evening);
    const QDateTime nextLateEvening = nextEarlyEvening.addSecs(transitionDuration);

    if (nextEarlyEvening < nextEarlyMorning) {
        return DayNightSchedule(DayNightTransition(DayNightPhase::Sunrise, nextEarlyMorning.addDays(-1), nextLateMorning.addDays(-1)),
                                DayNightTransition(DayNightPhase::Sunset, nextEarlyEvening, nextLateEvening));
    } else {
        return DayNightSchedule(DayNightTransition(DayNightPhase::Sunset, nextEarlyEvening.addDays(-1), nextLateEvening.addDays(-1)),
                                DayNightTransition(DayNightPhase::Sunrise, nextEarlyMorning, nextLateMorning));
    }
}

DayNightSchedule DayNightSchedule::create(const QDateTime &dateTime, const QGeoCoordinate &coordinate)
{
    const KHolidays::SunEvents todayEvents(dateTime, coordinate.latitude(), coordinate.longitude());

    const auto todaySunrise = DayNightTransition(DayNightPhase::Sunrise, todayEvents.civilDawn(), todayEvents.sunrise());
    const auto todaySunset = DayNightTransition(DayNightPhase::Sunset, todayEvents.sunset(), todayEvents.civilDusk());
    if (!todaySunrise.isValid() || !todaySunset.isValid()) {
        return DayNightSchedule();
    }

    switch (todaySunrise.test(dateTime)) {
    case DayNightTransition::Before: {
        const KHolidays::SunEvents yesterdayEvents(dateTime.addDays(-1), coordinate.latitude(), coordinate.longitude());

        const auto yesterdaySunset = DayNightTransition(DayNightPhase::Sunset, yesterdayEvents.sunset(), yesterdayEvents.civilDusk());
        if (!yesterdaySunset.isValid()) {
            return DayNightSchedule();
        }

        return DayNightSchedule(yesterdaySunset, todaySunrise);
    }

    case DayNightTransition::Inside:
        return DayNightSchedule(todaySunrise, todaySunset);

    case DayNightTransition::After:
        break;
    }

    switch (todaySunset.test(dateTime)) {
    case DayNightTransition::Before:
        return DayNightSchedule(todaySunrise, todaySunset);

    case DayNightTransition::Inside:
    case DayNightTransition::After: {
        const KHolidays::SunEvents tomorrowEvents(dateTime.addDays(1), coordinate.latitude(), coordinate.longitude());

        const auto tomorrowSunrise = DayNightTransition(DayNightPhase::Sunrise, tomorrowEvents.civilDawn(), tomorrowEvents.sunrise());
        if (!tomorrowSunrise.isValid()) {
            return DayNightSchedule();
        }

        return DayNightSchedule(todaySunset, tomorrowSunrise);
    }
    }

    Q_UNREACHABLE();
}

DayNightSchedule::DayNightSchedule()
{
}

DayNightSchedule::DayNightSchedule(const DayNightTransition &previous, const DayNightTransition &next)
    : m_previous(previous)
    , m_next(next)
{
}

DayNightPhase DayNightSchedule::phase(const QDateTime &dateTime) const
{
    switch (m_previous.test(dateTime)) {
    case DayNightTransition::Before:
        return m_previous.phase().previous();
    case DayNightTransition::Inside:
        return m_previous.phase();
    case DayNightTransition::After:
        break;
    }

    switch (m_next.test(dateTime)) {
    case DayNightTransition::Before:
        return m_next.phase().previous();
    case DayNightTransition::Inside:
        return m_next.phase();
    case DayNightTransition::After:
        return m_next.phase().next();
    }

    Q_UNREACHABLE();
}

DayNightSnapshot::DayNightSnapshot()
{
}

DayNightSnapshot::DayNightSnapshot(const QDateTime &timestamp, const QUrl &bottom, const QUrl &top, qreal blendFactor, bool disjoint)
    : m_timestamp(timestamp)
    , m_bottom(bottom)
    , m_top(top)
    , m_blendFactor(blendFactor)
    , m_disjoint(disjoint)
{
}

DayNightWallpaper::DayNightWallpaper(QObject *parent)
    : QObject(parent)
    , m_systemClockMonitor(new KSystemClockSkewNotifier(this))
    , m_blendTimer(new QTimer(this))
    , m_rescheduleTimer(new QTimer(this))
{
    m_blendTimer->setSingleShot(true);
    connect(m_blendTimer, &QTimer::timeout, this, &DayNightWallpaper::update);

    m_systemClockMonitor->setActive(true);
    connect(m_systemClockMonitor, &KSystemClockSkewNotifier::skewed, this, &DayNightWallpaper::schedule);

    m_rescheduleTimer->setSingleShot(true);
    connect(m_rescheduleTimer, &QTimer::timeout, this, &DayNightWallpaper::schedule);
}

void DayNightWallpaper::classBegin()
{
}

void DayNightWallpaper::componentComplete()
{
    load();
    m_complete = true;
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

void DayNightWallpaper::setSnapshot(const DayNightSnapshot &snapshot)
{
    if (m_snapshot != snapshot) {
        m_snapshot = snapshot;
        Q_EMIT snapshotChanged();
    }
}

void DayNightWallpaper::load()
{
    const QString rootPath = m_source.toLocalFile();

    auto metaData = KPluginMetaData::fromJsonFile(rootPath + u"/metadata.json");
    m_crossfade = metaData.value(u"X-KDE-CrossFade", true);

    m_day = QUrl(QStringLiteral("image://package/get"));
    m_day.setQuery({
        std::make_pair(QStringLiteral("dir"), rootPath),
        std::make_pair(QStringLiteral("darkMode"), QStringLiteral("0")),
    });

    m_night = QUrl(QStringLiteral("image://package/get"));
    m_night.setQuery({
        std::make_pair(QStringLiteral("dir"), rootPath),
        std::make_pair(QStringLiteral("darkMode"), QStringLiteral("1")),
    });

    schedule();
}

void DayNightWallpaper::schedule()
{
    const QDateTime now = QDateTime::currentDateTime();

    m_schedule = DayNightSchedule();
    if (m_location.isValid()) {
        m_schedule = DayNightSchedule::create(now, m_location);
    }
    if (!m_schedule.isValid()) {
        m_schedule = DayNightSchedule::create(now);
    }

    m_rescheduleTimer->start(now.msecsTo(m_schedule.next().start()));
    update();
}

void DayNightWallpaper::update()
{
    const QDateTime now = QDateTime::currentDateTime();

    DayNightPhase phase = m_schedule.phase(now);
    if (!m_crossfade) {
        if (phase == DayNightPhase::Sunrise) {
            phase = DayNightPhase::Day;
        } else if (phase == DayNightPhase::Sunset) {
            phase = DayNightPhase::Night;
        }
    }

    QUrl bottom;
    QUrl top;
    qreal blendFactor = 0.0;
    switch (phase) {
    case DayNightPhase::Night:
        bottom = m_night;
        break;
    case DayNightPhase::Sunrise:
        bottom = m_night;
        top = m_day;
        blendFactor = m_schedule.previous().progress(now);
        break;
    case DayNightPhase::Day:
        bottom = m_day;
        break;
    case DayNightPhase::Sunset:
        bottom = m_day;
        top = m_night;
        blendFactor = m_schedule.previous().progress(now);
        break;
    }

    const int blendInterval = 60000;
    switch (phase) {
    case DayNightPhase::Night:
    case DayNightPhase::Day:
        m_blendTimer->stop();
        break;
    case DayNightPhase::Sunrise:
    case DayNightPhase::Sunset:
        m_blendTimer->start(blendInterval);
        break;
    }

    // An update is considered discontinuous if it's blendInterval away from the previous. However,
    // in order to handle timers being imprecise, we add another blendInterval on top.
    bool disjoint = false;
    if (m_snapshot.isValid()) {
        const int disjointThreshold = 2 * blendInterval;
        if (m_snapshot.bottom() == bottom && m_snapshot.top() == top) {
            if (!m_snapshot.top().isEmpty()) {
                disjoint = std::abs(m_snapshot.timestamp().msecsTo(now)) > disjointThreshold;
            }
        } else if (m_snapshot.bottom() == bottom && m_snapshot.top().isEmpty()) {
            // Transitioning from either day to sunset or night to sunrise.
            disjoint = std::abs(m_schedule.previous().start().msecsTo(now)) > disjointThreshold;
        } else if (m_snapshot.bottom() == top && m_snapshot.top().isEmpty()) {
            // Transitioning from either sunset to day or sunrise to night.
            disjoint = std::abs(m_schedule.previous().end().msecsTo(now)) > disjointThreshold;
        } else {
            // Transitioning from either day to night or night to day. Or it's a new wallpaper.
            disjoint = true;
        }
    }

    setSnapshot(DayNightSnapshot(now, bottom, top, blendFactor, disjoint));
}
