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

// Sample URLs:
// https://api.wetter.com/location/index/search/Heidelberg/project/weatherion/cs/9090dec6e783b96bd6a6ca9d451f3fee
// https://api.wetter.com/forecast/weather/city/DE0004329/project/weatherion/cs/89f1264869cce5c6fd5a2db80051f3d8

#include "ion_wettercom.h"

#include "ion_wettercomdebug.h"

#include <KIO/Job>
#include <KUnitConversion/Converter>
#include <KLocalizedString>

#include <QCryptographicHash>
#include <QXmlStreamReader>
#include <QLocale>

/*
 * Initialization
 */

WetterComIon::WetterComIon(QObject *parent, const QVariantList &args)
        : IonInterface(parent, args)

{
#if defined(MIN_POLL_INTERVAL)
    setMinimumPollingInterval(MIN_POLL_INTERVAL);
#endif
    setInitialized(true);
}

WetterComIon::~WetterComIon()
{
    cleanup();
}

void WetterComIon::cleanup()
{
    // Clean up dynamically allocated forecasts
    QMutableHashIterator<QString, WeatherData> it(m_weatherData);
    while (it.hasNext()) {
        it.next();
        WeatherData &item = it.value();
        qDeleteAll(item.forecasts);
        item.forecasts.clear();
    }
}

void WetterComIon::reset()
{
    cleanup();
    m_sourcesToReset = sources();
    updateAllSources();
}

QMap<QString, IonInterface::ConditionIcons> WetterComIon::setupCommonIconMappings() const
{
    return QMap<QString, ConditionIcons> {
        { QStringLiteral("3"),  Overcast },
        { QStringLiteral("30"), Overcast },
        { QStringLiteral("4"),  Haze },
        { QStringLiteral("40"), Haze },
        { QStringLiteral("45"), Haze },
        { QStringLiteral("48"), Haze },
        { QStringLiteral("49"), Haze },
        { QStringLiteral("5"),  Mist },
        { QStringLiteral("50"), Mist },
        { QStringLiteral("51"), Mist },
        { QStringLiteral("53"), Mist },
        { QStringLiteral("55"), Mist },
        { QStringLiteral("56"), FreezingDrizzle },
        { QStringLiteral("57"), FreezingDrizzle },
        { QStringLiteral("6"),  Rain },
        { QStringLiteral("60"), LightRain },
        { QStringLiteral("61"), LightRain },
        { QStringLiteral("63"), Rain },
        { QStringLiteral("65"), Rain },
        { QStringLiteral("66"), FreezingRain },
        { QStringLiteral("67"), FreezingRain },
        { QStringLiteral("68"), RainSnow },
        { QStringLiteral("69"), RainSnow },
        { QStringLiteral("7"),  Snow },
        { QStringLiteral("70"), LightSnow },
        { QStringLiteral("71"), LightSnow },
        { QStringLiteral("73"), Snow },
        { QStringLiteral("75"), Flurries },
        { QStringLiteral("8"),  Showers },
        { QStringLiteral("81"), Showers },
        { QStringLiteral("82"), Showers },
        { QStringLiteral("83"), RainSnow },
        { QStringLiteral("84"), RainSnow },
        { QStringLiteral("85"), Snow },
        { QStringLiteral("86"), Snow },
        { QStringLiteral("9"),  Thunderstorm },
        { QStringLiteral("90"), Thunderstorm },
        { QStringLiteral("96"), Thunderstorm },
        { QStringLiteral("999"), NotAvailable },
    };
}

QMap<QString, IonInterface::ConditionIcons> WetterComIon::setupDayIconMappings() const
{
    QMap<QString, ConditionIcons> conditionList = setupCommonIconMappings();

    conditionList.insert(QStringLiteral("0"),  ClearDay);
    conditionList.insert(QStringLiteral("1"),  FewCloudsDay);
    conditionList.insert(QStringLiteral("10"), FewCloudsDay);
    conditionList.insert(QStringLiteral("2"),  PartlyCloudyDay);
    conditionList.insert(QStringLiteral("20"), PartlyCloudyDay);
    conditionList.insert(QStringLiteral("80"), ChanceShowersDay);
    conditionList.insert(QStringLiteral("95"), ChanceThunderstormDay);

    return conditionList;
}

