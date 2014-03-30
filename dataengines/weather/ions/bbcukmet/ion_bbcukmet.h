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

#include <QtXml/QXmlStreamReader>
#include <QDateTime>

#include "../ion.h"
#include "../dataengineconsumer.h"

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
    QString stationName;
    // Current observation information.
    QString obsTime;
    int iconPeriodHour;
    int iconPeriodMinute;
    double longitude;
    double latitude;

    QString condition;
    QString conditionIcon;
    QString temperature_C;
    QString windDirection;
    QString windSpeed_miles;
    QString humidity;
    QString pressure;
    QString pressureTendency;
    QString visibilityStr;

    // Five day forecast
    struct ForecastInfo {
        QString period;
        QString iconName;
        QString summary;
        int tempHigh;
        int tempLow;
        int windSpeed;
        QString windDirection;
    };

    // 5 day Forecast
    QVector <WeatherData::ForecastInfo *> forecasts;
};

class Q_DECL_EXPORT UKMETIon : public IonInterface, public Plasma::DataEngineConsumer
{
    Q_OBJECT

public:
    UKMETIon(QObject *parent, const QVariantList &args);
    ~UKMETIon();
    void init();  // Setup the city location, fetching the correct URL name.
    bool updateIonSource(const QString& source);
    void updateWeather(const QString& source);

    QString place(const QString& source) const;
    QString station(const QString& source) const;
    QString observationTime(const QString& source) const;
    //bool night(const QString& source) const;
    int periodHour(const QString& source) const;
    int periodMinute(const QString& source) const;
    double periodLatitude(const QString& source) const;
    double periodLongitude(const QString& source) const;
    QString condition(const QString& source) const;
    QMap<QString, QString> temperature(const QString& source) const;
    QMap<QString, QString> wind(const QString& source) const;
    QMap<QString, QString> humidity(const QString& source) const;
    QString visibility(const QString& source) const;
    QMap<QString, QString> pressure(const QString& source) const;
    QVector<QString> forecasts(const QString& source);

public Q_SLOTS:
    virtual void reset();

protected Q_SLOTS:
    void setup_slotDataArrived(KIO::Job *, const QByteArray &);
    void setup_slotJobFinished(KJob *);
    //void setup_slotRedirected(KIO::Job *, const KUrl &url);
    void observation_slotDataArrived(KIO::Job *, const QByteArray &);
    void observation_slotJobFinished(KJob *);
    void forecast_slotDataArrived(KIO::Job *, const QByteArray &);
    void forecast_slotJobFinished(KJob *);

private:
    /* UKMET Methods - Internal for Ion */

    QMap<QString, ConditionIcons> setupDayIconMappings(void) const;
    QMap<QString, ConditionIcons> setupNightIconMappings(void) const;

    QMap<QString, ConditionIcons> const& nightIcons(void) const;
    QMap<QString, ConditionIcons> const& dayIcons(void) const;

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

    void deleteForecasts();

    struct XMLMapInfo {
        QString place;
        QString XMLurl;
        QString forecastHTMLUrl;
        QString XMLforecastURL;
    };

    // Key dicts
    QHash<QString, UKMETIon::XMLMapInfo> m_place;
    QVector<QString> m_locations;

    // Weather information
    QHash<QString, WeatherData> m_weatherData;

    // Store KIO jobs - Search list
    QMap<KJob *, QByteArray *> m_jobHtml;
    QMap<KJob *, QString> m_jobList;

    QMap<KJob *, QXmlStreamReader*> m_obsJobXml;
    QMap<KJob *, QString> m_obsJobList;

    QMap<KJob *, QXmlStreamReader *> m_forecastJobXml;
    QMap<KJob *, QString> m_forecastJobList;

    KIO::TransferJob *m_job;
    Plasma::DataEngine *m_timeEngine;

    QDateTime m_dateFormat;
    QStringList m_sourcesToReset;
};

K_EXPORT_PLASMA_DATAENGINE(bbcukmet, UKMETIon)

#endif
