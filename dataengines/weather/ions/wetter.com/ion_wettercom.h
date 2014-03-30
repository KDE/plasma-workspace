/***************************************************************************
 *   Copyright (C) 2009 by Thilo-Alexander Ginkel <thilo@ginkel.com>       *
 *                                                                         *
 *   Based upon BBC Weather Ion by Shawn Starr                             *
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

/* Ion for weather data from wetter.com */

#ifndef ION_WETTERCOM_H
#define ION_WETTERCOM_H

#include <QtXml/QXmlStreamReader>
#include <QtCore/QStringList>
#include <QCryptographicHash>
#include <algorithm>
#include <kurl.h>
#include <kio/job.h>
#include <kio/scheduler.h>
#include <kdemacros.h>
#include <plasma/dataengine.h>
#include "../ion.h"

// wetter.com API project data
#define PROJECTNAME "weatherion"
#define SEARCH_URL "http://api.wetter.com/location/index/search/%1/project/" PROJECTNAME "/cs/%2"
#define FORECAST_URL "http://api.wetter.com/forecast/weather/city/%1/project/" PROJECTNAME "/cs/%2"
#define APIKEY "07025b9a22b4febcf8e8ec3e6f1140e8"
#define MIN_POLL_INTERVAL 3600000L // 1 h

class WeatherData
{
public:
    QString place;
    QString stationName;

    // time difference to UTC
    int timeDifference;

    // credits as returned from API request
    QString credits;
    QString creditsUrl;

    class ForecastBase
    {
    public:
        QDateTime period;
        QString iconName;
        QString summary;
        int probability;
    };

    class ForecastInfo : public ForecastBase
    {
    public:
        int tempHigh;
        int tempLow;
    };

    class ForecastPeriod : public ForecastInfo
    {
    public:
        ~ForecastPeriod();

        WeatherData::ForecastInfo getDayWeather() const;
        WeatherData::ForecastInfo getNightWeather() const;
        WeatherData::ForecastInfo getWeather() const;

        bool hasNightWeather() const;

        QVector<WeatherData::ForecastInfo *> dayForecasts;
        QVector<WeatherData::ForecastInfo *> nightForecasts;
    private:
        int getMaxTemp(QVector<WeatherData::ForecastInfo *> forecastInfos) const;
        int getMinTemp(QVector<WeatherData::ForecastInfo *> forecastInfos) const;
    };

    QVector<WeatherData::ForecastPeriod *> forecasts;
};

class Q_DECL_EXPORT WetterComIon : public IonInterface
{
    Q_OBJECT

public:
    WetterComIon(QObject *parent, const QVariantList &args);
    ~WetterComIon();

    bool updateIonSource(const QString& source);

public Q_SLOTS:
    virtual void reset();

protected Q_SLOTS:
    void setup_slotDataArrived(KIO::Job *, const QByteArray &);
    void setup_slotJobFinished(KJob *);
    void forecast_slotDataArrived(KIO::Job *, const QByteArray &);
    void forecast_slotJobFinished(KJob *);

private:
    void init();
    void cleanup();

    // Set up the mapping from the wetter.com condition code to the respective icon / condition name
    QMap<QString, ConditionIcons> setupCommonIconMappings(void) const;
    QMap<QString, ConditionIcons> setupDayIconMappings(void) const;
    QMap<QString, ConditionIcons> setupNightIconMappings(void) const;
    QMap<QString, QString> setupCommonConditionMappings(void) const;
    QMap<QString, QString> setupDayConditionMappings(void) const;
    QMap<QString, QString> setupNightConditionMappings(void) const;

    // Retrieve the mapping from the wetter.com condition code to the respective icon / condition name
    QMap<QString, ConditionIcons> const& nightIcons(void) const;
    QMap<QString, ConditionIcons> const& dayIcons(void) const;
    QMap<QString, QString> const& dayConditions(void) const;
    QMap<QString, QString> const& nightConditions(void) const;

    QString getWeatherCondition(const QMap<QString, QString> &conditionList, const QString& condition) const;

    // Find place
    void findPlace(const QString& place, const QString& source);
    void parseSearchResults(const QString& source, QXmlStreamReader& xml);
    void validate(const QString& source, bool parseError);

    // Retrieve and parse forecast
    void fetchForecast(const QString& source);
    void parseWeatherForecast(const QString& source, QXmlStreamReader& xml);
    void updateWeather(const QString& source, bool parseError);

    struct PlaceInfo {
        QString name;
        QString displayName;
        QString placeCode;
    };

    // Key dicts
    QHash<QString, WetterComIon::PlaceInfo> m_place;
    QVector<QString> m_locations;

    // Weather information
    QHash<QString, WeatherData> m_weatherData;

    // Store KIO jobs - Search list
    QMap<KJob *, QXmlStreamReader *> m_searchJobXml;
    QMap<KJob *, QString> m_searchJobList;

    // Store KIO jobs - Forecast retrieval
    QMap<KJob *, QXmlStreamReader *> m_forecastJobXml;
    QMap<KJob *, QString> m_forecastJobList;

    KIO::TransferJob *m_job;
    QStringList m_sourcesToReset;
};

K_EXPORT_PLASMA_DATAENGINE(wettercom, WetterComIon)

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 4; 
