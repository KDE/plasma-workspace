/***************************************************************************
 *   Copyright (C) 2007-2009 by Shawn Starr <shawn.starr@rogers.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

/* Ion for Environment Canada XML data */

#ifndef ION_ENVCAN_H
#define ION_ENVCAN_H

#include <QtXml/QXmlStreamReader>
#include <QDateTime>

#include <kdemacros.h>
#include <Plasma/DataEngine>

#include "../ion.h"
#include "../dataengineconsumer.h"

class KJob;
namespace KIO
{
    class Job;
} // namespace KIO

class WeatherData
{

public:
    // WeatherEvent can have more than one, especially in Canada, eh? :)
    struct WeatherEvent {
        QString url;
        QString type;
        QString priority;
        QString description;
        QString timestamp;
    };

    // Five day forecast
    struct ForecastInfo {
        QString forecastPeriod;
        QString forecastSummary;
        QString iconName;
        QString shortForecast;

        QString forecastTempHigh;
        QString forecastTempLow;
        QString popPrecent;
        QString windForecast;

        QString precipForecast;
        QString precipType;
        QString precipTotalExpected;
        int forecastHumidity;
    };

    QString countryName;
    QString longTerritoryName;
    QString shortTerritoryName;
    QString cityName;
    QString regionName;
    QString stationID;
    QString stationLat;
    QString stationLon;

    // Current observation information.
    QString obsTimestamp;

    // Icon info to aproximate periods
    int iconPeriodHour;
    int iconPeriodMinute;

    QString condition;
    QString temperature;
    QString dewpoint;

    // In winter windchill, in summer, humidex
    QString comforttemp;

    float pressure;
    QString pressureTendency;

    float visibility;
    QString humidity;

    QString windSpeed;
    QString windGust;
    QString windDirection;
    QString windDegrees;

    QVector <WeatherData::WeatherEvent *> watches;
    QVector <WeatherData::WeatherEvent *> warnings;

    QString normalHigh;
    QString normalLow;

    QString forecastTimestamp;

    QString UVIndex;
    QString UVRating;

    // 5 day Forecast
    QVector <WeatherData::ForecastInfo *> forecasts;

    // Historical data from previous day.
    QString prevHigh;
    QString prevLow;
    QString prevPrecipType;
    QString prevPrecipTotal;

    // Almanac info
    QString sunriseTimestamp;
    QString sunsetTimestamp;
    QString moonriseTimestamp;
    QString moonsetTimestamp;

    // Historical Records
    float recordHigh;
    float recordLow;
    float recordRain;
    float recordSnow;
};

class Q_DECL_EXPORT EnvCanadaIon : public IonInterface, public Plasma::DataEngineConsumer
{
    Q_OBJECT

public:
    EnvCanadaIon(QObject *parent, const QVariantList &args);
    ~EnvCanadaIon();
    bool updateIonSource(const QString& source); // Sync data source with Applet
    void updateWeather(const QString& source);

public Q_SLOTS:
    virtual void reset();

protected:
    void init();  // Setup the city location, fetching the correct URL name.

protected Q_SLOTS:
    void setup_slotDataArrived(KIO::Job *, const QByteArray &);
    void setup_slotJobFinished(KJob *);

    void slotDataArrived(KIO::Job *, const QByteArray &);
    void slotJobFinished(KJob *);

private:
    /* Environment Canada Methods - Internal for Ion */
    void deleteForecasts();

    QMap<QString, ConditionIcons> setupConditionIconMappings(void) const;
    QMap<QString, ConditionIcons> setupForecastIconMappings(void) const;

    QMap<QString, ConditionIcons> const& conditionIcons(void) const;
    QMap<QString, ConditionIcons> const& forecastIcons(void) const;

    // Place information
    QString const country(const QString& source) const;
    QString territory(const QString& source) const;
    QString city(const QString& source) const;
    QString region(const QString& source) const;
    QString station(const QString& source) const;
    QString latitude(const QString& source) const;
    QString longitude(const QString& source) const;