QMap<QString, IonInterface::ConditionIcons> const& WetterComIon::dayIcons() const
{
    static QMap<QString, ConditionIcons> const val = setupDayIconMappings();
    return val;
}

QMap<QString, IonInterface::ConditionIcons> WetterComIon::setupNightIconMappings() const
{
    QMap<QString, ConditionIcons> conditionList = setupCommonIconMappings();

    conditionList.insert(QStringLiteral("0"),  ClearNight);
    conditionList.insert(QStringLiteral("1"),  FewCloudsNight);
    conditionList.insert(QStringLiteral("10"), FewCloudsNight);
    conditionList.insert(QStringLiteral("2"),  PartlyCloudyNight);
    conditionList.insert(QStringLiteral("20"), PartlyCloudyNight);
    conditionList.insert(QStringLiteral("80"), ChanceShowersNight);
    conditionList.insert(QStringLiteral("95"), ChanceThunderstormNight);

    return conditionList;
}

QMap<QString, IonInterface::ConditionIcons> const& WetterComIon::nightIcons() const
{
    static QMap<QString, ConditionIcons> const val = setupNightIconMappings();
    return val;
}

QHash<QString, QString> WetterComIon::setupCommonConditionMappings() const
{
    return QHash<QString, QString> {
        { QStringLiteral("1"),   i18nc("weather condition", "few clouds") },
        { QStringLiteral("10"),  i18nc("weather condition", "few clouds") },
        { QStringLiteral("2"),   i18nc("weather condition", "cloudy") },
        { QStringLiteral("20"),  i18nc("weather condition", "cloudy") },
        { QStringLiteral("3"),   i18nc("weather condition", "overcast") },
        { QStringLiteral("30"),  i18nc("weather condition", "overcast") },
        { QStringLiteral("4"),   i18nc("weather condition", "haze") },
        { QStringLiteral("40"),  i18nc("weather condition", "haze") },
        { QStringLiteral("45"),  i18nc("weather condition", "haze") },
        { QStringLiteral("48"),  i18nc("weather condition", "fog with icing") },
        { QStringLiteral("49"),  i18nc("weather condition", "fog with icing") },
        { QStringLiteral("5"),   i18nc("weather condition", "drizzle") },
        { QStringLiteral("50"),  i18nc("weather condition", "drizzle") },
        { QStringLiteral("51"),  i18nc("weather condition", "light drizzle") },
        { QStringLiteral("53"),  i18nc("weather condition", "drizzle") },
        { QStringLiteral("55"),  i18nc("weather condition", "heavy drizzle") },
        { QStringLiteral("56"),  i18nc("weather condition", "freezing drizzle") },
        { QStringLiteral("57"),  i18nc("weather condition", "heavy freezing drizzle") },
        { QStringLiteral("6"),   i18nc("weather condition", "rain") },
        { QStringLiteral("60"),  i18nc("weather condition", "light rain") },
        { QStringLiteral("61"),  i18nc("weather condition", "light rain") },
        { QStringLiteral("63"),  i18nc("weather condition", "moderate rain") },
        { QStringLiteral("65"),  i18nc("weather condition", "heavy rain") },
        { QStringLiteral("66"),  i18nc("weather condition", "light freezing rain") },
        { QStringLiteral("67"),  i18nc("weather condition", "freezing rain") },
        { QStringLiteral("68"),  i18nc("weather condition", "light rain snow") },
        { QStringLiteral("69"),  i18nc("weather condition", "heavy rain snow") },
        { QStringLiteral("7"),   i18nc("weather condition", "snow") },
        { QStringLiteral("70"),  i18nc("weather condition", "light snow") },
        { QStringLiteral("71"),  i18nc("weather condition", "light snow") },
        { QStringLiteral("73"),  i18nc("weather condition", "moderate snow") },
        { QStringLiteral("75"),  i18nc("weather condition", "heavy snow") },
        { QStringLiteral("8"),   i18nc("weather condition", "showers") },
        { QStringLiteral("80"),  i18nc("weather condition", "light showers") },
        { QStringLiteral("81"),  i18nc("weather condition", "showers") },
        { QStringLiteral("82"),  i18nc("weather condition", "heavy showers") },
        { QStringLiteral("83"),  i18nc("weather condition", "light snow rain showers") },
        { QStringLiteral("84"),  i18nc("weather condition", "heavy snow rain showers") },
        { QStringLiteral("85"),  i18nc("weather condition", "light snow showers") },
        { QStringLiteral("86"),  i18nc("weather condition", "snow showers") },
        { QStringLiteral("9"),   i18nc("weather condition", "thunderstorm") },
        { QStringLiteral("90"),  i18nc("weather condition", "thunderstorm") },
        { QStringLiteral("95"),  i18nc("weather condition", "light thunderstorm") },
        { QStringLiteral("96"),  i18nc("weather condition", "heavy thunderstorm") },
        { QStringLiteral("999"), i18nc("weather condition", "n/a") },
    };
}

