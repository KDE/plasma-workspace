/*
    SPDX-FileCopyrightText: 2021 Emily Ehlert
    SPDX-FileCopyrightText: 2024 Ismael Asensio <isma.af@gmail.com>

    Based upon BBC Weather Ion and ENV Canada Ion by Shawn Starr
    SPDX-FileCopyrightText: 2007-2009 Shawn Starr <shawn.starr@rogers.com>

    also

    the wetter.com Ion by Thilo-Alexander Ginkel
    SPDX-FileCopyrightText: 2009 Thilo-Alexander Ginkel <thilo@ginkel.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Ion for weather data from Deutscher Wetterdienst (DWD) / German Weather Service */

#pragma once

#include "../ion.h"

#include <QList>

class KJob;
namespace KIO
{
class Job;
class TransferJob;
}

class WeatherData
{
public:
    QString place;

    // Current observation information.
    QDateTime observationDateTime;
    QDateTime sunriseTime;
    QDateTime sunsetTime;

    QString condIconNumber;
    QString windDirection;
    float temperature = qQNaN();
    float humidity = qQNaN();
    float pressure = qQNaN();
    float windSpeed = qQNaN();
    float gustSpeed = qQNaN();
    float dewpoint = qQNaN();

    // If current observations not available, use forecast data for current day
    QString windDirectionAlt;
    float windSpeedAlt = qQNaN();
    float gustSpeedAlt = qQNaN();

    // 7 forecast
    struct ForecastInfo {
        QDateTime period;
        QString iconName;
        QString summary;
        float tempHigh = qQNaN();
        float tempLow = qQNaN();
        int precipitation = 0;
        float windSpeed = qQNaN();
        QString windDirection;
    };

    // 7 day forecast
    QList<WeatherData::ForecastInfo> forecasts;

    struct WarningInfo {
        QString type;
        int priority = 0;
        QString headline;
        QString description;
        QDateTime timestamp;
    };

    QList<WeatherData::WarningInfo> warnings;

    bool isForecastsDataPending = false;
    bool isMeasureDataPending = false;
};

class Q_DECL_EXPORT DWDIon : public IonInterface
{
    Q_OBJECT

public:
    DWDIon(QObject *parent);

public: // IonInterface API
    bool updateIonSource(const QString &source) override;

protected: // IonInterface API
    void reset() override;

private Q_SLOTS:
    void setup_slotJobFinished(KJob *);
    void measure_slotJobFinished(KJob *);
    void forecast_slotJobFinished(KJob *);

private:
    QMap<QString, ConditionIcons> getUniversalIcons() const;
    QMap<QString, ConditionIcons> setupDayIconMappings() const;
    QMap<QString, ConditionIcons> setupNightIconMappings() const;
    QMap<QString, WindDirections> setupWindIconMappings() const;

    QMap<QString, ConditionIcons> const &dayIcons() const;
    QMap<QString, ConditionIcons> const &nightIcons() const;
    QMap<QString, WindDirections> const &windIcons() const;

    KJob *requestAPIJob(const QString &source, const QUrl &url);

    void findPlace(const QString &searchText);
    void parseStationData(const QByteArray &data);
    void searchInStationList(const QString &place);

    void validate(const QString &source);

    void fetchWeather(const QString &placeName, const QString &placeID);
    void parseForecastData(const QString &source, const QJsonDocument &doc);
    void parseMeasureData(const QString &source, const QJsonDocument &doc);
    void updateWeather(const QString &source);

    // Helper methods
    QString camelCaseString(const QString &text) const;
    QString roundWindDirections(int windDirection) const;
    bool isNightTime(const WeatherData &weatherData) const;
    float parseNumber(const QVariant &number) const;
    QDateTime parseDateFromMSecs(const QVariant &timestamp) const;

private:
    // Key dicts
    QMap<QString, QString> m_place;
    QList<QString> m_locations;

    // Weather information
    QHash<QString, WeatherData> m_weatherData;

    QHash<KJob *, std::shared_ptr<QByteArray>> m_jobData;
    QHash<KJob *, QString> m_jobList;

    QStringList m_sourcesToReset;
};
