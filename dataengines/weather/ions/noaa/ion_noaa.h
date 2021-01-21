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

/* Ion for NOAA's National Weather Service XML data */

#ifndef ION_NOAA_H
#define ION_NOAA_H

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

    QString locationName;
    QString stationID;
    double stationLatitude;
    double stationLongitude;
    QString stateName;

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
    };
    QVector<Forecast> forecasts;

    bool isForecastsDataPending = false;

    QString solarDataTimeEngineSourceName;
    bool isNight = false;
    bool isSolarDataPending = false;
};

Q_DECLARE_TYPEINFO(WeatherData::Forecast, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(WeatherData, Q_MOVABLE_TYPE);

class Q_DECL_EXPORT NOAAIon : public IonInterface, public Plasma::DataEngineConsumer
{
    Q_OBJECT

public:
    NOAAIon(QObject *parent, const QVariantList &args);
    ~NOAAIon() override;

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

    void forecast_slotDataArrived(KIO::Job *, const QByteArray &);
    void forecast_slotJobFinished(KJob *);

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

    // Load and Parse the place XML listing
    void getXMLSetup() const;
    bool readXMLSetup();

    // Load and parse the specific place(s)
    void getXMLData(const QString &source);
    bool readXMLData(const QString &source, QXmlStreamReader &xml);

    // Load and parse upcoming forecast for the next N days
    void getForecast(const QString &source);
    void readForecast(const QString &source, QXmlStreamReader &xml);

    // Check if place specified is valid or not
    QStringList validate(const QString &source) const;

    // Catchall for unknown XML tags
    void parseUnknownElement(QXmlStreamReader &xml) const;

    // Parse weather XML data
    void parseWeatherSite(WeatherData &data, QXmlStreamReader &xml);
    void parseStationID();
    void parseStationList();

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
    QHash<KJob *, QXmlStreamReader *> m_jobXml;
    QHash<KJob *, QString> m_jobList;
    QXmlStreamReader m_xmlSetup;

    // bool emitWhenSetup;
    QStringList m_sourcesToReset;
};

#endif