QHash<QString, QString> WetterComIon::setupDayConditionMappings() const
{
    QHash<QString, QString> conditionList = setupCommonConditionMappings();
    conditionList.insert(QStringLiteral("0"), i18nc("weather condition", "sunny"));
    return conditionList;
}

QHash<QString, QString> const& WetterComIon::dayConditions() const
{
    static QHash<QString, QString> const val = setupDayConditionMappings();
    return val;
}

QHash<QString, QString> WetterComIon::setupNightConditionMappings() const
{
    QHash<QString, QString> conditionList = setupCommonConditionMappings();
    conditionList.insert(QStringLiteral("0"), i18nc("weather condition", "clear sky"));
    return conditionList;
}

QHash<QString, QString> const& WetterComIon::nightConditions() const
{
    static QHash<QString, QString> const val = setupNightConditionMappings();
    return val;
}

QString WetterComIon::getWeatherCondition(const QHash<QString, QString> &conditionList, const QString& condition) const
{
    return conditionList[condition];
}

bool WetterComIon::updateIonSource(const QString& source)
{
    // We expect the applet to send the source in the following tokenization:
    // ionname|validate|place_name|extra - Triggers validation of place
    // ionname|weather|place_name|extra - Triggers receiving weather of place

    const QStringList sourceAction = source.split(QLatin1Char('|'));

    if (sourceAction.size() < 3) {
        setData(source, QStringLiteral("validate"), QStringLiteral("wettercom|malformed"));
        return true;
    }

    if (sourceAction[1] == QLatin1String("validate") && sourceAction.size() >= 3) {
        // Look for places to match
        findPlace(sourceAction[2], source);
        return true;
    }

    if (sourceAction[1] == QLatin1String("weather") && sourceAction.size() >= 3) {
        if (sourceAction.count() >= 4) {
            if (sourceAction[2].isEmpty()) {
                setData(source, QStringLiteral("validate"), QStringLiteral("wettercom|malformed"));
                return true;
            }

            // Extra data format: placeCode;displayName
            const QStringList extraData = sourceAction[3].split(QLatin1Char(';'));

            if (extraData.count() != 2) {
                setData(source, QStringLiteral("validate"), QStringLiteral("wettercom|malformed"));
                return true;
            }

            m_place[sourceAction[2]].placeCode = extraData[0];

            m_place[sourceAction[2]].displayName = extraData[1];

            qCDebug(IONENGINE_WETTERCOM) << "About to retrieve forecast for source: " << sourceAction[2];

            fetchForecast(sourceAction[2]);

            return true;
        }

        return false;
    }

    setData(source, QStringLiteral("validate"), QStringLiteral("wettercom|malformed"));
    return true;
}


/*
 * Handling of place searches
 */

void WetterComIon::findPlace(const QString& place, const QString& source)
{
    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(QByteArray(PROJECTNAME));
    md5.addData(QByteArray(APIKEY));
    md5.addData(place.toUtf8());
    const QString encodedKey = QString::fromLatin1(md5.result().toHex());

    const QUrl url(QStringLiteral(SEARCH_URL).arg(place, encodedKey));

    KIO::TransferJob* getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none")); // Disable displaying cookies
    m_searchJobXml.insert(getJob, new QXmlStreamReader);
    m_searchJobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data,
            this, &WetterComIon::setup_slotDataArrived);
    connect(getJob, &KJob::result,
            this, &WetterComIon::setup_slotJobFinished);
}

