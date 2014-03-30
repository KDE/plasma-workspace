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
// http://api.wetter.com/location/index/search/Heidelberg/project/weatherion/cs/9090dec6e783b96bd6a6ca9d451f3fee
// http://api.wetter.com/forecast/weather/city/DE0004329/project/weatherion/cs/89f1264869cce5c6fd5a2db80051f3d8

#include "ion_wettercom.h"
#include <QDebug>
#include <KUnitConversion/Converter>
#include <KLocalizedDate>

/*
 * Initialization
 */

WetterComIon::WetterComIon(QObject *parent, const QVariantList &args)
        : IonInterface(parent, args)

{
    Q_UNUSED(args)

#if defined(MIN_POLL_INTERVAL)
    setMinimumPollingInterval(MIN_POLL_INTERVAL);
#endif
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

void WetterComIon::init()
{
    setInitialized(true);
}

QMap<QString, IonInterface::ConditionIcons> WetterComIon::setupCommonIconMappings(void) const
{
    QMap<QString, ConditionIcons> conditionList;

    conditionList["3"]  = Overcast;
    conditionList["30"] = Overcast;
    conditionList["4"]  = Haze;
    conditionList["40"] = Haze;
    conditionList["45"] = Haze;
    conditionList["48"] = Haze;
    conditionList["49"] = Haze;
    conditionList["5"]  = Mist;
    conditionList["50"] = Mist;
    conditionList["51"] = Mist;
    conditionList["53"] = Mist;
    conditionList["55"] = Mist;
    conditionList["56"] = FreezingDrizzle;
    conditionList["57"] = FreezingDrizzle;
    conditionList["6"]  = Rain;
    conditionList["60"] = LightRain;
    conditionList["61"] = LightRain;
    conditionList["63"] = Rain;
    conditionList["65"] = Rain;
    conditionList["66"] = FreezingRain;
    conditionList["67"] = FreezingRain;
    conditionList["68"] = RainSnow;
    conditionList["69"] = RainSnow;
    conditionList["7"]  = Snow;
    conditionList["70"] = LightSnow;
    conditionList["71"] = LightSnow;
    conditionList["73"] = Snow;
    conditionList["75"] = Flurries;
    conditionList["8"]  = Showers;
    conditionList["81"] = Showers;
    conditionList["82"] = Showers;
    conditionList["83"] = RainSnow;
    conditionList["84"] = RainSnow;
    conditionList["85"] = Snow;
    conditionList["86"] = Snow;
    conditionList["9"] = Thunderstorm;
    conditionList["90"] = Thunderstorm;
    conditionList["96"] = Thunderstorm;
    conditionList["999"] = NotAvailable;

    return conditionList;
}

QMap<QString, IonInterface::ConditionIcons> WetterComIon::setupDayIconMappings(void) const
{
    QMap<QString, ConditionIcons> conditionList = setupCommonIconMappings();

    conditionList["0"]  = ClearDay;
    conditionList["1"]  = FewCloudsDay;
    conditionList["10"] = FewCloudsDay;
    conditionList["2"]  = PartlyCloudyDay;
    conditionList["20"] = PartlyCloudyDay;
    conditionList["80"] = ChanceShowersDay;
    conditionList["95"] = ChanceThunderstormDay;

    return conditionList;
}

QMap<QString, IonInterface::ConditionIcons> const& WetterComIon::dayIcons(void) const
{
    static QMap<QString, ConditionIcons> const val = setupDayIconMappings();
    return val;
}

QMap<QString, IonInterface::ConditionIcons> WetterComIon::setupNightIconMappings(void) const
{
    QMap<QString, ConditionIcons> conditionList = setupCommonIconMappings();

    conditionList["0"]  = ClearNight;
    conditionList["1"]  = FewCloudsNight;
    conditionList["10"] = FewCloudsNight;
    conditionList["2"]  = PartlyCloudyNight;
    conditionList["20"] = PartlyCloudyNight;
    conditionList["80"] = ChanceShowersNight;
    conditionList["95"] = ChanceThunderstormNight;

    return conditionList;
}

QMap<QString, IonInterface::ConditionIcons> const& WetterComIon::nightIcons(void) const
{
    static QMap<QString, ConditionIcons> const val = setupNightIconMappings();
    return val;
}

QMap<QString, QString> WetterComIon::setupCommonConditionMappings(void) const
{
    QMap<QString, QString> conditionList;
    conditionList["1"]  = i18nc("weather condition", "few clouds");
    conditionList["10"] = i18nc("weather condition", "few clouds");
    conditionList["2"]  = i18nc("weather condition", "cloudy");
    conditionList["20"] = i18nc("weather condition", "cloudy");
    conditionList["3"]  = i18nc("weather condition", "overcast");
    conditionList["30"] = i18nc("weather condition", "overcast");
    conditionList["4"]  = i18nc("weather condition", "haze");
    conditionList["40"] = i18nc("weather condition", "haze");
    conditionList["45"] = i18nc("weather condition", "haze");
    conditionList["48"] = i18nc("weather condition", "fog with icing");
    conditionList["49"] = i18nc("weather condition", "fog with icing");
    conditionList["5"]  = i18nc("weather condition", "drizzle");
    conditionList["50"] = i18nc("weather condition", "drizzle");
    conditionList["51"] = i18nc("weather condition", "light drizzle");
    conditionList["53"] = i18nc("weather condition", "drizzle");
    conditionList["55"] = i18nc("weather condition", "heavy drizzle");
    conditionList["56"] = i18nc("weather condition", "freezing drizzle");
    conditionList["57"] = i18nc("weather condition", "heavy freezing drizzle");
    conditionList["6"]  = i18nc("weather condition", "rain");
    conditionList["60"] = i18nc("weather condition", "light rain");
    conditionList["61"] = i18nc("weather condition", "light rain");
    conditionList["63"] = i18nc("weather condition", "moderate rain");
    conditionList["65"] = i18nc("weather condition", "heavy rain");
    conditionList["66"] = i18nc("weather condition", "light freezing rain");
    conditionList["67"] = i18nc("weather condition", "freezing rain");
    conditionList["68"] = i18nc("weather condition", "light rain snow");
    conditionList["69"] = i18nc("weather condition", "heavy rain snow");
    conditionList["7"]  = i18nc("weather condition", "snow");
    conditionList["70"] = i18nc("weather condition", "light snow");
    conditionList["71"] = i18nc("weather condition", "light snow");
    conditionList["73"] = i18nc("weather condition", "moderate snow");
    conditionList["75"] = i18nc("weather condition", "heavy snow");
    conditionList["8"]  = i18nc("weather condition", "showers");
    conditionList["80"] = i18nc("weather condition", "light showers");
    conditionList["81"] = i18nc("weather condition", "showers");
    conditionList["82"] = i18nc("weather condition", "heavy showers");
    conditionList["83"] = i18nc("weather condition", "light snow rain showers");
    conditionList["84"] = i18nc("weather condition", "heavy snow rain showers");
    conditionList["85"] = i18nc("weather condition", "light snow showers");
    conditionList["86"] = i18nc("weather condition", "snow showers");
    conditionList["9"]  = i18nc("weather condition", "thunderstorm");
    conditionList["90"] = i18nc("weather condition", "thunderstorm");
    conditionList["95"] = i18nc("weather condition", "light thunderstorm");
    conditionList["96"] = i18nc("weather condition", "heavy thunderstorm");
    conditionList["999"] = i18nc("weather condition", "n/a");

    return conditionList;
}

QMap<QString, QString> WetterComIon::setupDayConditionMappings(void) const
{
    QMap<QString, QString> conditionList = setupCommonConditionMappings();
    conditionList["0"]  = i18nc("weather condition", "sunny");
    return conditionList;
}

QMap<QString, QString> const& WetterComIon::dayConditions(void) const
{
    static QMap<QString, QString> const val = setupDayConditionMappings();
    return val;
}

QMap<QString, QString> WetterComIon::setupNightConditionMappings(void) const
{
    QMap<QString, QString> conditionList = setupCommonConditionMappings();
    conditionList["0"]  = i18nc("weather condition", "clear sky");
    return conditionList;
}

QMap<QString, QString> const& WetterComIon::nightConditions(void) const
{
    static QMap<QString, QString> const val = setupNightConditionMappings();
    return val;
}

QString WetterComIon::getWeatherCondition(const QMap<QString, QString> &conditionList, const QString& condition) const
{
    return conditionList[condition];
}

bool WetterComIon::updateIonSource(const QString& source)
{
    // We expect the applet to send the source in the following tokenization:
    // ionname|validate|place_name|extra - Triggers validation of place
    // ionname|weather|place_name|extra - Triggers receiving weather of place

    QStringList sourceAction = source.split('|');

    if (sourceAction.size() < 3) {
        setData(source, "validate", "wettercom|malformed");
        return true;
    }

    if (sourceAction[1] == "validate" && sourceAction.size() >= 3) {
        // Look for places to match
        findPlace(sourceAction[2], source);
        return true;
    } else if (sourceAction[1] == "weather" && sourceAction.size() >= 3) {
        if (sourceAction.count() >= 4) {
            if (sourceAction[2].isEmpty()) {
                setData(source, "validate", "wettercom|malformed");
                return true;
            }

            // Extra data format: placeCode;displayName
            QStringList extraData = sourceAction[3].split(';');

            if (extraData.count() != 2) {
                setData(source, "validate", "wettercom|malformed");
                return true;
            }

            m_place[sourceAction[2]].placeCode = extraData[0];

            m_place[sourceAction[2]].displayName = extraData[1];

            qDebug() << "About to retrieve forecast for source: " << sourceAction[2];

            fetchForecast(sourceAction[2]);

            return true;
        } else {
            return false;
        }
    } else {
        setData(source, "validate", "wettercom|malformed");
        return true;
    }

    return false;
}


/*
 * Handling of place searches
 */

void WetterComIon::findPlace(const QString& place, const QString& source)
{
    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(QString::fromLatin1(PROJECTNAME).toUtf8());
    md5.addData(QString::fromLatin1(APIKEY).toUtf8());
    md5.addData(place.toUtf8());

    KUrl url = QString::fromLatin1(SEARCH_URL).arg(place).arg(md5.result().toHex().data());

    m_job = KIO::get(url.url(), KIO::Reload, KIO::HideProgressInfo);
    m_job->addMetaData("cookies", "none");    // Disable displaying cookies
    m_searchJobXml.insert(m_job, new QXmlStreamReader);
    m_searchJobList.insert(m_job, source);

    if (m_job) {
        connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this,
                SLOT(setup_slotDataArrived(KIO::Job*,QByteArray)));
        connect(m_job, SIGNAL(result(KJob*)), this,
                SLOT(setup_slotJobFinished(KJob*)));
    }
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
    if (job->error() == 149) {
        setData(m_searchJobList[job], "validate",
                QString::fromLatin1("wettercom|timeout"));
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

        if (xml.isEndElement()) {
            if (xml.name() == "search") {
                break;
            } else if (xml.name() == "item") {
                // we parsed a place from the search result
                QString placeName;

                if (quarter.isEmpty()) {
                    placeName = i18nc("Geographical location: city, state, ISO-country-code", "%1, %2, %3",
                                      name, state, country);
                } else {
                    placeName = i18nc("Geographical location: quarter (city), state, ISO-country-code",
                                      "%1 (%2), %3, %4", quarter, name, state, country);
                }

                qDebug() << "Storing place data for place:" << placeName;

                m_place[placeName].name = placeName;
                m_place[placeName].displayName = name;
                m_place[placeName].placeCode = code;
                m_locations.append(placeName);

                name = "";
                code = "";
                quarter = "";
                country = "";
                state = "";
            }
        }

        if (xml.isStartElement()) {
            if (xml.name() == "name") {
                name = xml.readElementText();
            } else if (xml.name() == "city_code") {
                code = xml.readElementText();
            } else if (xml.name() == "quarter") {
                quarter = xml.readElementText();
            } else if (xml.name() == "adm_1_code") {
                country = xml.readElementText();
            } else if (xml.name() == "adm_2_name") {
                state = xml.readElementText();
            }
        }
    }

    validate(source, xml.error() != QXmlStreamReader::NoError);
}

