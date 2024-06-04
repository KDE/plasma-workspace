/*
    SPDX-FileCopyrightText: 2007-2009, 2019 Shawn Starr <shawn.starr@rogers.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Ion for NOAA's National Weather Service XML data */

#include "ion_noaa.h"

#include "ion_noaadebug.h"

#include <KIO/TransferJob>
#include <KLocalizedString>
#include <KUnitConversion/Converter>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QTimeZone>

using namespace Qt::StringLiterals;

WeatherData::WeatherData()
    : stationLatitude(qQNaN())
    , stationLongitude(qQNaN())
    , temperature_F(qQNaN())
    , temperature_C(qQNaN())
    , humidity(qQNaN())
    , windSpeed(qQNaN())
    , windGust(qQNaN())
    , pressure(qQNaN())
    , dewpoint_F(qQNaN())
    , dewpoint_C(qQNaN())
    , heatindex_F(qQNaN())
    , heatindex_C(qQNaN())
    , windchill_F(qQNaN())
    , windchill_C(qQNaN())
    , visibility(qQNaN())
{
}

QMap<QString, IonInterface::WindDirections> NOAAIon::setupWindIconMappings() const
{
    return QMap<QString, WindDirections>{
        {QStringLiteral("north"), N},
        {QStringLiteral("northeast"), NE},
        {QStringLiteral("south"), S},
        {QStringLiteral("southwest"), SW},
        {QStringLiteral("east"), E},
        {QStringLiteral("southeast"), SE},
        {QStringLiteral("west"), W},
        {QStringLiteral("northwest"), NW},
        {QStringLiteral("calm"), VR},
    };
}

QMap<QString, IonInterface::ConditionIcons> NOAAIon::setupConditionIconMappings() const
{
    QMap<QString, ConditionIcons> conditionList;
    return conditionList;
}

QMap<QString, IonInterface::ConditionIcons> const &NOAAIon::conditionIcons() const
{
    static QMap<QString, ConditionIcons> const condval = setupConditionIconMappings();
    return condval;
}

QMap<QString, IonInterface::WindDirections> const &NOAAIon::windIcons() const
{
    static QMap<QString, WindDirections> const wval = setupWindIconMappings();
    return wval;
}

// ctor, dtor
NOAAIon::NOAAIon(QObject *parent)
    : IonInterface(parent)
{
    // Get the real city XML URL so we can parse this
    getXMLSetup();
}

void NOAAIon::reset()
{
    m_sourcesToReset = sources();
    getXMLSetup();
}

NOAAIon::~NOAAIon()
{
    // seems necessary to avoid crash
    removeAllSources();
}

QStringList NOAAIon::validate(const QString &source) const
{
    QStringList placeList;
    QString station;
    QString sourceNormalized = source.toUpper();

    QHash<QString, NOAAIon::XMLMapInfo>::const_iterator it = m_places.constBegin();
    // If the source name might look like a station ID, check these too and return the name
    bool checkState = source.count() == 2;

    while (it != m_places.constEnd()) {
        if (checkState) {
            if (it.value().stateName == source) {
                placeList.append(QStringLiteral("place|").append(it.key()));
            }
        } else if (it.key().toUpper().contains(sourceNormalized)) {
            placeList.append(QStringLiteral("place|").append(it.key()));
        } else if (it.value().stationID == sourceNormalized) {
            station = QStringLiteral("place|").append(it.key());
        }

        ++it;
    }

    placeList.sort();
    if (!station.isEmpty()) {
        placeList.prepend(station);
    }

    return placeList;
}

bool NOAAIon::updateIonSource(const QString &source)
{
    // We expect the applet to send the source in the following tokenization:
    // ionname:validate:place_name - Triggers validation of place
    // ionname:weather:place_name - Triggers receiving weather of place

    QStringList sourceAction = source.split(QLatin1Char('|'));

    // Guard: if the size of array is not 2 then we have bad data, return an error
    if (sourceAction.size() < 2) {
        setData(source, QStringLiteral("validate"), QStringLiteral("noaa|malformed"));
        return true;
    }

    if (sourceAction[1] == QLatin1String("validate") && sourceAction.size() > 2) {
        QStringList result = validate(sourceAction[2]);

        if (result.size() == 1) {
            setData(source, QStringLiteral("validate"), QStringLiteral("noaa|valid|single|").append(result.join(QLatin1Char('|'))));
            return true;
        }
        if (result.size() > 1) {
            setData(source, QStringLiteral("validate"), QStringLiteral("noaa|valid|multiple|").append(result.join(QLatin1Char('|'))));
            return true;
        }
        // result.size() == 0
        setData(source, QStringLiteral("validate"), QStringLiteral("noaa|invalid|single|").append(sourceAction[2]));
        return true;
    }

    if (sourceAction[1] == QLatin1String("weather") && sourceAction.size() > 2) {
        getXMLData(source);
        return true;
    }

    setData(source, QStringLiteral("validate"), QStringLiteral("noaa|malformed"));
    return true;
}