void WetterComIon::setup_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    QByteArray local = data;

    if (data.isEmpty() || !m_searchJobXml.contains(job)) {
        return;
    }

    m_searchJobXml[job]->addData(local);
}

void WetterComIon::setup_slotJobFinished(KJob *job)
{
    if (job->error() == KIO::ERR_SERVER_TIMEOUT) {
        setData(m_searchJobList[job], QStringLiteral("validate"), QStringLiteral("wettercom|timeout"));
        disconnectSource(m_searchJobList[job], this);
        m_searchJobList.remove(job);
        delete m_searchJobXml[job];
        m_searchJobXml.remove(job);
        return;
    }

    QXmlStreamReader *reader = m_searchJobXml.value(job);

    if (reader) {
        parseSearchResults(m_searchJobList[job], *reader);
    }

    m_searchJobList.remove(job);

    delete m_searchJobXml[job];
    m_searchJobXml.remove(job);
}

void WetterComIon::parseSearchResults(const QString& source, QXmlStreamReader& xml)
{
    QString name, code, quarter, state, country;

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement()) {
            if (elementName == QLatin1String("search")) {
                break;
            } else if (elementName == QLatin1String("item")) {
                // we parsed a place from the search result
                QString placeName;

                if (quarter.isEmpty()) {
                    placeName = i18nc("Geographical location: city, state, ISO-country-code", "%1, %2, %3",
                                      name, state, country);
                } else {
                    placeName = i18nc("Geographical location: quarter (city), state, ISO-country-code",
                                      "%1 (%2), %3, %4", quarter, name, state, country);
                }

                qCDebug(IONENGINE_WETTERCOM) << "Storing place data for place:" << placeName;

                PlaceInfo& place = m_place[placeName];
                place.name = placeName;
                place.displayName = name;
                place.placeCode = code;
                m_locations.append(placeName);

                name.clear();
                code.clear();
                quarter.clear();
                country.clear();
                state.clear();
            }
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("name")) {
                name = xml.readElementText();
            } else if (elementName == QLatin1String("city_code")) {
                code = xml.readElementText();
            } else if (elementName == QLatin1String("quarter")) {
                quarter = xml.readElementText();
            } else if (elementName == QLatin1String("adm_1_code")) {
                country = xml.readElementText();
            } else if (elementName == QLatin1String("adm_2_name")) {
                state = xml.readElementText();
            }
        }
    }

    validate(source, xml.error() != QXmlStreamReader::NoError);
}

void WetterComIon::validate(const QString& source, bool parseError)
{
    if (!m_locations.count() || parseError) {
        const QString invalidPlace = source.section(QLatin1Char('|'), 2, 2);

        if (m_place[invalidPlace].name.isEmpty()) {
            setData(source, QStringLiteral("validate"),
                    QVariant(QLatin1String("wettercom|invalid|multiple|") + invalidPlace));
        }

        m_locations.clear();

        return;
    }

    QString placeList;
    for (const QString& place : qAsConst(m_locations)) {
        // Extra data format: placeCode;displayName
        placeList.append(QLatin1String("|place|") + place + QLatin1String("|extra|") +
                         m_place[place].placeCode + QLatin1Char(';') + m_place[place].displayName);
    }

    qCDebug(IONENGINE_WETTERCOM) << "Returning place list:" << placeList;

    if (m_locations.count() > 1) {
        setData(source, QStringLiteral("validate"),
                QVariant(QStringLiteral("wettercom|valid|multiple") + placeList));
    } else {
        placeList[7] = placeList[7].toUpper();
        setData(source, QStringLiteral("validate"),
                QVariant(QStringLiteral("wettercom|valid|single") + placeList));
    }

    m_locations.clear();
}


/*
 * Handling of forecasts
 */

void WetterComIon::fetchForecast(const QString& source)
{
    for (const QString& fetching : qAsConst(m_forecastJobList)) {
        if (fetching == source) {
            // already fetching!
            return;
        }
    }

    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(QByteArray(PROJECTNAME));
    md5.addData(QByteArray(APIKEY));
    md5.addData(m_place[source].placeCode.toUtf8());
    const QString encodedKey = QString::fromLatin1(md5.result().toHex());

    const QUrl url(QStringLiteral(FORECAST_URL).arg(m_place[source].placeCode, encodedKey));

    KIO::TransferJob* getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none"));
    m_forecastJobXml.insert(getJob, new QXmlStreamReader);
    m_forecastJobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data,
            this, &WetterComIon::forecast_slotDataArrived);
    connect(getJob, &KJob::result,
            this, &WetterComIon::forecast_slotJobFinished);
}