void WetterComIon::validate(const QString& source, bool parseError)
{
    bool beginflag = true;

    if (!m_locations.count() || parseError) {
        QStringList invalidPlace = source.split('|');

        if (m_place[invalidPlace[2]].name.isEmpty()) {
            setData(source, "validate",
                    QString::fromLatin1("wettercom|invalid|multiple|%1")
                    .arg(invalidPlace[2]));
        }

        m_locations.clear();

        return;
    } else {
        QString placeList;
        foreach(const QString &place, m_locations) {
            // Extra data format: placeCode;displayName
            if (beginflag) {
                placeList.append(QString::fromLatin1("%1|extra|%2;%3")
                                 .arg(place).arg(m_place[place].placeCode)
                                 .arg(m_place[place].displayName));
                beginflag = false;
            } else {
                placeList.append(QString::fromLatin1("|place|%1|extra|%2;%3")
                                 .arg(place).arg(m_place[place].placeCode)
                                 .arg(m_place[place].displayName));
            }
        }

        qDebug() << "Returning place list:" << placeList;

        if (m_locations.count() > 1) {
            setData(source, "validate",
                    QString::fromLatin1("wettercom|valid|multiple|place|%1")
                    .arg(placeList));
        } else {
            placeList[0] = placeList[0].toUpper();
            setData(source, "validate",
                    QString::fromLatin1("wettercom|valid|single|place|%1")
                    .arg(placeList));
        }
    }

    m_locations.clear();
}