KJob *NOAAIon::apiRequestJob(const QUrl &url, const QString &source)
{
    KIO::TransferJob *getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);

    m_jobData.insert(getJob, QByteArray());
    if (!source.isEmpty()) {
        m_jobList.insert(getJob, source);
    }

    qCDebug(IONENGINE_NOAA) << "Requesting URL:" << url;

    connect(getJob, &KIO::TransferJob::data, this, [this](KIO::Job *job, const QByteArray &data) {
        if (data.isEmpty() || !m_jobData.contains(job)) {
            return;
        }
        m_jobData[job].append(data);
    });

    return getJob;
}

// Parses city list and gets the correct city based on ID number
void NOAAIon::getXMLSetup(bool reset)
{
    const QStringList stationUrls = {
        u"https://w1.weather.gov/xml/current_obs/index.xml"_s,
        u"https://www.weather.gov/xml/current_obs/index.xml"_s,
    };
    static int retryCount = 0;

    if (reset) {
        retryCount = 0;
    } else {
        retryCount++;
        if (retryCount >= stationUrls.count()) {
            qCWarning(IONENGINE_NOAA) << "Couldn't retrieve the list of stations";
            return;
        }
    }

    auto getJob = apiRequestJob(QUrl(stationUrls.at(retryCount)), {});
    connect(getJob, &KJob::result, this, &NOAAIon::setup_slotJobFinished);
}

void NOAAIon::setup_slotJobFinished(KJob *job)
{
    QXmlStreamReader reader = QXmlStreamReader(m_jobData.value(job));

    const bool success = readXMLSetup(reader);
    setInitialized(success);

    if (!success) {
        getXMLSetup(/*reset*/ false);
    }

    m_jobData.remove(job);

    for (const QString &source : std::as_const(m_sourcesToReset)) {
        updateSourceEvent(source);
    }
}

// Gets specific city XML data
void NOAAIon::getXMLData(const QString &source)
{
    for (const QString &fetching : std::as_const(m_jobList)) {
        if (fetching == source) {
            // already getting this source and awaiting the data
            return;
        }
    }

    QString dataKey = source;
    dataKey.remove(QStringLiteral("noaa|weather|"));
    const QUrl url(m_places[dataKey].XMLurl);

    // If this is empty we have no valid data, send out an error and abort.
    if (url.url().isEmpty()) {
        setData(source, QStringLiteral("validate"), QStringLiteral("noaa|malformed"));
        return;
    }

    auto getJob = apiRequestJob(url, source);
    connect(getJob, &KJob::result, this, &NOAAIon::slotJobFinished);
}

void NOAAIon::slotJobFinished(KJob *job)
{
    // Dual use method, if we're fetching location data to parse we need to do this first
    const QString source(m_jobList.value(job));
    removeAllData(source);

    QXmlStreamReader reader = QXmlStreamReader(m_jobData.value(job));
    readXMLData(source, reader);

    // Now that we have the longitude and latitude, fetch the seven day forecast
    // and the alerts
    getForecast(source);
    getAlerts(source);

    m_jobList.remove(job);
    m_jobData.remove(job);
}

void NOAAIon::parseFloat(float &value, const QString &string)
{
    bool ok = false;
    const float result = string.toFloat(&ok);
    if (ok) {
        value = result;
    }
}

void NOAAIon::parseFloat(float &value, QXmlStreamReader &xml)
{
    bool ok = false;
    const float result = xml.readElementText().toFloat(&ok);
    if (ok) {
        value = result;
    }
}

void NOAAIon::parseDouble(double &value, QXmlStreamReader &xml)
{
    bool ok = false;
    const double result = xml.readElementText().toDouble(&ok);
    if (ok) {
        value = result;
    }
}

