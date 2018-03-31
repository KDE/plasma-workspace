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

/* Ion for BBC Weather from UKMET Office */

#ifndef ION_BBCUKMET_H
#define ION_BBCUKMET_H

#include "../ion.h"

#include <Plasma/DataEngineConsumer>

#include <QDateTime>
#include <QVector>

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
    WeatherData();

    QString place;
    QString stationName;
    double stationLatitude;
    double stationLongitude;

    // Current observation information.
    QString obsTime;
    QDateTime observationDateTime;

    QString condition;
    QString conditionIcon;
    float temperature_C;
    QString windDirection;
    float windSpeed_miles;
    float humidity;
    float pressure;
    QString pressureTendency;
    QString visibilityStr;

    QString solarDataTimeEngineSourceName;
    bool isNight = false;
    bool isSolarDataPending = false;

    // Five day forecast
    struct ForecastInfo {
        ForecastInfo();
        QString period;
        QString iconName;
        QString summary;
        float tempHigh;
        float tempLow;
        float windSpeed;
        QString windDirection;
    };

    // 5 day Forecast
    QVector <WeatherData::ForecastInfo *> forecasts;

    bool isForecastsDataPending = false;
};

Q_DECLARE_TYPEINFO(WeatherData::ForecastInfo, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(WeatherData, Q_MOVABLE_TYPE);


class Q_DECL_EXPORT UKMETIon : public IonInterface, public Plasma::DataEngineConsumer
{
    Q_OBJECT

public:
    UKMETIon(QObject *parent, const QVariantList &args);
    ~UKMETIon() override;

public: // IonInterface API
    bool updateIonSource(const QString& source) override;

public Q_SLOTS:
    // for solar data pushes from the time engine
    void dataUpdated(const QString& sourceName, const Plasma::DataEngine::Data& data);

protected: // IonInterface API
    void reset() override;

private Q_SLOTS:
    void setup_slotDataArrived(KIO::Job *, const QByteArray &);
    void setup_slotJobFinished(KJob *);
    //void setup_slotRedirected(KIO::Job *, const KUrl &url);

    void observation_slotDataArrived(KIO::Job *, const QByteArray &);
    void observation_slotJobFinished(KJob *);

    void forecast_slotDataArrived(KIO::Job *, const QByteArray &);
    void forecast_slotJobFinished(KJob *);

private:
    void updateWeather(const QString& source);

    //bool night(const QString& source) const;

    /* UKMET Methods - Internal for Ion */
    QMap<QString, ConditionIcons> setupDayIconMappings() const;
    QMap<QString, ConditionIcons> setupNightIconMappings() const;
    QMap<QString, IonInterface::WindDirections> setupWindIconMappings() const;

    QMap<QString, ConditionIcons> const& nightIcons() const;
    QMap<QString, ConditionIcons> const& dayIcons() const;
    QMap<QString, IonInterface::WindDirections> const& windIcons() const;

    // Load and Parse the place search XML listings
    void findPlace(const QString& place, const QString& source);
    void validate(const QString& source); // Sync data source with Applet
    void getFiveDayForecast(const QString& source);
    void getXMLData(const QString& source);
    void readSearchHTMLData(const QString& source, const QByteArray& html);
    bool readFiveDayForecastXMLData(const QString& source, QXmlStreamReader& xml);
    void parseSearchLocations(const QString& source, QXmlStreamReader& xml);

    // Observation parsing methods
    bool readObservationXMLData(const QString& source, QXmlStreamReader& xml);
    void parsePlaceObservation(const QString& source, WeatherData& data, QXmlStreamReader& xml);
    void parseWeatherChannel(const QString& source, WeatherData& data, QXmlStreamReader& xml);
    void parseWeatherObservation(const QString& source, WeatherData& data, QXmlStreamReader& xml);
    void parseFiveDayForecast(const QString& source, QXmlStreamReader& xml);
    void parsePlaceForecast(const QString& source, QXmlStreamReader& xml);
    void parseWeatherForecast(const QString& source, QXmlStreamReader& xml);
    void parseUnknownElement(QXmlStreamReader& xml) const;

    void parseFloat(float& value, const QString& string);

    void deleteForecasts();

private:
    struct XMLMapInfo {
        QString stationId;
        QString place;
        QString forecastHTMLUrl;
        QString sourceExtraArg;
    };

    // Key dicts
    QHash<QString, UKMETIon::XMLMapInfo> m_place;
    QVector<QString> m_locations;

    // Weather information
    QHash<QString, WeatherData> m_weatherData;

    // Store KIO jobs - Search list
    QHash<KJob *, QByteArray *> m_jobHtml;
    QHash<KJob *, QString> m_jobList;

    QHash<KJob *, QXmlStreamReader*> m_obsJobXml;
    QHash<KJob *, QString> m_obsJobList;

    QHash<KJob *, QXmlStreamReader *> m_forecastJobXml;
    QHash<KJob *, QString> m_forecastJobList;

    QStringList m_sourcesToReset;
};

#endif