/*
 * Handling of forecasts
 */

void WetterComIon::fetchForecast(const QString& source)
{
    foreach (const QString &fetching, m_forecastJobList) {
        if (fetching == source) {
            // already fetching!
            return;
        }
    }

    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(QString::fromLatin1(PROJECTNAME).toUtf8());
    md5.addData(QString::fromLatin1(APIKEY).toUtf8());
    md5.addData(m_place[source].placeCode.toUtf8());

    KUrl url = QString::fromLatin1(FORECAST_URL)
               .arg(m_place[source].placeCode).arg(md5.result().toHex().data());

    m_job = KIO::get(url.url(), KIO::Reload, KIO::HideProgressInfo);
    m_job->addMetaData("cookies", "none");
    m_forecastJobXml.insert(m_job, new QXmlStreamReader);
    m_forecastJobList.insert(m_job, source);

    if (m_job) {
        connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this,
                SLOT(forecast_slotDataArrived(KIO::Job*,QByteArray)));
        connect(m_job, SIGNAL(result(KJob*)), this,
                SLOT(forecast_slotJobFinished(KJob*)));
    }
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
        const QString weatherSource = QString::fromLatin1("wettercom|weather|%1|%2;%3")
            .arg(source).arg(m_place[source].placeCode)
            .arg(m_place[source].displayName);

        // so the weather engine updates it's data
        forceImmediateUpdateOfAllVisualizations();

        // update the clients of our engine
        emit forceUpdate(this, weatherSource);
    }
}

