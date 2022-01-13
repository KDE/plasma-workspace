/*
    SPDX-FileCopyrightText: 2021 Emily Ehlert

    Based upon BBC Weather Ion and ENV Canada Ion by Shawn Starr
    SPDX-FileCopyrightText: 2007-2009 Shawn Starr <shawn.starr@rogers.com>

    also

    the wetter.com Ion by Thilo-Alexander Ginkel
    SPDX-FileCopyrightText: 2009 Thilo-Alexander Ginkel <thilo@ginkel.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Ion for weather data from Deutscher Wetterdienst (DWD) / German Weather Service */

#pragma once

#include "../ion.h"

#include <QVector>

#define CATALOGUE_URL "https://www.dwd.de/DE/leistungen/met_verfahren_mosmix/mosmix_stationskatalog.cfg?view=nasPublication&nn=16102"
#define FORECAST_URL "https://app-prod-ws.warnwetter.de/v30/stationOverviewExtended?stationIds=%1"
#define MEASURE_URL "https://s3.eu-central-1.amazonaws.com/app-prod-static.warnwetter.de/v16/current_measurement_%1.json"

class KJob;
namespace KIO
{
class Job;
class TransferJob;
}

class WeatherData
{
public:
    WeatherData();

    QString place;

    // Current observation information.
    QDateTime observationDateTime;

    QString conditionIcon;
    QString windDirection;
    float temperature;
    float humidity;
    float pressure;
    float windSpeed;
    float gustSpeed;
    float dewpoint;

    // If current observations not available, use forecast data for current day
    QString windDirectionAlt;
    float windSpeedAlt;
    float gustSpeedAlt;

    // 7 forecast
    struct ForecastInfo {
        ForecastInfo();
        QDateTime period;
        QString iconName;
        QString summary;
        float tempHigh;
        float tempLow;
        float windSpeed;
        QString windDirection;
    };

    // 7 day forecast
    QVector<WeatherData::ForecastInfo *> forecasts;

    struct WarningInfo {
        QString type;
        int priority;
        QString description;
        QDateTime timestamp;
    };

    QVector<WeatherData::WarningInfo *> warnings;

    bool isForecastsDataPending = false;
    bool isMeasureDataPending = false;
};

class Q_DECL_EXPORT DWDIon : public IonInterface
{
    Q_OBJECT

public:
    DWDIon(QObject *parent, const QVariantList &args);
    ~DWDIon() override;

public: // IonInterface API
    bool updateIonSource(const QString &source) override;

protected: // IonInterface API
    void reset() override;

private Q_SLOTS:
    void setup_slotDataArrived(KIO::Job *, const QByteArray &);
    void setup_slotJobFinished(KJob *);

    void measure_slotDataArrived(KIO::Job *, const QByteArray &);
    void measure_slotJobFinished(KJob *);

    void forecast_slotDataArrived(KIO::Job *, const QByteArray &);
    void forecast_slotJobFinished(KJob *);

private:
    QMap<QString, ConditionIcons> setupDayIconMappings() const;
    QMap<QString, WindDirections> setupWindIconMappings() const;

    QMap<QString, ConditionIcons> const &dayIcons() const;
    QMap<QString, WindDirections> const &windIcons() const;

    void findPlace(const QString &searchText);
    void parseStationData(QByteArray data);
    void searchInStationList(const QString place);

    void validate(const QString &source);

    void fetchWeather(QString placeName, QString placeID);
    void parseForecastData(const QString source, QJsonDocument doc);
    void parseMeasureData(const QString source, QJsonDocument doc);
    void updateWeather(const QString &source);

    void deleteForecasts();

    // Helper methods
    void calculatePositions(QStringList lines, QVector<int> &namePositionalInfo, QVector<int> &stationIdPositionalInfo);
    QString camelCaseString(const QString text);
    QString extractString(QByteArray array, int start, int length);
    QString roundWindDirections(int windDirection);
    float parseNumber(int number);

private:
    // Key dicts
    QMap<QString, QString> m_place;
    QList<QString> m_locations;

    // Weather information
    QHash<QString, WeatherData> m_weatherData;

    QHash<KJob *, QByteArray> m_searchJobData;
    QHash<KJob *, QString> m_searchJobList;

    QHash<KJob *, QByteArray> m_forecastJobJSON;
    QHash<KJob *, QString> m_forecastJobList;

    QHash<KJob *, QByteArray> m_measureJobJSON;
    QHash<KJob *, QString> m_measureJobList;

    QStringList m_sourcesToReset;
};
// kate: indent-mode cstyle; space-indent on; indent-width 4;