void NOAAIon::parseStationID(QXmlStreamReader &xml)
{
    QString state;
    QString stationName;
    QString stationID;
    QString xmlurl;

    while (!xml.atEnd()) {
        xml.readNext();

        const auto elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("station")) {
            if (!xmlurl.isEmpty()) {
                NOAAIon::XMLMapInfo info;
                info.stateName = state;
                info.stationName = stationName;
                info.stationID = stationID;
                info.XMLurl = xmlurl;

                QString tmp = stationName + QLatin1String(", ") + state; // Build the key name.
                m_places[tmp] = info;
            }
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("station_id")) {
                stationID = xml.readElementText();
            } else if (elementName == QLatin1String("state")) {
                state = xml.readElementText();
            } else if (elementName == QLatin1String("station_name")) {
                stationName = xml.readElementText();
            } else if (elementName == QLatin1String("xml_url")) {
                xmlurl = xml.readElementText().replace(QStringLiteral("http://"), QStringLiteral("http://www."));
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

void NOAAIon::parseStationList(QXmlStreamReader &xml)
{
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("station")) {
                parseStationID(xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

// Parse the city list and store into a QMap
bool NOAAIon::readXMLSetup(QXmlStreamReader &xml)
{
    bool success = false;
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("wx_station_index")) {
                parseStationList(xml);
                success = true;
            }
        }
    }
    return (!xml.error() && success);
}

void NOAAIon::parseWeatherSite(WeatherData &data, QXmlStreamReader &xml)
{
    data.temperature_C = qQNaN();
    data.temperature_F = qQNaN();
    data.dewpoint_C = qQNaN();
    data.dewpoint_F = qQNaN();
    data.weather = QStringLiteral("N/A");
    data.stationID = i18n("N/A");
    data.pressure = qQNaN();
    data.visibility = qQNaN();
    data.humidity = qQNaN();
    data.windSpeed = qQNaN();
    data.windGust = qQNaN();
    data.windchill_F = qQNaN();
    data.windchill_C = qQNaN();
    data.heatindex_F = qQNaN();
    data.heatindex_C = qQNaN();

    while (!xml.atEnd()) {
        xml.readNext();

        const auto elementName = xml.name();

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("location")) {
                data.locationName = xml.readElementText();
            } else if (elementName == QLatin1String("station_id")) {
                data.stationID = xml.readElementText();
            } else if (elementName == QLatin1String("latitude")) {
                parseDouble(data.stationLatitude, xml);
            } else if (elementName == QLatin1String("longitude")) {
                parseDouble(data.stationLongitude, xml);
            } else if (elementName == QLatin1String("observation_time_rfc822")) {
                data.observationDateTime = QDateTime::fromString(xml.readElementText(), Qt::RFC2822Date);
            } else if (elementName == QLatin1String("observation_time")) {
                data.observationTime = xml.readElementText();
                QStringList tmpDateStr = data.observationTime.split(QLatin1Char(' '));
                data.observationTime = QStringLiteral("%1 %2").arg(tmpDateStr[6], tmpDateStr[7]);
            } else if (elementName == QLatin1String("weather")) {
                const QString weather = xml.readElementText();
                data.weather = (weather.isEmpty() || weather == QLatin1String("NA")) ? QStringLiteral("N/A") : weather;
                // Pick which icon set depending on period of day
            } else if (elementName == QLatin1String("temp_f")) {
                parseFloat(data.temperature_F, xml);
            } else if (elementName == QLatin1String("temp_c")) {
                parseFloat(data.temperature_C, xml);
            } else if (elementName == QLatin1String("relative_humidity")) {
                parseFloat(data.humidity, xml);
            } else if (elementName == QLatin1String("wind_dir")) {
                data.windDirection = xml.readElementText();
            } else if (elementName == QLatin1String("wind_mph")) {
                const QString windSpeed = xml.readElementText();
                if (windSpeed == QLatin1String("NA")) {
                    data.windSpeed = 0.0;
                } else {
                    parseFloat(data.windSpeed, windSpeed);
                }
            } else if (elementName == QLatin1String("wind_gust_mph")) {
                const QString windGust = xml.readElementText();
                if (windGust == QLatin1String("NA") || windGust == QLatin1String("N/A")) {
                    data.windGust = 0.0;
                } else {
                    parseFloat(data.windGust, windGust);
                }
            } else if (elementName == QLatin1String("pressure_in")) {
                parseFloat(data.pressure, xml);
            } else if (elementName == QLatin1String("dewpoint_f")) {
                parseFloat(data.dewpoint_F, xml);
            } else if (elementName == QLatin1String("dewpoint_c")) {
                parseFloat(data.dewpoint_C, xml);
            } else if (elementName == QLatin1String("heat_index_f")) {
                parseFloat(data.heatindex_F, xml);
            } else if (elementName == QLatin1String("heat_index_c")) {
                parseFloat(data.heatindex_C, xml);
            } else if (elementName == QLatin1String("windchill_f")) {
                parseFloat(data.windchill_F, xml);
            } else if (elementName == QLatin1String("windchill_c")) {
                parseFloat(data.windchill_C, xml);
            } else if (elementName == QLatin1String("visibility_mi")) {
                parseFloat(data.visibility, xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

// Parse Weather data main loop, from here we have to descend into each tag pair
bool NOAAIon::readXMLData(const QString &source, QXmlStreamReader &xml)
{
    WeatherData data;
    data.isForecastsDataPending = true;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("current_observation")) {
                parseWeatherSite(data, xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }

    bool solarDataSourceNeedsConnect = false;
    Plasma5Support::DataEngine *timeEngine = dataEngine(QStringLiteral("time"));
    if (timeEngine) {
        const bool canCalculateElevation = (data.observationDateTime.isValid() && (!qIsNaN(data.stationLatitude) && !qIsNaN(data.stationLongitude)));
        if (canCalculateElevation) {
            data.solarDataTimeEngineSourceName = QStringLiteral("%1|Solar|Latitude=%2|Longitude=%3|DateTime=%4")
                                                     .arg(QString::fromUtf8(data.observationDateTime.timeZone().id()))
                                                     .arg(data.stationLatitude)
                                                     .arg(data.stationLongitude)
                                                     .arg(data.observationDateTime.toString(Qt::ISODate));
            solarDataSourceNeedsConnect = true;
        }

        // check any previous data
        const auto it = m_weatherData.constFind(source);
        if (it != m_weatherData.constEnd()) {
            const QString &oldSolarDataTimeEngineSource = it.value().solarDataTimeEngineSourceName;

            if (oldSolarDataTimeEngineSource == data.solarDataTimeEngineSourceName) {
                // can reuse elevation source (if any), copy over data
                data.isNight = it.value().isNight;
                solarDataSourceNeedsConnect = false;
            } else if (!oldSolarDataTimeEngineSource.isEmpty()) {
                // drop old elevation source
                timeEngine->disconnectSource(oldSolarDataTimeEngineSource, this);
            }
        }
    }

    m_weatherData[source] = data;

    // connect only after m_weatherData has the data, so the instant data push handling can see it
    if (solarDataSourceNeedsConnect) {
        data.isSolarDataPending = true;
        timeEngine->connectSource(data.solarDataTimeEngineSourceName, this);
    }

    return !xml.error();
}

// handle when no XML tag is found
void NOAAIon::parseUnknownElement(QXmlStreamReader &xml) const
{
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        if (xml.isStartElement()) {
            parseUnknownElement(xml);
        }
    }
}

void NOAAIon::updateWeather(const QString &source)
{
    const WeatherData &weatherData = m_weatherData[source];

    if (weatherData.isForecastsDataPending || weatherData.isSolarDataPending) {
        return;
    }

    Plasma5Support::DataEngine::Data data;

    data.insert(QStringLiteral("Place"), weatherData.locationName);
    data.insert(QStringLiteral("Station"), weatherData.stationID);

    const bool stationCoordValid = (!qIsNaN(weatherData.stationLatitude) && !qIsNaN(weatherData.stationLongitude));

    if (stationCoordValid) {
        data.insert(QStringLiteral("Latitude"), weatherData.stationLatitude);
        data.insert(QStringLiteral("Longitude"), weatherData.stationLongitude);
    }

    // Real weather - Current conditions
    if (weatherData.observationDateTime.isValid()) {
        data.insert(QStringLiteral("Observation Timestamp"), weatherData.observationDateTime);
    }

    data.insert(QStringLiteral("Observation Period"), weatherData.observationTime);

    const QString conditionI18n = weatherData.weather == QLatin1String("N/A") ? i18n("N/A") : i18nc("weather condition", weatherData.weather.toUtf8().data());

    data.insert(QStringLiteral("Current Conditions"), conditionI18n);
    qCDebug(IONENGINE_NOAA) << "i18n condition string: " << qPrintable(conditionI18n);

    const QString weather = weatherData.weather.toLower();
    ConditionIcons condition = getConditionIcon(weather, !weatherData.isNight);
    data.insert(QStringLiteral("Condition Icon"), getWeatherIcon(condition));

    if (!qIsNaN(weatherData.temperature_F)) {
        data.insert(QStringLiteral("Temperature"), weatherData.temperature_F);
    }

    // Used for all temperatures
    data.insert(QStringLiteral("Temperature Unit"), KUnitConversion::Fahrenheit);

    if (!qIsNaN(weatherData.windchill_F)) {
        data.insert(QStringLiteral("Windchill"), weatherData.windchill_F);
    }

    if (!qIsNaN(weatherData.heatindex_F)) {
        data.insert(QStringLiteral("Heat Index"), weatherData.heatindex_F);
    }

    if (!qIsNaN(weatherData.dewpoint_F)) {
        data.insert(QStringLiteral("Dewpoint"), weatherData.dewpoint_F);
    }

    if (!qIsNaN(weatherData.pressure)) {
        data.insert(QStringLiteral("Pressure"), weatherData.pressure);
        data.insert(QStringLiteral("Pressure Unit"), KUnitConversion::InchesOfMercury);
    }

    if (!qIsNaN(weatherData.visibility)) {
        data.insert(QStringLiteral("Visibility"), weatherData.visibility);
        data.insert(QStringLiteral("Visibility Unit"), KUnitConversion::Mile);
    }

    if (!qIsNaN(weatherData.humidity)) {
        data.insert(QStringLiteral("Humidity"), weatherData.humidity);
        data.insert(QStringLiteral("Humidity Unit"), KUnitConversion::Percent);
    }

    if (!qIsNaN(weatherData.windSpeed)) {
        data.insert(QStringLiteral("Wind Speed"), weatherData.windSpeed);
    }

    if (!qIsNaN(weatherData.windSpeed) || !qIsNaN(weatherData.windGust)) {
        data.insert(QStringLiteral("Wind Speed Unit"), KUnitConversion::MilePerHour);
    }

    if (!qIsNaN(weatherData.windGust)) {
        data.insert(QStringLiteral("Wind Gust"), weatherData.windGust);
    }

    if (!qIsNaN(weatherData.windSpeed) && static_cast<int>(weatherData.windSpeed) == 0) {
        data.insert(QStringLiteral("Wind Direction"), QStringLiteral("VR")); // Variable/calm
    } else if (!weatherData.windDirection.isEmpty()) {
        data.insert(QStringLiteral("Wind Direction"), getWindDirectionIcon(windIcons(), weatherData.windDirection.toLower()));
    }

    // Dayly forecasts
    int forecastDay = 0;
    for (const WeatherData::Forecast &forecast : weatherData.forecasts) {
        ConditionIcons icon = getConditionIcon(forecast.summary.toLower(), true);
        QString iconName = getWeatherIcon(icon);

        // Sometimes the forecast for the later days is unavailable, so skip
        // remianing days since their forecast data is probably unavailable.
        if (forecast.summary.isEmpty()) {
            break;
        }

        // Get the short day name for the forecast
        data.insert(QStringLiteral("Short Forecast Day %1").arg(forecastDay),
                    QStringLiteral("%1|%2|%3|%4|%5|%6")
                        .arg(forecast.day, iconName, i18nc("weather forecast", forecast.summary.toUtf8().data()), forecast.high, forecast.low)
                        .arg(forecast.precipitation));
        ++forecastDay;
    }
    // Set the number of days we provide after the filtering
    data.insert(QStringLiteral("Total Weather Days"), forecastDay);

    data.insert(u"Total Warnings Issued"_s, weatherData.alerts.size());
    int alertNum = 0;
    for (const WeatherData::Alert &alert : weatherData.alerts) {
        // TODO: Add a Headline parameter to the engine and the applet
        data.insert(u"Warning Description %1"_s.arg(alertNum), u"<p><b>%1</b></p>%2"_s.arg(alert.headline, alert.description));
        data.insert(u"Warning Timestamp %1"_s.arg(alertNum), QLocale().toString(alert.startTime, QLocale::ShortFormat));
        data.insert(u"Warning Priority %1"_s.arg(alertNum), alert.priority);
        ++alertNum;
    }

    data.insert(QStringLiteral("Credit"), i18nc("credit line, keep string short)", "Data from NOAA National\302\240Weather\302\240Service"));

    setData(source, data);
}

/**
 * Determine the condition icon based on the list of possible NOAA weather conditions as defined at
 * <https://www.weather.gov/xml/current_obs/weather.php> and
 * <https://graphical.weather.gov/xml/mdl/XML/Design/MDL_XML_Design.htm#_Toc141760782>
 * Since the number of NOAA weather conditions need to be fitted into the narowly defined groups in IonInterface::ConditionIcons, we
 * try to group the NOAA conditions as best as we can based on their priorities/severity.
 * TODO: summaries "Hot" & "Cold" have no proper matching entry in ConditionIcons, consider extending it
 */
IonInterface::ConditionIcons NOAAIon::getConditionIcon(const QString &weather, bool isDayTime) const
{
    IonInterface::ConditionIcons result;
    // Consider any type of storm, tornado or funnel to be a thunderstorm.
    if (weather.contains(QLatin1String("thunderstorm")) || weather.contains(QLatin1String("funnel")) || weather.contains(QLatin1String("tornado"))
        || weather.contains(QLatin1String("storm")) || weather.contains(QLatin1String("tstms"))) {
        if (weather.contains(QLatin1String("vicinity")) || weather.contains(QLatin1String("chance"))) {
            result = isDayTime ? IonInterface::ChanceThunderstormDay : IonInterface::ChanceThunderstormNight;
        } else {
            result = IonInterface::Thunderstorm;
        }

    } else if (weather.contains(QLatin1String("pellets")) || weather.contains(QLatin1String("crystals")) || weather.contains(QLatin1String("hail"))) {
        result = IonInterface::Hail;

    } else if (((weather.contains(QLatin1String("rain")) || weather.contains(QLatin1String("drizzle")) || weather.contains(QLatin1String("showers")))
                && weather.contains(QLatin1String("snow")))
               || weather.contains(QLatin1String("wintry mix"))) {
        result = IonInterface::RainSnow;

    } else if (weather.contains(QLatin1String("flurries"))) {
        result = IonInterface::Flurries;

    } else if (weather.contains(QLatin1String("snow")) && weather.contains(QLatin1String("light"))) {
        result = IonInterface::LightSnow;

    } else if (weather.contains(QLatin1String("snow"))) {
        if (weather.contains(QLatin1String("vicinity")) || weather.contains(QLatin1String("chance"))) {
            result = isDayTime ? IonInterface::ChanceSnowDay : IonInterface::ChanceSnowNight;
        } else {
            result = IonInterface::Snow;
        }

    } else if (weather.contains(QLatin1String("freezing rain"))) {
        result = IonInterface::FreezingRain;

    } else if (weather.contains(QLatin1String("freezing drizzle"))) {
        result = IonInterface::FreezingDrizzle;

    } else if (weather.contains(QLatin1String("cold"))) {
        // temperature condition has not hint about air ingredients, so let's assume chance of snow
        result = isDayTime ? IonInterface::ChanceSnowDay : IonInterface::ChanceSnowNight;

    } else if (weather.contains(QLatin1String("showers"))) {
        if (weather.contains(QLatin1String("vicinity")) || weather.contains(QLatin1String("chance"))) {
            result = isDayTime ? IonInterface::ChanceShowersDay : IonInterface::ChanceShowersNight;
        } else {
            result = IonInterface::Showers;
        }
    } else if (weather.contains(QLatin1String("light rain")) || weather.contains(QLatin1String("drizzle"))) {
        result = IonInterface::LightRain;

    } else if (weather.contains(QLatin1String("rain"))) {
        result = IonInterface::Rain;

    } else if (weather.contains(QLatin1String("few clouds")) || weather.contains(QLatin1String("mostly sunny"))
               || weather.contains(QLatin1String("mostly clear")) || weather.contains(QLatin1String("increasing clouds"))
               || weather.contains(QLatin1String("becoming cloudy")) || weather.contains(QLatin1String("clearing"))
               || weather.contains(QLatin1String("decreasing clouds")) || weather.contains(QLatin1String("becoming sunny"))) {
        if (weather.contains(QLatin1String("breezy")) || weather.contains(QLatin1String("wind")) || weather.contains(QLatin1String("gust"))) {
            result = isDayTime ? IonInterface::FewCloudsWindyDay : IonInterface::FewCloudsWindyNight;
        } else {
            result = isDayTime ? IonInterface::FewCloudsDay : IonInterface::FewCloudsNight;
        }

    } else if (weather.contains(QLatin1String("partly cloudy")) || weather.contains(QLatin1String("partly sunny"))
               || weather.contains(QLatin1String("partly clear"))) {
        if (weather.contains(QLatin1String("breezy")) || weather.contains(QLatin1String("wind")) || weather.contains(QLatin1String("gust"))) {
            result = isDayTime ? IonInterface::PartlyCloudyWindyDay : IonInterface::PartlyCloudyWindyNight;
        } else {
            result = isDayTime ? IonInterface::PartlyCloudyDay : IonInterface::PartlyCloudyNight;
        }

    } else if (weather.contains(QLatin1String("overcast")) || weather.contains(QLatin1String("cloudy"))) {
        if (weather.contains(QLatin1String("breezy")) || weather.contains(QLatin1String("wind")) || weather.contains(QLatin1String("gust"))) {
            result = IonInterface::OvercastWindy;
        } else {
            result = IonInterface::Overcast;
        }

    } else if (weather.contains(QLatin1String("haze")) || weather.contains(QLatin1String("smoke")) || weather.contains(QLatin1String("dust"))
               || weather.contains(QLatin1String("sand"))) {
        result = IonInterface::Haze;

    } else if (weather.contains(QLatin1String("fair")) || weather.contains(QLatin1String("clear")) || weather.contains(QLatin1String("sunny"))) {
        if (weather.contains(QLatin1String("breezy")) || weather.contains(QLatin1String("wind")) || weather.contains(QLatin1String("gust"))) {
            result = isDayTime ? IonInterface::ClearWindyDay : IonInterface::ClearWindyNight;
        } else {
            result = isDayTime ? IonInterface::ClearDay : IonInterface::ClearNight;
        }

    } else if (weather.contains(QLatin1String("fog"))) {
        result = IonInterface::Mist;

    } else if (weather.contains(QLatin1String("hot"))) {
        // temperature condition has not hint about air ingredients, so let's assume the sky is clear when it is hot
        if (weather.contains(QLatin1String("breezy")) || weather.contains(QLatin1String("wind")) || weather.contains(QLatin1String("gust"))) {
            result = isDayTime ? IonInterface::ClearWindyDay : IonInterface::ClearWindyNight;
        } else {
            result = isDayTime ? IonInterface::ClearDay : IonInterface::ClearNight;
        }

    } else if (weather.contains(QLatin1String("breezy")) || weather.contains(QLatin1String("wind")) || weather.contains(QLatin1String("gust"))) {
        // Assume a clear sky when it's windy but no clouds have been mentioned
        result = isDayTime ? IonInterface::ClearWindyDay : IonInterface::ClearWindyNight;
    } else {
        result = IonInterface::NotAvailable;
    }

    return result;
}

void NOAAIon::getForecast(const QString &source)
{
    const double lat = m_weatherData[source].stationLatitude;
    const double lon = m_weatherData[source].stationLongitude;
    if (qIsNaN(lat) || qIsNaN(lon)) {
        return;
    }

    /* Assuming that we have the latitude and longitude data at this point, get the 7-day forecast.
     * The provider is more likely to reject requests with more than 3 decimal digits precision
     */
    const QUrl url("https://graphical.weather.gov/xml/sample_products/browser_interface/ndfdBrowserClientByDay.php?lat=%1&lon=%2&format=24+hourly&numDays=7"_L1
                       .arg(QString::number(lat, 'f', 3))
                       .arg(QString::number(lon, 'f', 3)));

    auto getJob = apiRequestJob(url, source);
    connect(getJob, &KJob::result, this, &NOAAIon::forecast_slotJobFinished);
}

void NOAAIon::forecast_slotJobFinished(KJob *job)
{
    QXmlStreamReader reader = QXmlStreamReader(m_jobData.value(job));
    const QString source = m_jobList.value(job);

    if (!reader.atEnd()) {
        readForecast(source, reader);
        updateWeather(source);
    }

    m_jobList.remove(job);
    m_jobData.remove(job);

    if (m_sourcesToReset.contains(source)) {
        m_sourcesToReset.removeAll(source);

        // so the weather engine updates it's data
        forceImmediateUpdateOfAllVisualizations();

        // update the clients of our engine
        Q_EMIT forceUpdate(this, source);
    }
}

void NOAAIon::readForecast(const QString &source, QXmlStreamReader &xml)
{
    WeatherData &weatherData = m_weatherData[source];
    QList<WeatherData::Forecast> &forecasts = weatherData.forecasts;

    // Clear the current forecasts
    forecasts.clear();

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            /* Read all reported days from <time-layout>. We check for existence of a specific
             * <layout-key> which indicates the separate day listings.  The schema defines it to be
             * the first item before the day listings.
             */
            if (xml.name() == "layout-key"_L1 && xml.readElementText().startsWith("k-p24h"_L1)) {
                // Read days until we get to end of parent (<time-layout>)tag
                while (!(xml.isEndElement() && xml.name() == QLatin1String("time-layout"))) {
                    xml.readNext();

                    if (xml.name() == QLatin1String("start-valid-time")) {
                        QString data = xml.readElementText();
                        QDateTime date = QDateTime::fromString(data, Qt::ISODate);

                        WeatherData::Forecast forecast;
                        forecast.day = QLocale().toString(date.date().day());
                        forecasts.append(forecast);
                        // qCDebug(IONENGINE_NOAA) << forecast.day;
                    }
                }

            } else if (xml.name() == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("type")) == QLatin1String("maximum")) {
                // Read max temps until we get to end tag
                int i = 0;
                while (!(xml.isEndElement() && xml.name() == QLatin1String("temperature")) && i < forecasts.count()) {
                    xml.readNext();

                    if (xml.name() == QLatin1String("value")) {
                        forecasts[i].high = xml.readElementText();
                        // qCDebug(IONENGINE_NOAA) << forecasts[i].high;
                        i++;
                    }
                }
            } else if (xml.name() == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("type")) == QLatin1String("minimum")) {
                // Read min temps until we get to end tag
                int i = 0;
                while (!(xml.isEndElement() && xml.name() == QLatin1String("temperature")) && i < forecasts.count()) {
                    xml.readNext();

                    if (xml.name() == QLatin1String("value")) {
                        forecasts[i].low = xml.readElementText();
                        // qCDebug(IONENGINE_NOAA) << forecasts[i].low;
                        i++;
                    }
                }
            } else if (xml.name() == QLatin1String("probability-of-precipitation")) {
                // Precipitation is usually provided in 12-hour periods instead of 24.
                int periodHours = xml.attributes().value(QStringLiteral("type")).split(QLatin1Char(' '))[0].toInt();
                if (!periodHours) {
                    periodHours = 24;
                }

                int i = 0;
                int hours = 0;
                while (!(xml.isEndElement() && xml.name() == QLatin1String("weather")) && i < forecasts.count()) {
                    xml.readNext();
                    if (xml.name() != QLatin1String("value")) {
                        continue;
                    }

                    const int probability = xml.readElementText().toInt();
                    forecasts[i].precipitation = qMax(probability, forecasts[i].precipitation);
                    hours += periodHours;

                    if (hours >= 24) {
                        hours = 0;
                        i++;
                    }
                }
            } else if (xml.name() == QLatin1String("weather")) {
                // Read weather conditions until we get to end tag
                int i = 0;
                while (!(xml.isEndElement() && xml.name() == QLatin1String("weather")) && i < forecasts.count()) {
                    xml.readNext();

                    if (xml.name() == QLatin1String("weather-conditions") && xml.isStartElement()) {
                        QString summary = xml.attributes().value(QStringLiteral("weather-summary")).toString();
                        forecasts[i].summary = summary;
                        // qCDebug(IONENGINE_NOAA) << forecasts[i].summary;
                        qCDebug(IONENGINE_NOAA) << "i18n summary string: " << i18nc("weather forecast", forecasts[i].summary.toUtf8().data());
                        i++;
                    }
                }
            } else if (xml.name() == "problem"_L1) {
                // We've received a server error
                const QString problem = xml.readElementText();
                qCWarning(IONENGINE_NOAA) << "Server error requesting forecast:" << problem;
            }
        }
    }

    weatherData.isForecastsDataPending = false;
}

