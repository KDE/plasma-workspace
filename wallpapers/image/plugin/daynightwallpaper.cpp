/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "daynightwallpaper.h"

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
    return {Kind(positiveMod(int(m_kind) - 1, 4))};
}

DayNightPhase DayNightPhase::next() const
{
    return {Kind(positiveMod(int(m_kind) + 1, 4))};
}

DayNightPhase DayNightPhase::from(KDarkLightTransition::Type type)
{
    switch (type) {
    case KDarkLightTransition::Morning:
        return Kind::Sunrise;
    case KDarkLightTransition::Evening:
        return Kind::Sunset;
    }

    Q_UNREACHABLE();
}

DayNightPhase DayNightPhase::from(const QDateTime &dateTime, const KDarkLightTransition &previousTransition, const KDarkLightTransition &nextTransition)
{
    const DayNightPhase previousPhase = from(previousTransition.type());
    switch (previousTransition.test(dateTime)) {
    case KDarkLightTransition::Upcoming:
        return previousPhase.previous();
    case KDarkLightTransition::InProgress:
        return previousPhase;
    case KDarkLightTransition::Passed:
        break;
    }

    const DayNightPhase nextPhase = from(nextTransition.type());
    switch (nextTransition.test(dateTime)) {
    case KDarkLightTransition::Upcoming:
        return nextPhase.previous();
    case KDarkLightTransition::InProgress:
        return nextPhase;
    case KDarkLightTransition::Passed:
        return nextPhase.next();
    }

    Q_UNREACHABLE();
}

DayNightSnapshot::DayNightSnapshot() = default;

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
    m_darkLightScheduleProvider = new KDarkLightScheduleProvider(m_initialState, this);
    connect(m_darkLightScheduleProvider, &KDarkLightScheduleProvider::scheduleChanged, this, [this]() {
        setState(m_darkLightScheduleProvider->state());
        schedule();
    });

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

void DayNightWallpaper::setSnapshot(const DayNightSnapshot &snapshot)
{
    if (m_snapshot != snapshot) {
        m_snapshot = snapshot;
        Q_EMIT snapshotChanged();
    }
}

void DayNightWallpaper::setInitialState(const QString &state)
{
    if (m_initialState != state) {
        m_initialState = state;
        Q_EMIT initialStateChanged();
    }
}

void DayNightWallpaper::setState(const QString &state)
{
    if (m_state != state) {
        m_state = state;
        Q_EMIT stateChanged();
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
    const KDarkLightSchedule schedule = m_darkLightScheduleProvider->schedule();

    m_previousTransition = *schedule.previousTransition(now);
    m_nextTransition = *schedule.nextTransition(now);

    m_rescheduleTimer->start(now.msecsTo(m_nextTransition.startDateTime()));
    update();
}

void DayNightWallpaper::update()
{
    const QDateTime now = QDateTime::currentDateTime();

    DayNightPhase phase = DayNightPhase::from(now, m_previousTransition, m_nextTransition);
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
        blendFactor = m_previousTransition.progress(now);
        break;
    case DayNightPhase::Day:
        bottom = m_day;
        break;
    case DayNightPhase::Sunset:
        bottom = m_day;
        top = m_night;
        blendFactor = m_previousTransition.progress(now);
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
            disjoint = std::abs(m_previousTransition.startDateTime().msecsTo(now)) > disjointThreshold;
        } else if (m_snapshot.bottom() == top && m_snapshot.top().isEmpty()) {
            // Transitioning from either sunset to day or sunrise to night.
            disjoint = std::abs(m_previousTransition.endDateTime().msecsTo(now)) > disjointThreshold;
        } else {
            // Transitioning from either day to night or night to day. Or it's a new wallpaper.
            disjoint = true;
        }
    }

    setSnapshot(DayNightSnapshot(now, bottom, top, blendFactor, disjoint));
}