void WetterComIon::parseWeatherForecast(const QString& source, QXmlStreamReader& xml)
{
    qDebug() << "About to parse forecast for source:" << source;

    // Clear old forecasts when updating
    m_weatherData[source].forecasts.clear();

    WeatherData::ForecastPeriod *forecastPeriod = new WeatherData::ForecastPeriod;
    WeatherData::ForecastInfo *forecast = new WeatherData::ForecastInfo;
    int summaryWeather = -1, summaryProbability = 0;
    int tempMax = -273, tempMin = 100, weather = -1, probability = 0;
    uint summaryUtcTime = 0, utcTime = 0, localTime = 0;
    QString date, time;

    m_weatherData[source].place = source;

    while (!xml.atEnd()) {
        xml.readNext();

        qDebug() << "parsing xml elem: " << xml.name();

        if (xml.isEndElement()) {
            if (xml.name() == "city") {
                break;
            } else if (xml.name() == "date") {
                // we have parsed a complete day

                forecastPeriod->period = QDateTime::fromTime_t(summaryUtcTime);
                QString weatherString = QString::number(summaryWeather);
                forecastPeriod->iconName = getWeatherIcon(dayIcons(),
                                           weatherString);
                forecastPeriod->summary = getWeatherCondition(dayConditions(),
                                          weatherString);
                forecastPeriod->probability = summaryProbability;

                m_weatherData[source].forecasts.append(forecastPeriod);
                forecastPeriod = new WeatherData::ForecastPeriod;

                date = "";
                summaryWeather = -1;
                summaryProbability = 0;
                summaryUtcTime = 0;
            } else if (xml.name() == "time") {
                // we have parsed one forecast

                qDebug() << "Parsed a forecast interval:" << date << time;

                // yep, that field is written to more often than needed...
                m_weatherData[source].timeDifference = localTime - utcTime;

                forecast->period = QDateTime::fromTime_t(utcTime);
                QString weatherString = QString::number(weather);
                forecast->tempHigh = tempMax;
                forecast->tempLow = tempMin;
                forecast->probability = probability;

                QTime localWeatherTime = QDateTime::fromTime_t(utcTime).time();
                localWeatherTime.addSecs(m_weatherData[source].timeDifference);

                qDebug() << "localWeatherTime =" << localWeatherTime;

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
                time = "";
            }
        }

        if (xml.isStartElement()) {
            if (xml.name() == "date") {
                date = xml.attributes().value("value").toString();
            } else if (xml.name() == "time") {
                time = xml.attributes().value("value").toString();
            } else if (xml.name() == "tx") {
                tempMax = qRound(xml.readElementText().toDouble());
                qDebug() << "parsed t_max:" << tempMax;
            } else if (xml.name() == "tn") {
                tempMin = qRound(xml.readElementText().toDouble());
                qDebug() << "parsed t_min:" << tempMin;
            } else if (xml.name() == "w") {
                int tmp = xml.readElementText().toInt();

                if (!time.isEmpty())
                    weather = tmp;
                else
                    summaryWeather = tmp;

                qDebug() << "parsed weather condition:" << tmp;
            } else if (xml.name() == "name") {
                m_weatherData[source].stationName = xml.readElementText();
                qDebug() << "parsed station name:" << m_weatherData[source].stationName;
            } else if (xml.name() == "pc") {
                int tmp = xml.readElementText().toInt();

                if (!time.isEmpty())
                    probability = tmp;
                else
                    summaryProbability = tmp;

                qDebug() << "parsed probability:" << probability;
            } else if (xml.name() == "text") {
                m_weatherData[source].credits = xml.readElementText();
                qDebug() << "parsed credits:" << m_weatherData[source].credits;
            } else if (xml.name() == "link") {
                m_weatherData[source].creditsUrl = xml.readElementText();
                qDebug() << "parsed credits url:" << m_weatherData[source].creditsUrl;
            } else if (xml.name() == "d") {
                localTime = xml.readElementText().toInt();
                qDebug() << "parsed local time:" << localTime;
            } else if (xml.name() == "du") {
                int tmp = xml.readElementText().toInt();

                if (!time.isEmpty())
                    utcTime = tmp;
                else
                    summaryUtcTime = tmp;

                qDebug() << "parsed UTC time:" << tmp;
            }
        }
    }

    delete forecast;
    delete forecastPeriod;

    updateWeather(source, xml.error() != QXmlStreamReader::NoError);
}