void WetterComIon::forecast_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    QByteArray local = data;

    if (data.isEmpty() || !m_forecastJobXml.contains(job)) {
        return;
    }

    m_forecastJobXml[job]->addData(local);
}

void WetterComIon::forecast_slotJobFinished(KJob *job)
{
    const QString source(m_forecastJobList.value(job));
    setData(source, Data());
    QXmlStreamReader *reader = m_forecastJobXml.value(job);

    if (reader) {
        parseWeatherForecast(source, *reader);
    }

    m_forecastJobList.remove(job);

    delete m_forecastJobXml[job];
    m_forecastJobXml.remove(job);

    if (m_sourcesToReset.contains(source)) {
        m_sourcesToReset.removeAll(source);
        const QString weatherSource = QStringLiteral("wettercom|weather|%1|%2;%3")
            .arg(source,
                 m_place[source].placeCode,
                 m_place[source].displayName);

        // so the weather engine updates it's data
        forceImmediateUpdateOfAllVisualizations();

        // update the clients of our engine
        emit forceUpdate(this, weatherSource);
    }
}

void WetterComIon::parseWeatherForecast(const QString& source, QXmlStreamReader& xml)
{
    qCDebug(IONENGINE_WETTERCOM) << "About to parse forecast for source:" << source;

    WeatherData& weatherData = m_weatherData[source];

    // Clear old forecasts when updating
    weatherData.forecasts.clear();

    WeatherData::ForecastPeriod *forecastPeriod = new WeatherData::ForecastPeriod;
    WeatherData::ForecastInfo *forecast = new WeatherData::ForecastInfo;
    int summaryWeather = -1, summaryProbability = 0;
    int tempMax = -273, tempMin = 100, weather = -1, probability = 0;
    uint summaryUtcTime = 0, utcTime = 0, localTime = 0;
    QString date, time;

    weatherData.place = source;

    while (!xml.atEnd()) {
        xml.readNext();

        qCDebug(IONENGINE_WETTERCOM) << "parsing xml elem: " << xml.name();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement()) {
            if (elementName == QLatin1String("city")) {
                break;
            }
            if (elementName == QLatin1String("date")) {
                // we have parsed a complete day

                forecastPeriod->period = QDateTime::fromSecsSinceEpoch(summaryUtcTime, Qt::LocalTime);
                QString weatherString = QString::number(summaryWeather);
                forecastPeriod->iconName = getWeatherIcon(dayIcons(),
                                           weatherString);
                forecastPeriod->summary = getWeatherCondition(dayConditions(),
                                          weatherString);
                forecastPeriod->probability = summaryProbability;

                weatherData.forecasts.append(forecastPeriod);
                forecastPeriod = new WeatherData::ForecastPeriod;

                date.clear();
                summaryWeather = -1;
                summaryProbability = 0;
                summaryUtcTime = 0;
            } else if (elementName == QLatin1String("time")) {
                // we have parsed one forecast

                qCDebug(IONENGINE_WETTERCOM) << "Parsed a forecast interval:" << date << time;

                // yep, that field is written to more often than needed...
                weatherData.timeDifference = localTime - utcTime;

                forecast->period = QDateTime::fromSecsSinceEpoch(utcTime, Qt::LocalTime);
                QString weatherString = QString::number(weather);
                forecast->tempHigh = tempMax;
                forecast->tempLow = tempMin;
                forecast->probability = probability;

                QTime localWeatherTime = QDateTime::fromSecsSinceEpoch(utcTime, Qt::LocalTime).time();
                localWeatherTime = localWeatherTime.addSecs(weatherData.timeDifference);

                qCDebug(IONENGINE_WETTERCOM) << "localWeatherTime =" << localWeatherTime;

                // TODO use local sunset/sunrise time

                if (localWeatherTime.hour() < 20 && localWeatherTime.hour() > 6) {
                    forecast->iconName = getWeatherIcon(dayIcons(),
                                                        weatherString);
                    forecast->summary = getWeatherCondition(dayConditions(),
                                                            weatherString);
                    forecastPeriod->dayForecasts.append(forecast);
                } else {
                    forecast->iconName = getWeatherIcon(nightIcons(),
                                                        weatherString);
                    forecast->summary = getWeatherCondition(nightConditions(),
                                                            weatherString);
                    forecastPeriod->nightForecasts.append(forecast);
                }

                forecast = new WeatherData::ForecastInfo;

                tempMax = -273;
                tempMin = 100;
                weather = -1;
                probability = 0;
                utcTime = localTime = 0;
                time.clear();
            }
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("date")) {
                date = xml.attributes().value(QStringLiteral("value")).toString();
            } else if (elementName == QLatin1String("time")) {
                time = xml.attributes().value(QStringLiteral("value")).toString();
            } else if (elementName == QLatin1String("tx")) {
                tempMax = qRound(xml.readElementText().toDouble());
                qCDebug(IONENGINE_WETTERCOM) << "parsed t_max:" << tempMax;
            } else if (elementName == QLatin1String("tn")) {
                tempMin = qRound(xml.readElementText().toDouble());
                qCDebug(IONENGINE_WETTERCOM) << "parsed t_min:" << tempMin;
            } else if (elementName == QLatin1Char('w')) {
                int tmp = xml.readElementText().toInt();

                if (!time.isEmpty())
                    weather = tmp;
                else
                    summaryWeather = tmp;

                qCDebug(IONENGINE_WETTERCOM) << "parsed weather condition:" << tmp;
            } else if (elementName == QLatin1String("name")) {
                weatherData.stationName = xml.readElementText();
                qCDebug(IONENGINE_WETTERCOM) << "parsed station name:" << weatherData.stationName;
            } else if (elementName == QLatin1String("pc")) {
                int tmp = xml.readElementText().toInt();

                if (!time.isEmpty())
                    probability = tmp;
                else
                    summaryProbability = tmp;

                qCDebug(IONENGINE_WETTERCOM) << "parsed probability:" << probability;
            } else if (elementName == QLatin1String("text")) {
                weatherData.credits = xml.readElementText();
                qCDebug(IONENGINE_WETTERCOM) << "parsed credits:" << weatherData.credits;
            } else if (elementName == QLatin1String("link")) {
                weatherData.creditsUrl = xml.readElementText();
                qCDebug(IONENGINE_WETTERCOM) << "parsed credits url:" << weatherData.creditsUrl;
            } else if (elementName == QLatin1Char('d')) {
                localTime = xml.readElementText().toInt();
                qCDebug(IONENGINE_WETTERCOM) << "parsed local time:" << localTime;
            } else if (elementName == QLatin1String("du")) {
                int tmp = xml.readElementText().toInt();

                if (!time.isEmpty())
                    utcTime = tmp;
                else
                    summaryUtcTime = tmp;

                qCDebug(IONENGINE_WETTERCOM) << "parsed UTC time:" << tmp;
            }
        }
    }

    delete forecast;
    delete forecastPeriod;

    updateWeather(source, xml.error() != QXmlStreamReader::NoError);
}