void NOAAIon::getCountyID(const QString &source)
{
    const double lat = m_weatherData[source].stationLatitude;
    const double lon = m_weatherData[source].stationLongitude;
    if (qIsNaN(lat) || qIsNaN(lon)) {
        return;
    }

    const QUrl url(QStringLiteral("https://api.weather.gov/points/%1,%2").arg(lat).arg(lon));

    auto getJob = apiRequestJob(url, source);
    connect(getJob, &KJob::result, this, &NOAAIon::county_slotJobFinished);
}

void NOAAIon::county_slotJobFinished(KJob *job)
{
    const QString source = m_jobList.value(job);

    if (!job->error()) {
        QJsonDocument doc = QJsonDocument::fromJson(m_jobData.value(job));
        if (!doc.isEmpty()) {
            readCountyID(source, doc);
        }
    } else {
        qCWarning(IONENGINE_NOAA) << "Error getting coordinates info" << job->errorText();
    }

    m_jobList.remove(job);
    m_jobData.remove(job);
}

void NOAAIon::readCountyID(const QString &source, const QJsonDocument &doc)
{
    if (doc.isEmpty()) {
        return;
    }

    const auto properties = doc[QStringLiteral("properties")];
    if (!properties.isObject()) {
        return;
    }

    const QString countyUrl = properties[QStringLiteral("county")].toString();
    const QString countyID = countyUrl.split(QLatin1Char('/')).last();
    m_weatherData[source].countyID = countyID;

    getAlerts(source);
}

