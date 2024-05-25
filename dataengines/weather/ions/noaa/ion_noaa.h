/*
    SPDX-FileCopyrightText: 2007-2009, 2019 Shawn Starr <shawn.starr@rogers.com>
    SPDX-FileCopyrightText: 2024 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Ion for NOAA's National Weather Service openAPI data */

#pragma once

#include "../ion.h"

#include <Plasma5Support/DataEngineConsumer>

#include <KUnitConversion/Converter>
#include <QDateTime>
#include <QXmlStreamReader>

class KJob;
namespace KIO
{
class Job;
} // namespace KIO

class WeatherData
{
public:
    QString locationName;
    QString stationID;
    double stationLatitude = qQNaN();
    double stationLongitude = qQNaN();
    QString stateName;
    QString countyID;
    QString forecastUrl;

    // Current observation information.
    struct Observation {
        QDateTime timestamp;
        QString weather;
        float temperature_F = qQNaN();
        float humidity = qQNaN();
        float windDirection = qQNaN();
        float windSpeed = qQNaN();
        float windGust = qQNaN();
        float pressure = qQNaN();
        float dewpoint_F = qQNaN();
        float heatindex_F = qQNaN();
        float windchill_F = qQNaN();
        float visibility = qQNaN();
    };
    Observation observation;

    struct Forecast {
        QString day;
        QString summary;
        float low = qQNaN();
        float high = qQNaN();
        int precipitation = 0;
        bool isDayTime = true;
    };
    QList<Forecast> forecasts;

    struct Alert {
        QString headline;
        QString description;
        QString infoUrl;
        int priority;
        QDateTime startTime;
        QDateTime endTime;
    };
    QList<Alert> alerts;

    bool isForecastsDataPending = false;

    QString solarDataTimeEngineSourceName;
    bool isNight = false;
    bool isSolarDataPending = false;
};

Q_DECLARE_TYPEINFO(WeatherData::Forecast, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(WeatherData, Q_RELOCATABLE_TYPE);

class Q_DECL_EXPORT NOAAIon : public IonInterface, public Plasma5Support::DataEngineConsumer
{
    Q_OBJECT

public:
    NOAAIon(QObject *parent);
    ~NOAAIon() override;

public: // IonInterface API
    bool updateIonSource(const QString &source) override;

public Q_SLOTS:
    // for solar data pushes from the time engine
    void dataUpdated(const QString &sourceName, const Plasma5Support::DataEngine::Data &data);

Q_SIGNALS:
    void locationUpdated(const QString &source);
    void observationUpdated(const QString &source);
    void pointsInfoUpdated(const QString &source);

protected: // IonInterface API
    void reset() override;

private:
    void updateWeather(const QString &source);

    /* NOAA Methods - Internal for Ion */
    QMap<QString, ConditionIcons> setupConditionIconMappings() const;
    QMap<QString, ConditionIcons> const &conditionIcons() const;
    QMap<QString, WindDirections> setupWindIconMappings() const;
    QMap<QString, WindDirections> const &windIcons() const;

    // Current Conditions Weather info
    // bool night(const QString& source);
    IonInterface::ConditionIcons getConditionIcon(const QString &weather, bool isDayTime) const;

    // Helper to make an API request
    using Callback = void (NOAAIon::*)(const QString &, const QJsonDocument &);
    KJob *requestAPIJob(const QString &source, const QUrl &url, Callback onResult);

    // Load and parse the station list
    void getStationList(bool reset = true);
    Q_SLOT void stationListReceived(KJob *);
    bool readStationList(QXmlStreamReader &xml);
    void parseStationID(QXmlStreamReader &xml);

    // Initialize the station and location data
    void setUpStation(const QString &source);

    // Load and parse the observation data from a station
    void getObservation(const QString &source);
    void readObservation(const QString &source, const QJsonDocument &doc);

    // To know whether the local observation is day or night time
    void getSolarData(const QString &source);

    // Load and parse upcoming forecast for the next N days
    void getForecast(const QString &source);
    void readForecast(const QString &source, const QJsonDocument &doc);

    // The NOAA API is based on grid of points
    void getPointsInfo(const QString &source);
    void readPointsInfo(const QString &source, const QJsonDocument &doc);

    // Methods to get alerts.
    void getAlerts(const QString &source);
    void readAlerts(const QString &source, const QJsonDocument &doc);

    // Check if place specified is valid or not
    QStringList validate(const QString &source) const;

    // Utility method to parse XML data
    void parseUnknownElement(QXmlStreamReader &xml) const;

    // Utility methods to parse JSON data
    KUnitConversion::UnitId parseUnit(const QString &unitCode) const;
    float parseQV(const QJsonValue &qv, KUnitConversion::UnitId destUnit = KUnitConversion::InvalidUnit) const;
    QString windDirectionFromAngle(float degrees) const;

private:
    struct StationInfo {
        QString stateName;
        QString stationName;
        QString stationID;
        QPointF location;
    };

    // Station list
    QHash<QString, NOAAIon::StationInfo> m_places;

    // Weather information
    QHash<QString, WeatherData> m_weatherData;

    // Store KIO jobs
    QHash<KJob *, QByteArray> m_jobData;

    KUnitConversion::Converter m_converter;

    QStringList m_sourcesToReset;
};