void WetterComIon::updateWeather(const QString& source, bool parseError)
{
    qCDebug(IONENGINE_WETTERCOM) << "Source:" << source;

    const PlaceInfo& placeInfo = m_place[source];

    QString weatherSource = QStringLiteral("wettercom|weather|%1|%2;%3")
                            .arg(source,
                                 placeInfo.placeCode,
                                 placeInfo.displayName);

    const WeatherData& weatherData = m_weatherData[source];

    Plasma::DataEngine::Data data;
    data.insert(QStringLiteral("Place"), placeInfo.displayName);

    if (!parseError && !weatherData.forecasts.isEmpty()) {
        data.insert(QStringLiteral("Station"), placeInfo.displayName);
        //data.insert("Condition Icon", "N/A");
        //data.insert("Temperature", "N/A");
        data.insert(QStringLiteral("Temperature Unit"), KUnitConversion::Celsius);

        int i = 0;
        for (const WeatherData::ForecastPeriod* forecastPeriod : weatherData.forecasts) {
            if (i > 0) {
                WeatherData::ForecastInfo weather = forecastPeriod->getWeather();

                data.insert(QStringLiteral("Short Forecast Day %1").arg(i),
                            QStringLiteral("%1|%2|%3|%4|%5|%6")
                            .arg(QLocale().toString(weather.period.date().day()),
                                 weather.iconName,
                                 weather.summary)
                            .arg(weather.tempHigh)
                            .arg(weather.tempLow)
                            .arg(weather.probability));
                i++;
            } else {
                WeatherData::ForecastInfo dayWeather = forecastPeriod->getDayWeather();

                data.insert(QStringLiteral("Short Forecast Day %1").arg(i),
                            QStringLiteral("%1|%2|%3|%4|%5|%6")
                            .arg(i18n("Day"),
                                 dayWeather.iconName,
                                 dayWeather.summary)
                            .arg(dayWeather.tempHigh)
                            .arg(dayWeather.tempLow)
                            .arg(dayWeather.probability));
                i++;

                if (forecastPeriod->hasNightWeather()) {
                    WeatherData::ForecastInfo nightWeather = forecastPeriod->getNightWeather();
                    data.insert(QStringLiteral("Short Forecast Day %1").arg(i),
                                QStringLiteral("%1 nt|%2|%3|%4|%5|%6")
                                .arg(i18n("Night"),
                                     nightWeather.iconName,
                                     nightWeather.summary)
                                .arg(nightWeather.tempHigh)
                                .arg(nightWeather.tempLow)
                                .arg(nightWeather.probability));
                    i++;
                }
            }
        }

        // Set number of forecasts per day/night supported
        data.insert(QStringLiteral("Total Weather Days"), i);

        data.insert(QStringLiteral("Credit"), weatherData.credits); // FIXME i18n?
        data.insert(QStringLiteral("Credit Url"), weatherData.creditsUrl);

        qCDebug(IONENGINE_WETTERCOM) << "updated weather data:" << weatherSource << data;
    } else {
        qCDebug(IONENGINE_WETTERCOM) << "Something went wrong when parsing weather data for source:" << source;
    }

    setData(weatherSource, data);
}


