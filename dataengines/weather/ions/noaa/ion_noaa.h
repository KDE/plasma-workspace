/*
    SPDX-FileCopyrightText: 2007-2009, 2019 Shawn Starr <shawn.starr@rogers.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Ion for NOAA's National Weather Service XML data */

#pragma once

#include "../ion.h"

#include <Plasma5Support/DataEngineConsumer>

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
    WeatherData();

    QString locationName;
    QString stationID;
    double stationLatitude;
    double stationLongitude;
    QString stateName;
    QString countyID;

    // Current observation information.
    QString observationTime;
    QDateTime observationDateTime;
    QString weather;

    float temperature_F;
    float temperature_C;
    float humidity;
    QString windString;
    QString windDirection;
    float windSpeed;
    float windGust;
    float pressure;
    float dewpoint_F;
    float dewpoint_C;
    float heatindex_F;
    float heatindex_C;
    float windchill_F;
    float windchill_C;
    float visibility;

    struct Forecast {
        QString day;
        QString summary;
        QString low;
        QString high;
        int precipitation = 0;
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

protected: // IonInterface API
    void reset() override;

private Q_SLOTS:
    void setup_slotJobFinished(KJob *);
    void slotJobFinished(KJob *);
    void forecast_slotJobFinished(KJob *);
    void county_slotJobFinished(KJob *);
    void alerts_slotJobFinished(KJob *);

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
    KJob *apiRequestJob(const QUrl &url, const QString &source);

    // Load and Parse the place XML listing
    void getXMLSetup(bool reset = true);
    bool readXMLSetup(QXmlStreamReader &xml);

    // Load and parse the specific place(s)
    void getXMLData(const QString &source);
    bool readXMLData(const QString &source, QXmlStreamReader &xml);

    // Load and parse upcoming forecast for the next N days
    void getForecast(const QString &source);
    void readForecast(const QString &source, QXmlStreamReader &xml);

    // Methods to get alerts. We need the county ID first
    void getCountyID(const QString &source);
    void readCountyID(const QString &source, const QJsonDocument &doc);
    void getAlerts(const QString &source);
    void readAlerts(const QString &source, const QJsonDocument &doc);

    // Check if place specified is valid or not
    QStringList validate(const QString &source) const;

    // Catchall for unknown XML tags
    void parseUnknownElement(QXmlStreamReader &xml) const;

    // Parse weather XML data
    void parseWeatherSite(WeatherData &data, QXmlStreamReader &xml);
    void parseStationID(QXmlStreamReader &xml);
    void parseStationList(QXmlStreamReader &xml);

    void parseFloat(float &value, const QString &string);
    void parseFloat(float &value, QXmlStreamReader &xml);
    void parseDouble(double &value, QXmlStreamReader &xml);

private:
    struct XMLMapInfo {
        QString stateName;
        QString stationName;
        QString stationID;
        QString XMLurl;
    };

    // Key dicts
    QHash<QString, NOAAIon::XMLMapInfo> m_places;

    // Weather information
    QHash<QString, WeatherData> m_weatherData;

    // Store KIO jobs
    QHash<KJob *, QByteArray> m_jobData;
    QHash<KJob *, QString> m_jobList;

    // bool emitWhenSetup;
    QStringList m_sourcesToReset;
};
