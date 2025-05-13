/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KSystemClockSkewNotifier>

#include <QDateTime>
#include <QGeoCoordinate>
#include <QObject>
#include <QQmlParserStatus>
#include <QTimer>
#include <QUrl>

/**
 * The DayNightPhase type specifies a period of time in a day, e.g. sunrise or sunset, etc.
 */
class DayNightPhase
{
public:
    enum Kind {
        Night,
        Sunrise,
        Day,
        Sunset,
    };

    DayNightPhase();
    DayNightPhase(Kind kind);

    operator Kind() const;

    DayNightPhase previous() const;
    DayNightPhase next() const;

private:
    Kind m_kind;
};

/**
 * The DayNightTransition type specifies a transition from one phase to another.
 */
class DayNightTransition
{
public:
    enum Relation {
        Before,
        Inside,
        After,
    };

    DayNightTransition();
    DayNightTransition(DayNightPhase phase, const QDateTime &start, const QDateTime &end);

    bool isValid() const;
    Relation test(const QDateTime &dateTime) const;
    qreal progress(const QDateTime &dateTime) const;

    DayNightPhase phase() const;
    QDateTime start() const;
    QDateTime end() const;

private:
    DayNightPhase m_phase;
    QDateTime m_start;
    QDateTime m_end;
};

/**
 * The DayNightSchedule type provides scheduling information about the previous and the next transition.
 */
class DayNightSchedule
{
public:
    static DayNightSchedule create(const QDateTime &dateTime);
    static DayNightSchedule create(const QDateTime &dateTime, const QGeoCoordinate &coordinate);

    DayNightSchedule();
    DayNightSchedule(const DayNightTransition &previous, const DayNightTransition &next);

    bool isValid() const;
    DayNightPhase phase(const QDateTime &dateTime) const;

    DayNightTransition previous() const;
    DayNightTransition next() const;

private:
    DayNightTransition m_previous;
    DayNightTransition m_next;
};

/**
 * The DayNightSnapshot type specifies what images should be displayed and how much they should be
 * blended, if they need to be blended at all. The disjoint flag specifies whether this snapshot can
 * be displayed without a smooth animation from the previous snapshot.
 */
class DayNightSnapshot
{
    Q_GADGET
    Q_PROPERTY(QUrl bottom MEMBER m_bottom CONSTANT)
    Q_PROPERTY(QUrl top MEMBER m_top CONSTANT)
    Q_PROPERTY(qreal blendFactor MEMBER m_blendFactor CONSTANT)
    Q_PROPERTY(bool disjoint MEMBER m_disjoint CONSTANT)

public:
    DayNightSnapshot();
    DayNightSnapshot(const QDateTime &timestamp, const QUrl &bottom, const QUrl &top, qreal blendFactor, bool disjoint);

    auto operator<=>(const DayNightSnapshot &other) const = default;

    bool isValid() const;
    QDateTime timestamp() const;
    QUrl bottom() const;
    QUrl top() const;
    qreal blendFactor() const;
    bool disjoint() const;

private:
    QDateTime m_timestamp;
    QUrl m_bottom;
    QUrl m_top;
    qreal m_blendFactor = 0.0;
    bool m_disjoint = false;
};

/**
 * The DayNightWallpaper type represents a dynamic wallpaper that switches between dark and light
 * images depending on time of day.
 *
 * If the device's current location is available, the DayNightWallpaper will synchronize dark and
 * light images with the position of the Sun. The dark image will be displayed at night, the light
 * image will be displayed at day.
 *
 * The transition between dark and light images happens at sunrise and sunset. Its duration matches
 * the duration of sunrise and sunset. The `"X-KDE-CrossFade": false` metadata option can be set if
 * crossfading between images is undesired.
 */
class DayNightWallpaper : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QGeoCoordinate location READ location WRITE setLocation RESET resetLocation NOTIFY locationChanged)
    Q_PROPERTY(DayNightSnapshot snapshot READ snapshot NOTIFY snapshotChanged)

public:
    explicit DayNightWallpaper(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    QUrl source() const;
    void setSource(const QUrl &source);

    QGeoCoordinate location() const;
    void setLocation(const QGeoCoordinate &location);
    void resetLocation();

    DayNightSnapshot snapshot() const;
    void setSnapshot(const DayNightSnapshot &snapshot);

Q_SIGNALS:
    void sourceChanged();
    void locationChanged();
    void snapshotChanged();

private:
    void load();
    void schedule();
    void update();

    KSystemClockSkewNotifier *m_systemClockMonitor;

    QUrl m_source;
    QGeoCoordinate m_location;
    QUrl m_day;
    QUrl m_night;
    bool m_crossfade = true;

    DayNightSnapshot m_snapshot;
    DayNightSchedule m_schedule;
    QTimer *m_blendTimer;
    QTimer *m_rescheduleTimer;

    bool m_complete = false;
};

inline DayNightPhase::operator DayNightPhase::Kind() const
{
    return m_kind;
}

inline bool DayNightTransition::isValid() const
{
    return m_start.isValid() && m_end.isValid();
}

inline DayNightPhase DayNightTransition::phase() const
{
    return m_phase;
}

inline QDateTime DayNightTransition::start() const
{
    return m_start;
}

inline QDateTime DayNightTransition::end() const
{
    return m_end;
}

inline bool DayNightSchedule::isValid() const
{
    return m_previous.isValid() && m_next.isValid();
}

inline DayNightTransition DayNightSchedule::previous() const
{
    return m_previous;
}

inline DayNightTransition DayNightSchedule::next() const
{
    return m_next;
}

inline bool DayNightSnapshot::isValid() const
{
    return m_timestamp.isValid();
}

inline QDateTime DayNightSnapshot::timestamp() const
{
    return m_timestamp;
}

inline QUrl DayNightSnapshot::bottom() const
{
    return m_bottom;
}

inline QUrl DayNightSnapshot::top() const
{
    return m_top;
}

inline qreal DayNightSnapshot::blendFactor() const
{
    return m_blendFactor;
}

inline bool DayNightSnapshot::disjoint() const
{
    return m_disjoint;
}

inline QUrl DayNightWallpaper::source() const
{
    return m_source;
}

inline QGeoCoordinate DayNightWallpaper::location() const
{
    return m_location;
}

inline DayNightSnapshot DayNightWallpaper::snapshot() const
{
    return m_snapshot;
}