/*
 * WeatherData::ForecastPeriod convenience methods
 */

WeatherData::ForecastPeriod::~ForecastPeriod()
{
    qDeleteAll(dayForecasts);
    qDeleteAll(nightForecasts);
}

WeatherData::ForecastInfo WeatherData::ForecastPeriod::getDayWeather() const
{
    WeatherData::ForecastInfo result;
    result.period = period;
    result.iconName = iconName;
    result.summary = summary;
    result.tempHigh = getMaxTemp(dayForecasts);
    result.tempLow = getMinTemp(dayForecasts);
    result.probability = probability;
    return result;
}

WeatherData::ForecastInfo WeatherData::ForecastPeriod::getNightWeather() const
{
    qCDebug(IONENGINE_WETTERCOM) << "nightForecasts.size() =" << nightForecasts.size();

    // TODO do not just pick the first night forecast
    return *(nightForecasts.at(0));
}

bool WeatherData::ForecastPeriod::hasNightWeather() const
{
    return !nightForecasts.isEmpty();
}

WeatherData::ForecastInfo WeatherData::ForecastPeriod::getWeather() const
{
    WeatherData::ForecastInfo result = getDayWeather();
    result.tempHigh = std::max(result.tempHigh, getMaxTemp(nightForecasts));
    result.tempLow = std::min(result.tempLow, getMinTemp(nightForecasts));
    return result;
}

int WeatherData::ForecastPeriod::getMaxTemp(const QVector<WeatherData::ForecastInfo*>& forecastInfos) const
{
    int result = -273;
    for (const WeatherData::ForecastInfo* forecast : forecastInfos) {
        result = std::max(result, forecast->tempHigh);
    }

    return result;
}

int WeatherData::ForecastPeriod::getMinTemp(const QVector<WeatherData::ForecastInfo*>& forecastInfos) const
{
    int result = 100;
    for (const WeatherData::ForecastInfo* forecast : forecastInfos) {
        result = std::min(result, forecast->tempLow);
    }

    return result;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(wettercom, WetterComIon, "ion-wettercom.json")


#include "ion_wettercom.moc"
