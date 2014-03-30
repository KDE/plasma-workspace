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

/* Ion for NOAA's National Weather Service XML data */

#ifndef ION_NOAA_H
#define ION_NOAA_H

#include <QtXml/QXmlStreamReader>
#include <QStringList>
#include <QDateTime>

class KJob;
namespace KIO
{
    class Job;
} // namespace KIO
#include <kdemacros.h>

#include <Plasma/DataEngine>

#include "../dataengineconsumer.h"
#include "../ion.h"

class WeatherData
{

public:
    //QString countryName; // USA
    QString locationName;
    QString stationID;
    QString stationLat;
    QString stationLon;
    QString stateName;

    // Current observation information.
    QString observationTime;
    QString iconPeriodHour;
    QString iconPeriodAP;
    QString weather;

    QString temperature_F;
    QString temperature_C;
    QString humidity;
    QString windString;
    QString windDirection;
    QString windSpeed; // Float value
    QString windGust; // Float value
    QString pressure;
    QString dewpoint_F;
    QString dewpoint_C;
    QString heatindex_F;
    QString heatindex_C;
    QString windchill_F;
    QString windchill_C;
    QString visibility;

    struct Forecast
    {
        QString day;
        QString summary;
        QString low;
        QString high;
    };
    QList<Forecast> forecasts;
};

class Q_DECL_EXPORT NOAAIon : public IonInterface, public Plasma::DataEngineConsumer
{
    Q_OBJECT

public:
    NOAAIon(QObject *parent, const QVariantList &args);
    ~NOAAIon();
    void init(void);  // Setup the city location, fetching the correct URL name.
    bool updateIonSource(const QString& source); // Sync data source with Applet
    void updateWeather(const QString& source);

public Q_SLOTS:
    virtual void reset();

protected Q_SLOTS:
    void setup_slotDataArrived(KIO::Job *, const QByteArray &);
    void setup_slotJobFinished(KJob *);

    void slotDataArrived(KIO::Job *, const QByteArray &);
    void slotJobFinished(KJob *);

    void forecast_slotDataArrived(KIO::Job *, const QByteArray &);
    void forecast_slotJobFinished(KJob *);

private:
    /* NOAA Methods - Internal for Ion */
    QMap<QString, ConditionIcons> setupConditionIconMappings(void) const;
    QMap<QString, ConditionIcons> const & conditionIcons(void) const;
    QMap<QString, WindDirections> setupWindIconMappings(void) const;
    QMap<QString, WindDirections> const& windIcons(void) const;

    // Place information
    QString const country(const QString& source) const;
    QString place(const QString& source) const;
    QString station(const QString& source) const;
    QString latitude(const QString& source) const;
    QString longitude(const QString& source) const;

    // Current Conditions Weather info
    QString observationTime(const QString& source) const;
    //bool night(const QString& source);
    int periodHour(const QString& source) const;
    QString condition(const QString& source);
    QString conditionI18n(const QString& source);
    QMap<QString, QString> temperature(const QString& source) const;
    QString dewpoint(const QString& source) const;
    QMap<QString, QString> humidity(const QString& source) const;
    QMap<QString, QString> visibility(const QString& source) const;
    QMap<QString, QString> pressure(const QString& source) const;
    QMap<QString, QString> wind(const QString& source) const;
    IonInterface::ConditionIcons getConditionIcon(const QString& weather, bool isDayTime) const;

    // Load and Parse the place XML listing
    void getXMLSetup(void) const;
    bool readXMLSetup(void);

    // Load and parse the specific place(s)
    void getXMLData(const QString& source);
    bool readXMLData(const QString& source, QXmlStreamReader& xml);

    // Load and parse upcoming forecast for the next N days
    void getForecast(const QString& source);
    void readForecast(const QString& source, QXmlStreamReader& xml);

    // Check if place specified is valid or not
    QStringList validate(const QString& source) const;

    // Catchall for unknown XML tags
    void parseUnknownElement(QXmlStreamReader& xml) const;

    // Parse weather XML data
    void parseWeatherSite(WeatherData& data, QXmlStreamReader& xml);
    void parseStationID(void);
    void parseStationList(void);

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
    QMap<KJob *, QXmlStreamReader*> m_jobXml;
    QMap<KJob *, QString> m_jobList;
    QXmlStreamReader m_xmlSetup;

    Plasma::DataEngine *m_timeEngine;
    QDateTime m_dateFormat;
    bool emitWhenSetup;
    QStringList m_sourcesToReset;
};

K_EXPORT_PLASMA_DATAENGINE(noaa, NOAAIon)

#endif
