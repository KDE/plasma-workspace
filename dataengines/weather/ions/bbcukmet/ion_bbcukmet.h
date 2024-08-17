/*
    SPDX-FileCopyrightText: 2007-2009 Shawn Starr <shawn.starr@rogers.com>
    SPDX-FileCopyrightText: 2024 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Ion for BBC Weather from UKMET Office */

#pragma once

#include "../ion.h"

#include <Plasma5Support/DataEngineConsumer>

#include <QDateTime>
#include <QList>

class KJob;
namespace KIO
{
class Job;
class TransferJob;
}
class QXmlStreamReader;

class WeatherData
{
public:
    QString place;
    QString stationName;
    double stationLatitude = qQNaN();
    double stationLongitude = qQNaN();

    // Current observation information.
    struct Observation {
        QString obsTime;
        QDateTime observationDateTime;

        QString condition;
        QString conditionIcon;
        float temperature_C = qQNaN();
        QString windDirection;
        float windSpeed_miles = qQNaN();
        float humidity = qQNaN();
        float pressure = qQNaN();
        QString pressureTendency;
        QString visibilityStr;
    };
    Observation current;
    bool isObservationDataPending = false;

    QString solarDataTimeEngineSourceName;
    bool isNight = false;
    bool isSolarDataPending = false;

    // Forecasts
    struct ForecastInfo {
        QString period;
        QString iconName;
        QString summary;
        float tempHigh = qQNaN();
        float tempLow = qQNaN();
        float windSpeed = qQNaN();
        QString windDirection;
    };

    QList<WeatherData::ForecastInfo> forecasts;
    bool isForecastsDataPending = false;
};

Q_DECLARE_TYPEINFO(WeatherData::ForecastInfo, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(WeatherData, Q_RELOCATABLE_TYPE);

class Q_DECL_EXPORT UKMETIon : public IonInterface, public Plasma5Support::DataEngineConsumer
{
    Q_OBJECT

public:
    UKMETIon(QObject *parent);

public: // IonInterface API
    bool updateIonSource(const QString &source) override;

public Q_SLOTS:
    // for solar data pushes from the time engine
    void dataUpdated(const QString &sourceName, const Plasma5Support::DataEngine::Data &data);

protected: // IonInterface API
    void reset() override;

private Q_SLOTS:
    void search_slotJobFinished(KJob *);
    void observation_slotJobFinished(KJob *);
    void forecast_slotJobFinished(KJob *);

private:
    void updateWeather(const QString &source);

    /* UKMET Methods - Internal for Ion */
    QMap<QString, ConditionIcons> setupDayIconMappings() const;
    QMap<QString, ConditionIcons> setupNightIconMappings() const;
    QMap<QString, IonInterface::WindDirections> setupWindIconMappings() const;

    QMap<QString, ConditionIcons> const &nightIcons() const;
    QMap<QString, ConditionIcons> const &dayIcons() const;
    QMap<QString, IonInterface::WindDirections> const &windIcons() const;

    KJob *requestAPIJob(const QString &source, const QUrl &url);

    // Load and Parse the place search listings
    void findPlace(const QString &place, const QString &source);
    void readSearchData(const QString &source, const QByteArray &json);
    void validate(const QString &source); // Sync data source with Applet

    // Load and parse the weather forecast
    void getForecast(const QString &source);
    bool readForecast(const QString &source, QXmlStreamReader &xml);
    void parseForecast(const QString &source, QXmlStreamReader &xml);
    void parseWeatherForecast(const QString &source, QXmlStreamReader &xml);
    void parsePlaceForecast(const QString &source, QXmlStreamReader &xml);

    // Observation parsing methods
    void getObservation(const QString &source);
    void getSolarData(const QString &source);
    bool readObservationData(const QString &source, QXmlStreamReader &xml);
    void parsePlaceObservation(const QString &source, WeatherData &data, QXmlStreamReader &xml);
    void parseWeatherChannel(const QString &source, WeatherData &data, QXmlStreamReader &xml);
    void parseWeatherObservation(const QString &source, WeatherData &data, QXmlStreamReader &xml);

    void parseUnknownElement(QXmlStreamReader &xml) const;
    void parseFloat(float &value, const QString &string);

private:
    struct XMLMapInfo {
        QString stationId;
        QString place;
        QString forecastHTMLUrl;
    };

    // Key dicts
    QHash<QString, UKMETIon::XMLMapInfo> m_place;
    QList<QString> m_locations;

    // Weather information
    QHash<QString, WeatherData> m_weatherData;

    // Store KIO jobs - Search list
    QHash<KJob *, std::shared_ptr<QByteArray>> m_jobData;
    QHash<KJob *, QString> m_jobList;

    int m_pendingSearchCount = 0;

    QStringList m_sourcesToReset;
};