void NOAAIon::getAlerts(const QString &source)
{
    // We get the alerts by county because it includes all the events.
    // Using the forecast zone would miss some of them, and the lat/lon point
    // corresponds to the weather station, not necessarily the user location
    const QString countyID = m_weatherData[source].countyID;
    if (countyID.isEmpty()) {
        getCountyID(source);
        return;
    }

    const QUrl url(QStringLiteral("https://api.weather.gov/alerts/active?zone=%1").arg(countyID));

    auto getJob = apiRequestJob(url, source);
    connect(getJob, &KJob::result, this, &NOAAIon::alerts_slotJobFinished);
}

void NOAAIon::alerts_slotJobFinished(KJob *job)
{
    const QString source = m_jobList.value(job);

    if (!job->error()) {
        QJsonDocument doc = QJsonDocument::fromJson(m_jobData.value(job));
        if (!doc.isEmpty()) {
            readAlerts(source, doc);
        }
    } else {
        qCWarning(IONENGINE_NOAA) << "Error getting alerts info" << job->errorText();
    }

    m_jobList.remove(job);
    m_jobData.remove(job);
}

// Helpers to parse warnings
int mapSeverity(const QString &severity)
{
    if (severity == "Extreme"_L1) {
        return 4;
    } else if (severity == "Severe"_L1) {
        return 3;
    } else if (severity == "Moderate"_L1) {
        return 2;
    } else if (severity == "Minor"_L1) {
        return 1;
    } else { // severity: "Unknown"
        return 0;
    }
};