    // Current Conditions Weather info
    QString observationTime(const QString& source) const;
    int periodHour(const QString& source) const;
    int periodMinute(const QString& source) const;
    QMap<QString, QString> watches(const QString& source) const;
    QMap<QString, QString> warnings(const QString& source) const;
    QString condition(const QString& source);
    QMap<QString, QString> temperature(const QString& source) const;
    QString dewpoint(const QString& source) const;
    QMap<QString, QString> humidity(const QString& source) const;
    QMap<QString, QString> visibility(const QString& source) const;
    QMap<QString, QString> pressure(const QString& source) const;
    QMap<QString, QString> wind(const QString& source) const;
    QMap<QString, QString> regionalTemperatures(const QString& source) const;
    QMap<QString, QString> uvIndex(const QString& source) const;
    QVector<QString> forecasts(const QString& source);
    QMap<QString, QString> yesterdayWeather(const QString& source) const;
    QMap<QString, QString> sunriseSet(const QString& source) const;
    QMap<QString, QString> moonriseSet(const QString& source) const;
    QMap<QString, QString> weatherRecords(const QString& source) const;

    // Load and Parse the place XML listing
    void getXMLSetup(void);
    bool readXMLSetup(void);

    // Load and parse the specific place(s)
    void getXMLData(const QString& source);
    bool readXMLData(const QString& source, QXmlStreamReader& xml);

    // Check if place specified is valid or not
    QStringList validate(const QString& source) const;

    // Catchall for unknown XML tags
    void parseUnknownElement(QXmlStreamReader& xml) const;

    // Parse weather XML data
    void parseWeatherSite(WeatherData& data, QXmlStreamReader& xml);
    void parseDateTime(WeatherData& data, QXmlStreamReader& xml, WeatherData::WeatherEvent* event = NULL);
    void parseLocations(WeatherData& data, QXmlStreamReader& xml);
    void parseConditions(WeatherData& data, QXmlStreamReader& xml);
    void parseWarnings(WeatherData& data, QXmlStreamReader& xml);
    void parseWindInfo(WeatherData& data, QXmlStreamReader& xml);
    void parseWeatherForecast(WeatherData& data, QXmlStreamReader& xml);
    void parseRegionalNormals(WeatherData& data, QXmlStreamReader& xml);
    void parseForecast(WeatherData& data, QXmlStreamReader& xml, WeatherData::ForecastInfo* forecast);
    void parseShortForecast(WeatherData::ForecastInfo* forecast, QXmlStreamReader& xml);
    void parseForecastTemperatures(WeatherData::ForecastInfo* forecast, QXmlStreamReader& xml);
    void parseWindForecast(WeatherData::ForecastInfo* forecast, QXmlStreamReader& xml);
    void parsePrecipitationForecast(WeatherData::ForecastInfo* forecast, QXmlStreamReader& xml);
    void parsePrecipTotals(WeatherData::ForecastInfo* forecast, QXmlStreamReader& xml);
    void parseUVIndex(WeatherData& data, QXmlStreamReader& xml);
    void parseYesterdayWeather(WeatherData& data, QXmlStreamReader& xml);
    void parseAstronomicals(WeatherData& data, QXmlStreamReader& xml);
    void parseWeatherRecords(WeatherData& data, QXmlStreamReader& xml);

    struct XMLMapInfo {
        QString cityName;
        QString territoryName;
        QString cityCode;
    };

    // Key dicts
    QHash<QString, EnvCanadaIon::XMLMapInfo> m_places;

    // Weather information
    QHash<QString, WeatherData> m_weatherData;

    // Store KIO jobs
    QHash<KJob *, QXmlStreamReader*> m_jobXml;
    QHash<KJob *, QString> m_jobList;
    QStringList m_sourcesToReset;
    QXmlStreamReader m_xmlSetup;
    Plasma::DataEngine *m_timeEngine;

    QDateTime m_dateFormat;
    bool emitWhenSetup;

};

K_EXPORT_PLASMA_DATAENGINE(envcan, EnvCanadaIon)

#endif
