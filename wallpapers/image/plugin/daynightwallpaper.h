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

    Kind kind() const;

    DayNightPhase previous() const;
    DayNightPhase next() const;

private:
    Kind m_kind;
};

struct DayNightTransition {
    enum Relation {
        Before,
        Inside,
        After,
    };

    static int tolerance();

    bool isValid() const;

    Relation test(const QDateTime &dateTime) const;
    qreal progress(const QDateTime &dateTime) const;

    DayNightPhase phase;
    QDateTime start;
    QDateTime end;
};

struct DayNightCycle {
    static DayNightCycle create(const QDateTime &dateTime);
    static DayNightCycle create(const QDateTime &dateTime, const QGeoCoordinate &coordinate);

    bool isValid() const;

    DayNightPhase phase(const QDateTime &dateTime) const;

    DayNightTransition previous;
    DayNightTransition next;
};

class DayNightWallpaper : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QGeoCoordinate location READ location WRITE setLocation RESET resetLocation NOTIFY locationChanged)
    Q_PROPERTY(QUrl current READ current NOTIFY currentChanged)
    Q_PROPERTY(QUrl next READ next NOTIFY nextChanged)
    Q_PROPERTY(qreal blendFactor READ blendFactor NOTIFY blendFactorChanged)

public:
    explicit DayNightWallpaper(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    QUrl source() const;
    void setSource(const QUrl &source);

    QGeoCoordinate location() const;
    void setLocation(const QGeoCoordinate &location);
    void resetLocation();

    QUrl current() const;
    void setCurrent(const QUrl &current);

    QUrl next() const;
    void setNext(const QUrl &next);

    qreal blendFactor() const;
    void setBlendFactor(qreal blendFactor);

Q_SIGNALS:
    void sourceChanged();
    void locationChanged();
    void currentChanged();
    void nextChanged();
    void blendFactorChanged();

private:
    void load();
    void schedule();
    void update();

    KSystemClockSkewNotifier *m_systemClockMonitor;

    QUrl m_source;
    QGeoCoordinate m_location;
    QUrl m_day;
    QUrl m_night;

    QUrl m_current;
    QUrl m_next;
    qreal m_blendFactor;

    DayNightCycle m_cycle;
    QTimer *m_blendTimer;
    QTimer *m_scheduleTimer;

    bool m_complete = false;
};
