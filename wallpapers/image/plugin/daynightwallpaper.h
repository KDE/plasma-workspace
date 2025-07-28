/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KDarkLightScheduleProvider>
#include <KSystemClockSkewNotifier>

#include <QDateTime>
#include <QObject>
#include <QQmlParserStatus>
#include <QTimer>
#include <QUrl>
#include <qqmlintegration.h>

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

    static DayNightPhase from(KDarkLightTransition::Type type);
    static DayNightPhase from(const QDateTime &dateTime, const KDarkLightTransition &previousTransition, const KDarkLightTransition &nextTransition);

private:
    Kind m_kind;
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
    QML_ELEMENT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(DayNightSnapshot snapshot READ snapshot NOTIFY snapshotChanged)
    Q_PROPERTY(QString initialState READ initialState WRITE setInitialState NOTIFY initialStateChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)

public:
    explicit DayNightWallpaper(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    QUrl source() const;
    void setSource(const QUrl &source);

    DayNightSnapshot snapshot() const;
    void setSnapshot(const DayNightSnapshot &snapshot);

    QString initialState() const;
    void setInitialState(const QString &state);

    QString state() const;
    void setState(const QString &state);

Q_SIGNALS:
    void sourceChanged();
    void snapshotChanged();
    void initialStateChanged();
    void stateChanged();

private:
    void load();
    void schedule();
    void update();

    KDarkLightScheduleProvider *m_darkLightScheduleProvider = nullptr;
    KSystemClockSkewNotifier *m_systemClockMonitor;

    QUrl m_source;
    QUrl m_day;
    QUrl m_night;
    bool m_crossfade = true;

    QString m_initialState;
    QString m_state;

    DayNightSnapshot m_snapshot;
    KDarkLightTransition m_previousTransition;
    KDarkLightTransition m_nextTransition;
    QTimer *m_blendTimer;
    QTimer *m_rescheduleTimer;

    bool m_complete = false;
};

inline DayNightPhase::operator DayNightPhase::Kind() const
{
    return m_kind;
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

inline QString DayNightWallpaper::initialState() const
{
    return m_initialState;
}

inline QString DayNightWallpaper::state() const
{
    return m_state;
}

inline DayNightSnapshot DayNightWallpaper::snapshot() const
{
    return m_snapshot;
}
