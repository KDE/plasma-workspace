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

#include "../ion.h"

#include <QVector>

// wetter.com API project data
#define PROJECTNAME "weatherion"
#define SEARCH_URL "https://api.wetter.com/location/index/search/%1/project/" PROJECTNAME "/cs/%2"
#define FORECAST_URL "https://api.wetter.com/forecast/weather/city/%1/project/" PROJECTNAME "/cs/%2"
#define APIKEY "07025b9a22b4febcf8e8ec3e6f1140e8"
#define MIN_POLL_INTERVAL 3600000L // 1 h

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
        int getMaxTemp(const QVector<WeatherData::ForecastInfo*>& forecastInfos) const;
        int getMinTemp(const QVector<WeatherData::ForecastInfo*>& forecastInfos) const;
    };

    QVector<WeatherData::ForecastPeriod *> forecasts;
};

Q_DECLARE_TYPEINFO(WeatherData::ForecastInfo, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(WeatherData::ForecastPeriod, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(WeatherData, Q_MOVABLE_TYPE);


class Q_DECL_EXPORT WetterComIon : public IonInterface
{
    Q_OBJECT

public:
    WetterComIon(QObject *parent, const QVariantList &args);
    ~WetterComIon() override;

public: // IonInterface API
    bool updateIonSource(const QString& source) override;

protected: // IonInterface API
    void reset() override;

private Q_SLOTS:
    void setup_slotDataArrived(KIO::Job *, const QByteArray &);
    void setup_slotJobFinished(KJob *);

    void forecast_slotDataArrived(KIO::Job *, const QByteArray &);
    void forecast_slotJobFinished(KJob *);

private:
    void cleanup();

    // Set up the mapping from the wetter.com condition code to the respective icon / condition name
    QMap<QString, ConditionIcons> setupCommonIconMappings() const;
    QMap<QString, ConditionIcons> setupDayIconMappings() const;
    QMap<QString, ConditionIcons> setupNightIconMappings() const;
    QHash<QString, QString> setupCommonConditionMappings() const;
    QHash<QString, QString> setupDayConditionMappings() const;
    QHash<QString, QString> setupNightConditionMappings() const;

    // Retrieve the mapping from the wetter.com condition code to the respective icon / condition name
    QMap<QString, ConditionIcons> const& nightIcons() const;
    QMap<QString, ConditionIcons> const& dayIcons() const;
    QHash<QString, QString> const& dayConditions() const;
    QHash<QString, QString> const& nightConditions() const;

    QString getWeatherCondition(const QHash<QString, QString> &conditionList, const QString& condition) const;

    // Find place
    void findPlace(const QString& place, const QString& source);
    void parseSearchResults(const QString& source, QXmlStreamReader& xml);
    void validate(const QString& source, bool parseError);

    // Retrieve and parse forecast
    void fetchForecast(const QString& source);
    void parseWeatherForecast(const QString& source, QXmlStreamReader& xml);
    void updateWeather(const QString& source, bool parseError);

private:
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
    QHash<KJob *, QXmlStreamReader *> m_searchJobXml;
    QHash<KJob *, QString> m_searchJobList;

    // Store KIO jobs - Forecast retrieval
    QHash<KJob *, QXmlStreamReader *> m_forecastJobXml;
    QHash<KJob *, QString> m_forecastJobList;

    QStringList m_sourcesToReset;
};


#endif
// kate: indent-mode cstyle; space-indent on; indent-width 4; 