QString formatAlertDescription(QString description)
{
    /* -- Example of an alert's description --
    * WHAT...Minor flooding is occurring and minor flooding is forecast.\n
    \n
    * WHERE...Santee River near Jamestown.\n
    \n
    * WHEN...Until further notice.\n
    \n
    * IMPACTS...At 12.0 feet, several dirt logging roads are impassable.\n
    \n
    * ADDITIONAL DETAILS...\n
    - At 930 PM EST Tuesday, the stage was 11.4 feet.\n
    - Forecast...The river is expected to rise to a crest of 11.7\n
    feet Thursday evening.\n
    - Flood stage is 10.0 feet.\n
    */
    description.replace("* "_L1, "<b>"_L1);
    description.replace("..."_L1, ":</b> "_L1);
    description.replace("\n\n"_L1, "<br/>"_L1);
    description.replace("\n-"_L1, "<br/>-"_L1);
    return description;
}

void NOAAIon::readAlerts(const QString &source, const QJsonDocument &doc)
{
    if (doc.isEmpty()) {
        return;
    }

    auto &alerts = m_weatherData[source].alerts;
    alerts.clear();

    const auto features = doc[u"features"_s].toArray();
    qCDebug(IONENGINE_NOAA) << u"Received %1 alert/s"_s.arg(features.count());

    for (const auto &alertInfo : features) {
        const auto properties = alertInfo[u"properties"_s];
        if (!properties.isObject()) {
            continue;
        }

        auto alert = WeatherData::Alert();
        alert.startTime = QDateTime::fromString(properties[u"onset"_s].toString(), Qt::ISODate);
        alert.endTime = QDateTime::fromString(properties[u"ends"_s].toString(), Qt::ISODate);
        alert.priority = mapSeverity(properties[u"severity"_s].toString());
        alert.headline = properties[u"parameters"_s][u"NWSheadline"_s][0].toString();
        alert.description = formatAlertDescription(properties[u"description"_s].toString());

        alerts << alert;
    }

    // Sort by higher priority and the lower start time
    std::sort(alerts.begin(), alerts.end(), [](auto a, auto b) {
        if (a.priority != b.priority) {
            return a.priority > b.priority;
        }
        return a.startTime < b.startTime;
    });

    updateWeather(source);
    forceImmediateUpdateOfAllVisualizations();
    Q_EMIT forceUpdate(this, source);
}

void NOAAIon::dataUpdated(const QString &sourceName, const Plasma5Support::DataEngine::Data &data)
{
    const bool isNight = (data.value(QStringLiteral("Corrected Elevation")).toDouble() < 0.0);

    for (auto end = m_weatherData.end(), it = m_weatherData.begin(); it != end; ++it) {
        auto &weatherData = it.value();
        if (weatherData.solarDataTimeEngineSourceName == sourceName) {
            weatherData.isNight = isNight;
            weatherData.isSolarDataPending = false;
            updateWeather(it.key());
        }
    }
}

K_PLUGIN_CLASS_WITH_JSON(NOAAIon, "ion-noaa.json")

#include "ion_noaa.moc"
