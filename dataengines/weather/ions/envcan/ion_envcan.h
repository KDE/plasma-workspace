/***************************************************************************
 *   Copyright (C) 2007-2009,2019 by Shawn Starr <shawn.starr@rogers.com>  *
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

#include "../ion.h"

#include <Plasma/DataEngineConsumer>

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
        ForecastInfo();

        QString forecastPeriod;
        QString forecastSummary;
        QString iconName;
        QString shortForecast;

        float tempHigh;
        float tempLow;
        float popPrecent;
        QString windForecast;

        QString precipForecast;
        QString precipType;
        QString precipTotalExpected;
        int forecastHumidity;
    };

    QString creditUrl;
    QString countryName;
    QString longTerritoryName;
    QString shortTerritoryName;
    QString cityName;
    QString regionName;
    QString stationID;
    double stationLatitude;
    double stationLongitude;

    // Current observation information.
    QString obsTimestamp;
    QDateTime observationDateTime;

    QString condition;
    float temperature;
    float dewpoint;

    // In winter windchill, in summer, humidex
    QString humidex;
    float windchill;

    float pressure;
    QString pressureTendency;

    float visibility;
    float humidity;

    float windSpeed;
    float windGust;
    QString windDirection;
    QString windDegrees;

    QVector<WeatherData::WeatherEvent *> watches;
    QVector<WeatherData::WeatherEvent *> warnings;

    float normalHigh;
    float normalLow;

    QString forecastTimestamp;

    QString UVIndex;
    QString UVRating;

    // 5 day Forecast
    QVector<WeatherData::ForecastInfo *> forecasts;

    // Historical data from previous day.
    float prevHigh;
    float prevLow;
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

    QString solarDataTimeEngineSourceName;
    bool isNight = false;
};

Q_DECLARE_TYPEINFO(WeatherData::WeatherEvent, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(WeatherData::ForecastInfo, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(WeatherData, Q_MOVABLE_TYPE);

/**
 * https://weather.gc.ca/mainmenu/disclaimer_e.html
 */
class Q_DECL_EXPORT EnvCanadaIon : public IonInterface, public Plasma::DataEngineConsumer
{
    Q_OBJECT

public:
    EnvCanadaIon(QObject *parent, const QVariantList &args);
    ~EnvCanadaIon() override;

public: // IonInterface API
    bool updateIonSource(const QString &source) override;

public Q_SLOTS:
    // for solar data pushes from the time engine
    void dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data);

protected: // IonInterface API
    void reset() override;

private Q_SLOTS:
    void setup_slotDataArrived(KIO::Job *, const QByteArray &);
    void setup_slotJobFinished(KJob *);

    void slotDataArrived(KIO::Job *, const QByteArray &);
    void slotJobFinished(KJob *);

private:
    void updateWeather(const QString &source);

    /* Environment Canada Methods - Internal for Ion */
    void deleteForecasts();

    QMap<QString, ConditionIcons> setupConditionIconMappings() const;
    QMap<QString, ConditionIcons> setupForecastIconMappings() const;

    QMap<QString, ConditionIcons> const &conditionIcons() const;
    QMap<QString, ConditionIcons> const &forecastIcons() const;

    // Load and Parse the place XML listing
    void getXMLSetup();
    bool readXMLSetup();

    // Load and parse the specific place(s)
    void getXMLData(const QString &source);
    bool readXMLData(const QString &source, QXmlStreamReader &xml);

    // Check if place specified is valid or not
    QStringList validate(const QString &source) const;

    // Catchall for unknown XML tags
    void parseUnknownElement(QXmlStreamReader &xml) const;

    // Parse weather XML data
    void parseWeatherSite(WeatherData &data, QXmlStreamReader &xml);
    void parseDateTime(WeatherData &data, QXmlStreamReader &xml, WeatherData::WeatherEvent *event = nullptr);
    void parseLocations(WeatherData &data, QXmlStreamReader &xml);
    void parseConditions(WeatherData &data, QXmlStreamReader &xml);
    void parseWarnings(WeatherData &data, QXmlStreamReader &xml);
    void parseWindInfo(WeatherData &data, QXmlStreamReader &xml);
    void parseWeatherForecast(WeatherData &data, QXmlStreamReader &xml);
    void parseRegionalNormals(WeatherData &data, QXmlStreamReader &xml);
    void parseForecast(WeatherData &data, QXmlStreamReader &xml, WeatherData::ForecastInfo *forecast);
    void parseShortForecast(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml);
    void parseForecastTemperatures(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml);
    void parseWindForecast(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml);
    void parsePrecipitationForecast(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml);
    void parsePrecipTotals(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml);
    void parseUVIndex(WeatherData &data, QXmlStreamReader &xml);
    void parseYesterdayWeather(WeatherData &data, QXmlStreamReader &xml);
    void parseAstronomicals(WeatherData &data, QXmlStreamReader &xml);
    void parseWeatherRecords(WeatherData &data, QXmlStreamReader &xml);

    void parseFloat(float &value, QXmlStreamReader &xml);

private:
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
    QHash<KJob *, QXmlStreamReader *> m_jobXml;
    QHash<KJob *, QString> m_jobList;
    QStringList m_sourcesToReset;
    QXmlStreamReader m_xmlSetup;

    bool emitWhenSetup;
};

#endif
