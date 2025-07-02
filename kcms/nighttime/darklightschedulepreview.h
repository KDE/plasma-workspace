/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QGeoCoordinate>
#include <QObject>
#include <QQmlParserStatus>

class KDarkLightTransition;

class DarkLightSchedulePreview : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate RESET resetCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(QString sunsetStart READ sunsetStart WRITE setSunsetStart NOTIFY sunsetStartChanged)
    Q_PROPERTY(QString sunriseStart READ sunriseStart WRITE setSunriseStart NOTIFY sunriseStartChanged)
    Q_PROPERTY(int transitionDuration READ transitionDuration WRITE setTransitionDuration NOTIFY transitionDurationChanged)
    Q_PROPERTY(QDateTime startSunriseDateTime READ startSunriseDateTime NOTIFY startSunriseDateTimeChanged)
    Q_PROPERTY(QDateTime endSunriseDateTime READ endSunriseDateTime NOTIFY endSunriseDateTimeChanged)
    Q_PROPERTY(QDateTime startSunsetDateTime READ startSunsetDateTime NOTIFY startSunsetDateTimeChanged)
    Q_PROPERTY(QDateTime endSunsetDateTime READ endSunsetDateTime NOTIFY endSunsetDateTimeChanged)
    Q_PROPERTY(FallbackReason fallbackReason READ fallbackReason NOTIFY fallbackReasonChanged)

public:
    enum class FallbackReason {
        None,
        PolarDay,
        PolarNight,
        Other,
    };
    Q_ENUM(FallbackReason)

    explicit DarkLightSchedulePreview(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    QGeoCoordinate coordinate() const;
    void setCoordinate(const QGeoCoordinate &coordinate);
    void resetCoordinate();

    QString sunsetStart() const;
    void setSunsetStart(const QString &start);

    QString sunriseStart() const;
    void setSunriseStart(const QString &start);

    int transitionDuration() const;
    void setTransitionDuration(int duration);

    QDateTime startSunriseDateTime() const;
    void setStartSunriseDateTime(const QDateTime &dateTime);

    QDateTime endSunriseDateTime() const;
    void setEndSunriseDateTime(const QDateTime &dateTime);

    QDateTime startSunsetDateTime() const;
    void setStartSunsetDateTime(const QDateTime &dateTime);

    QDateTime endSunsetDateTime() const;
    void setEndSunsetDateTime(const QDateTime &dateTime);

    FallbackReason fallbackReason() const;
    void setFallbackReason(FallbackReason reason);

Q_SIGNALS:
    void coordinateChanged();
    void sunsetStartChanged();
    void sunriseStartChanged();
    void transitionDurationChanged();
    void startSunriseDateTimeChanged();
    void endSunriseDateTimeChanged();
    void startSunsetDateTimeChanged();
    void endSunsetDateTimeChanged();
    void fallbackReasonChanged();

private:
    void recalculate();
    void apply(const KDarkLightTransition &transition);

    QGeoCoordinate m_coordinate;
    QString m_sunsetStart;
    QString m_sunriseStart;
    std::chrono::milliseconds m_transitionDuration = std::chrono::milliseconds::zero();
    QDateTime m_startSunriseDateTime;
    QDateTime m_endSunriseDateTime;
    QDateTime m_startSunsetDateTime;
    QDateTime m_endSunsetDateTime;
    FallbackReason m_fallbackReason = FallbackReason::None;
    bool m_complete = false;
};