void WetterComIon::updateWeather(const QString& source, bool parseError)
{
    qDebug() << "Source:" << source;

    QString weatherSource = QString::fromLatin1("wettercom|weather|%1|%2;%3")
                            .arg(source).arg(m_place[source].placeCode)
                            .arg(m_place[source].displayName);

    Plasma::DataEngine::Data data;
    data.insert("Place", m_place[source].displayName);

    if (!parseError && !m_weatherData[source].forecasts.isEmpty()) {
        data.insert("Station", m_place[source].displayName);
        //data.insert("Condition Icon", "N/A");
        //data.insert("Temperature", "N/A");
        data.insert("Temperature Unit", QString::number(KUnitConversion::Celsius));

        int i = 0;
        foreach(WeatherData::ForecastPeriod * forecastPeriod, m_weatherData[source].forecasts) {
            if (i > 0) {
                WeatherData::ForecastInfo weather = forecastPeriod->getWeather();

                data.insert(QString::fromLatin1("Short Forecast Day %1").arg(i),
                            QString::fromLatin1("%1|%2|%3|%4|%5|%6")
                            .arg(KLocalizedDate(weather.period.date()).formatDate(KLocale::DayName, KLocale::ShortName))
                            .arg(weather.iconName).arg(weather.summary)
                            .arg(weather.tempHigh).arg(weather.tempLow)
                            .arg(weather.probability));
                i++;
            } else {
                WeatherData::ForecastInfo dayWeather = forecastPeriod->getDayWeather();

                data.insert(QString::fromLatin1("Short Forecast Day %1").arg(i),
                            QString::fromLatin1("%1|%2|%3|%4|%5|%6")
                            .arg(i18n("Day")).arg(dayWeather.iconName)
                            .arg(dayWeather.summary).arg(dayWeather.tempHigh)
                            .arg(dayWeather.tempLow).arg(dayWeather.probability));
                i++;

                if (forecastPeriod->hasNightWeather()) {
                    WeatherData::ForecastInfo nightWeather = forecastPeriod->getNightWeather();
                    data.insert(QString::fromLatin1("Short Forecast Day %1").arg(i),
                                QString::fromLatin1("%1 nt|%2|%3|%4|%5|%6")
                                .arg(i18n("Night")).arg(nightWeather.iconName)
                                .arg(nightWeather.summary)
                                .arg(nightWeather.tempHigh)
                                .arg(nightWeather.tempLow)
                                .arg(nightWeather.probability));
                    i++;
                }
            }
        }

        // Set number of forecasts per day/night supported
        data.insert("Total Weather Days", i);

        data.insert("Credit", m_weatherData[source].credits); // FIXME i18n?
        data.insert("Credit Url", m_weatherData[source].creditsUrl);

        qDebug() << "updated weather data:" << data;
    } else {
        qDebug() << "Something went wrong when parsing weather data for source:" << source;
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
    qDebug() << "nightForecasts.size() =" << nightForecasts.size();

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

int WeatherData::ForecastPeriod::getMaxTemp(QVector<WeatherData::ForecastInfo *> forecastInfos) const
{
    int result = -273;
    foreach(const WeatherData::ForecastInfo * forecast, forecastInfos) {
        result = std::max(result, forecast->tempHigh);
    }

    return result;
}

int WeatherData::ForecastPeriod::getMinTemp(QVector<WeatherData::ForecastInfo *> forecastInfos) const
{
    int result = 100;
    foreach(const WeatherData::ForecastInfo * forecast, forecastInfos) {
        result = std::min(result, forecast->tempLow);
    }

    return result;
}

#include "ion_wettercom.moc"
// kate: indent-mode cstyle; space-indent on; indent-width 4; 
