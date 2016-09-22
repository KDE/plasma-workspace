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

#include "ion_noaa.h"

#include "ion_noaadebug.h"

#include <KIO/Job>
#include <KUnitConversion/Converter>
#include <KLocalizedString>

#include <QLocale>


WeatherData::WeatherData()
  : stationLat(qQNaN())
  , stationLon(qQNaN())
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
    QMap<QString, WindDirections> windDir;
    windDir[QStringLiteral("north")] = N;
    windDir[QStringLiteral("northeast")] = NE;
    windDir[QStringLiteral("south")] = S;
    windDir[QStringLiteral("southwest")] = SW;
    windDir[QStringLiteral("east")] = E;
    windDir[QStringLiteral("southeast")] = SE;
    windDir[QStringLiteral("west")] = W;
    windDir[QStringLiteral("northwest")] = NW;
    windDir[QStringLiteral("calm")] = VR;
    return windDir;
}

QMap<QString, IonInterface::ConditionIcons> NOAAIon::setupConditionIconMappings() const
{

    QMap<QString, ConditionIcons> conditionList;
    return conditionList;
}

QMap<QString, IonInterface::ConditionIcons> const& NOAAIon::conditionIcons() const
{
    static QMap<QString, ConditionIcons> const condval = setupConditionIconMappings();
    return condval;
}

QMap<QString, IonInterface::WindDirections> const& NOAAIon::windIcons() const
{
    static QMap<QString, WindDirections> const wval = setupWindIconMappings();
    return wval;
}

// ctor, dtor
NOAAIon::NOAAIon(QObject *parent, const QVariantList &args)
        : IonInterface(parent, args)
{
    // Get the real city XML URL so we can parse this
    getXMLSetup();

    // not used while daytime not considered, see below
    // m_timeEngine = dataEngine(QStringLiteral("time"));
}

void NOAAIon::reset()
{
    m_sourcesToReset = sources();
    getXMLSetup();
}

NOAAIon::~NOAAIon()
{
    //seems necessary to avoid crash
    removeAllSources();
}

QStringList NOAAIon::validate(const QString& source) const
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

bool NOAAIon::updateIonSource(const QString& source)
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

// Parses city list and gets the correct city based on ID number
void NOAAIon::getXMLSetup() const
{
    const QUrl url(QStringLiteral("http://www.weather.gov/data/current_obs/index.xml"));

    KIO::TransferJob* getJob = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);

    connect(getJob, &KIO::TransferJob::data,
            this, &NOAAIon::setup_slotDataArrived);
    connect(getJob, &KJob::result,
            this, &NOAAIon::setup_slotJobFinished);
}

// Gets specific city XML data
void NOAAIon::getXMLData(const QString& source)
{
    foreach (const QString &fetching, m_jobList) {
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

    KIO::TransferJob* getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    m_jobXml.insert(getJob, new QXmlStreamReader);
    m_jobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data,
            this, &NOAAIon::slotDataArrived);
    connect(getJob, &KJob::result,
            this, &NOAAIon::slotJobFinished);
}

void NOAAIon::setup_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)

    if (data.isEmpty()) {
        return;
    }

    // Send to xml.
    m_xmlSetup.addData(data);
}

void NOAAIon::slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    if (data.isEmpty() || !m_jobXml.contains(job)) {
        return;
    }

    // Send to xml.
    m_jobXml[job]->addData(data);
}

void NOAAIon::slotJobFinished(KJob *job)
{
    // Dual use method, if we're fetching location data to parse we need to do this first
    const QString source(m_jobList.value(job));
    removeAllData(source);
    QXmlStreamReader *reader = m_jobXml.value(job);
    if (reader) {
        readXMLData(m_jobList[job], *reader);
    }

    // Now that we have the longitude and latitude, fetch the seven day forecast.
    getForecast(m_jobList[job]);

    m_jobList.remove(job);
    m_jobXml.remove(job);
    delete reader;
}

void NOAAIon::setup_slotJobFinished(KJob *job)
{
    Q_UNUSED(job)
    const bool success = readXMLSetup();
    setInitialized(success);

    foreach (const QString &source, m_sourcesToReset) {
        updateSourceEvent(source);
    }
}

void NOAAIon::parseFloat(float& value, const QString& string)
{
    bool ok = false;
    const float result = string.toFloat(&ok);
    if (ok) {
        value = result;
    }
}

void NOAAIon::parseFloat(float& value, QXmlStreamReader& xml)
{
    bool ok = false;
    const float result = xml.readElementText().toFloat(&ok);
    if (ok) {
        value = result;
    }
}

void NOAAIon::parseDouble(double& value, QXmlStreamReader& xml)
{
    bool ok = false;
    const double result = xml.readElementText().toDouble(&ok);
    if (ok) {
        value = result;
    }
}

void NOAAIon::parseStationID()
{
    QString state;
    QString stationName;
    QString stationID;
    QString xmlurl;

    while (!m_xmlSetup.atEnd()) {
        m_xmlSetup.readNext();

        const QStringRef elementName = m_xmlSetup.name();

        if (m_xmlSetup.isEndElement() && elementName == QLatin1String("station")) {
            if (!xmlurl.isEmpty()) {
                NOAAIon::XMLMapInfo info;
                info.stateName = state;
                info.stationName = stationName;
                info.stationID = stationID;
                info.XMLurl = xmlurl;

                QString tmp = stationName + QStringLiteral(", ") + state; // Build the key name.
                m_places[tmp] = info;
            }
            break;
        }

        if (m_xmlSetup.isStartElement()) {
            if (elementName == QLatin1String("station_id")) {
                stationID = m_xmlSetup.readElementText();
            } else if (elementName == QLatin1String("state")) {
                state = m_xmlSetup.readElementText();
            } else if (elementName == QLatin1String("station_name")) {
                stationName = m_xmlSetup.readElementText();
            } else if (elementName == QLatin1String("xml_url")) {
                xmlurl = m_xmlSetup.readElementText().replace(QStringLiteral("http://"), QStringLiteral("http://www."));
            } else {
                parseUnknownElement(m_xmlSetup);
            }
        }
    }
}

void NOAAIon::parseStationList()
{
    while (!m_xmlSetup.atEnd()) {
        m_xmlSetup.readNext();

        if (m_xmlSetup.isEndElement()) {
            break;
        }

        if (m_xmlSetup.isStartElement()) {
            if (m_xmlSetup.name() == QLatin1String("station")) {
                parseStationID();
            } else {
                parseUnknownElement(m_xmlSetup);
            }
        }
    }
}

// Parse the city list and store into a QMap
bool NOAAIon::readXMLSetup()
{
    bool success = false;
    while (!m_xmlSetup.atEnd()) {
        m_xmlSetup.readNext();

        if (m_xmlSetup.isStartElement()) {
            if (m_xmlSetup.name() == QLatin1String("wx_station_index")) {
                parseStationList();
                success = true;
            }
        }
    }
    return (!m_xmlSetup.error() && success);
}

void NOAAIon::parseWeatherSite(WeatherData& data, QXmlStreamReader& xml)
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

        const QStringRef elementName = xml.name();

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("location")) {
                data.locationName = xml.readElementText();
            } else if (elementName == QLatin1String("station_id")) {
                data.stationID = xml.readElementText();
            } else if (elementName == QLatin1String("latitude")) {
                parseDouble(data.stationLat, xml);
            } else if (elementName == QLatin1String("longitude")) {
                parseDouble(data.stationLon, xml);
            } else if (elementName == QLatin1String("observation_time")) {
                data.observationTime = xml.readElementText();
                QStringList tmpDateStr = data.observationTime.split(QLatin1Char(' '));
                data.observationTime = QStringLiteral("%1 %2").arg(tmpDateStr[6], tmpDateStr[7]);
                m_dateFormat = QDateTime::fromString(data.observationTime, QStringLiteral("h:mm ap"));
                data.iconPeriodHour = m_dateFormat.toString(QStringLiteral("HH"));
                data.iconPeriodAP = m_dateFormat.toString(QStringLiteral("ap"));

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

// Parse Weather data main loop, from here we have to decend into each tag pair
bool NOAAIon::readXMLData(const QString& source, QXmlStreamReader& xml)
{
    WeatherData data;

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

    m_weatherData[source] = data;
    return !xml.error();
}

// handle when no XML tag is found
void NOAAIon::parseUnknownElement(QXmlStreamReader& xml) const
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

void NOAAIon::updateWeather(const QString& source)
{
    const WeatherData& weatherData = m_weatherData[source];

    Plasma::DataEngine::Data data;

    data.insert(QStringLiteral("Country"), QStringLiteral("USA"));
    data.insert(QStringLiteral("Place"), weatherData.locationName);
    data.insert(QStringLiteral("Station"), weatherData.stationID);

    const double lat = weatherData.stationLat;
    const double lon = weatherData.stationLon;
    if (!qIsNaN(lat) && !qIsNaN(lon)) {
        data.insert(QStringLiteral("Latitude"), lat);
        data.insert(QStringLiteral("Longitude"), lon);
    }

    // Real weather - Current conditions
    data.insert(QStringLiteral("Observation Period"), weatherData.observationTime);
    const QString conditionI18n = weatherData.weather == QLatin1String("N/A") ? i18n("N/A") : i18nc("weather condition", weatherData.weather.toUtf8().data());

    data.insert(QStringLiteral("Current Conditions"), conditionI18n);
    qCDebug(IONENGINE_NOAA) << "i18n condition string: " << qPrintable(conditionI18n);

//TODO: Port to Plasma2
#if 0
    // Determine the weather icon based on the current time and computed sunrise/sunset time.
    const Plasma::DataEngine::Data timeData = m_timeEngine->query(
            QString("Local|Solar|Latitude=%1|Longitude=%2")
                .arg(latitude(source)).arg(longitude(source)));

    QTime sunriseTime = timeData["Sunrise"].toDateTime().time();
    QTime sunsetTime = timeData["Sunset"].toDateTime().time();
    QTime currentTime = QDateTime::currentDateTime().time();

    // Provide mapping for the condition-type to the icons to display
    if (currentTime > sunriseTime && currentTime < sunsetTime) {
#endif
        // Day
        QString weather = weatherData.weather.toLower();
        ConditionIcons condition = getConditionIcon(weather, true);
        data.insert(QStringLiteral("Condition Icon"), getWeatherIcon(condition));
        qCDebug(IONENGINE_NOAA) << "Using daytime icons\n";
#if 0
    } else {
        // Night
        QString weather = weatherData.weather.toLower();
        ConditionIcons condition = getConditionIcon(weather, false);
        data.insert("Condition Icon", getWeatherIcon(condition));
        qCDebug(IONENGINE_NOAA) << "Using nighttime icons\n";
    }
#endif

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
        data.insert(QStringLiteral("Wind Direction"),
                    getWindDirectionIcon(windIcons(), weatherData.windDirection.toLower()));
    }

    // Set number of forecasts per day/night supported
    data.insert(QStringLiteral("Total Weather Days"), weatherData.forecasts.size());

    int i = 0;
    foreach(const WeatherData::Forecast &forecast, weatherData.forecasts) {

        ConditionIcons icon = getConditionIcon(forecast.summary.toLower(), true);
        QString iconName = getWeatherIcon(icon);

        /* Sometimes the forecast for the later days is unavailable, if so skip remianing days
         * since their forecast data is probably unavailable.
         */
        if (forecast.low.isEmpty() || forecast.high.isEmpty()) {
            break;
        }

        // Get the short day name for the forecast
        data.insert(QStringLiteral("Short Forecast Day %1").arg(i),
                    QStringLiteral("%1|%2|%3|%4|%5|%6")
                    .arg(forecast.day)
                    .arg(iconName)
                    .arg(i18nc("weather forecast", forecast.summary.toUtf8().data()))
                    .arg(forecast.high)
                    .arg(forecast.low)
                    .arg(QStringLiteral()));
        ++i;
    }

    data.insert(QStringLiteral("Credit"), i18nc("credit line, keep string short)", "Data from NOAA National\302\240Weather\302\240Service"));

    setData(source, data);
}

/**
  * Determine the condition icon based on the list of possible NOAA weather conditions as defined at
  * <http://www.weather.gov/xml/current_obs/weather.php> and <http://www.weather.gov/mdl/XML/Design/MDL_XML_Design.htm#_Toc141760783>
  * Since the number of NOAA weather conditions need to be fitted into the narowly defined groups in IonInterface::ConditionIcons, we
  * try to group the NOAA conditions as best as we can based on their priorities/severity.
  */
IonInterface::ConditionIcons NOAAIon::getConditionIcon(const QString& weather, bool isDayTime) const
{
    IonInterface::ConditionIcons result;
    // Consider any type of storm, tornado or funnel to be a thunderstorm.
    if (weather.contains(QStringLiteral("thunderstorm")) || weather.contains(QStringLiteral("funnel")) ||
        weather.contains(QStringLiteral("tornado")) || weather.contains(QStringLiteral("storm")) || weather.contains(QStringLiteral("tstms"))) {

        if (weather.contains(QStringLiteral("vicinity")) || weather.contains(QStringLiteral("chance"))) {
            result = isDayTime ? IonInterface::ChanceThunderstormDay : IonInterface::ChanceThunderstormNight;
        } else {
            result = IonInterface::Thunderstorm;
        }

    } else if (weather.contains(QStringLiteral("pellets")) || weather.contains(QStringLiteral("crystals")) ||
             weather.contains(QStringLiteral("hail"))) {
        result = IonInterface::Hail;

    } else if (((weather.contains(QStringLiteral("rain")) || weather.contains(QStringLiteral("drizzle")) ||
              weather.contains(QStringLiteral("showers"))) && weather.contains(QStringLiteral("snow"))) || weather.contains(QStringLiteral("wintry mix"))) {
        result = IonInterface::RainSnow;

    } else if (weather.contains(QStringLiteral("snow")) && weather.contains(QStringLiteral("light"))) {
        result = IonInterface::LightSnow;

    } else if (weather.contains(QStringLiteral("snow"))) {
        if (weather.contains(QStringLiteral("vicinity")) || weather.contains(QStringLiteral("chance"))) {
            result = isDayTime ? IonInterface::ChanceSnowDay : IonInterface::ChanceSnowNight;
        } else {
            result = IonInterface::Snow;
        }

    } else if (weather.contains(QStringLiteral("freezing rain"))) {
        result = IonInterface::FreezingRain;

    } else if (weather.contains(QStringLiteral("freezing drizzle"))) {
        result = IonInterface::FreezingDrizzle;

    } else if (weather.contains(QStringLiteral("showers"))) {

        if (weather.contains(QStringLiteral("vicinity")) || weather.contains(QStringLiteral("chance"))) {
            result = isDayTime ? IonInterface::ChanceShowersDay : IonInterface::ChanceShowersNight;
        } else {
            result = IonInterface::Showers;
        }
    } else if (weather.contains(QStringLiteral("light rain")) || weather.contains(QStringLiteral("drizzle"))) {
        result = IonInterface::LightRain;

    } else if (weather.contains(QStringLiteral("rain"))) {
        result = IonInterface::Rain;

    } else if (weather.contains(QStringLiteral("few clouds")) || weather.contains(QStringLiteral("mostly sunny")) ||
               weather.contains(QStringLiteral("mostly clear")) || weather.contains(QStringLiteral("increasing clouds")) ||
               weather.contains(QStringLiteral("becoming cloudy")) || weather.contains(QStringLiteral("clearing")) ||
               weather.contains(QStringLiteral("decreasing clouds")) || weather.contains(QStringLiteral("becoming sunny"))) {
        result = isDayTime ? IonInterface::FewCloudsDay : IonInterface::FewCloudsNight;

    } else if (weather.contains(QStringLiteral("partly cloudy")) || weather.contains(QStringLiteral("partly sunny")) ||
               weather.contains(QStringLiteral("partly clear"))) {
        result = isDayTime ? IonInterface::PartlyCloudyDay : IonInterface::PartlyCloudyNight;

    } else if (weather.contains(QStringLiteral("overcast")) || weather.contains(QStringLiteral("cloudy"))) {
        result = IonInterface::Overcast;

    } else if (weather.contains(QStringLiteral("haze")) || weather.contains(QStringLiteral("smoke")) ||
             weather.contains(QStringLiteral("dust")) || weather.contains(QStringLiteral("sand"))) {
        result = IonInterface::Haze;

    } else if (weather.contains(QStringLiteral("fair")) || weather.contains(QStringLiteral("clear")) || weather.contains(QStringLiteral("sunny"))) {
        result = isDayTime ? IonInterface::ClearDay : IonInterface::ClearNight;

    } else if (weather.contains(QStringLiteral("fog"))) {
        result = IonInterface::Mist;

    } else {
        result = IonInterface::NotAvailable;
    }

    return result;
}

void NOAAIon::getForecast(const QString& source)
{
    const double lat = m_weatherData[source].stationLat;
    const double lon = m_weatherData[source].stationLon;
    if (qIsNaN(lat) || qIsNaN(lon)) {
        return;
    }

    /* Assuming that we have the latitude and longitude data at this point, get the 7-day
     * forecast.
     */
    const QUrl url(QLatin1String("http://www.weather.gov/forecasts/xml/sample_products/browser_interface/"
                                 "ndfdBrowserClientByDay.php?lat=") + QString::number(lat) +
                                 QLatin1String("&lon=") + QString::number(lon) +
                                 QLatin1String("&format=24+hourly&numDays=7"));

    KIO::TransferJob* getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    m_jobXml.insert(getJob, new QXmlStreamReader);
    m_jobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data,
            this, &NOAAIon::forecast_slotDataArrived);
    connect(getJob, &KJob::result,
            this, &NOAAIon::forecast_slotJobFinished);
}

void NOAAIon::forecast_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    if (data.isEmpty() || !m_jobXml.contains(job)) {
        return;
    }

    // Send to xml.
    m_jobXml[job]->addData(data);
}

void NOAAIon::forecast_slotJobFinished(KJob *job)
{
    QXmlStreamReader *reader = m_jobXml.value(job);
    const QString source = m_jobList.value(job);

    if (reader) {
        readForecast(source, *reader);
        updateWeather(source);
    }

    m_jobList.remove(job);
    delete m_jobXml[job];
    m_jobXml.remove(job);

    if (m_sourcesToReset.contains(source)) {
        m_sourcesToReset.removeAll(source);

        // so the weather engine updates it's data
        forceImmediateUpdateOfAllVisualizations();

        // update the clients of our engine
        emit forceUpdate(this, source);
    }
}

void NOAAIon::readForecast(const QString& source, QXmlStreamReader& xml)
{
    QList<WeatherData::Forecast>& forecasts = m_weatherData[source].forecasts;

    // Clear the current forecasts
    forecasts.clear();

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {

            /* Read all reported days from <time-layout>. We check for existence of a specific
             * <layout-key> which indicates the separate day listings.  The schema defines it to be
             * the first item before the day listings.
             */
            if (xml.name() == QLatin1String("layout-key") && xml.readElementText() == QLatin1String("k-p24h-n7-1")) {

                // Read days until we get to end of parent (<time-layout>)tag
                while (! (xml.isEndElement() && xml.name() == QLatin1String("time-layout"))) {

                    xml.readNext();

                    if (xml.name() == QLatin1String("start-valid-time")) {
                        QString data = xml.readElementText();
                        QDateTime date = QDateTime::fromString(data, Qt::ISODate);

                        WeatherData::Forecast forecast;
                        forecast.day = QLocale().toString(date.date().day());
                        forecasts.append(forecast);
                        //qCDebug(IONENGINE_NOAA) << forecast.day;
                    }
                }

            } else if (xml.name() == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("type")) == QLatin1String("maximum")) {

                // Read max temps until we get to end tag
                int i = 0;
                while (! (xml.isEndElement() && xml.name() == QLatin1String("temperature")) &&
                       i < forecasts.count()) {

                    xml.readNext();

                    if (xml.name() == QLatin1String("value")) {
                        forecasts[i].high = xml.readElementText();
                        //qCDebug(IONENGINE_NOAA) << forecasts[i].high;
                        i++;
                    }
                }
            } else if (xml.name() == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("type")) == QLatin1String("minimum")) {

                // Read min temps until we get to end tag
                int i = 0;
                while (! (xml.isEndElement() && xml.name() == QLatin1String("temperature")) &&
                       i < forecasts.count()) {

                    xml.readNext();

                    if (xml.name() == QLatin1String("value")) {
                        forecasts[i].low = xml.readElementText();
                        //qCDebug(IONENGINE_NOAA) << forecasts[i].low;
                        i++;
                    }
                }
            } else if (xml.name() == QLatin1String("weather")) {

                // Read weather conditions until we get to end tag
                int i = 0;
                while (! (xml.isEndElement() && xml.name() == QLatin1String("weather")) &&
                       i < forecasts.count()) {

                    xml.readNext();

                    if (xml.name() == QLatin1String("weather-conditions") && xml.isStartElement()) {
                        QString summary = xml.attributes().value(QStringLiteral("weather-summary")).toString();
                        forecasts[i].summary = summary;
                        //qCDebug(IONENGINE_NOAA) << forecasts[i].summary;
			qCDebug(IONENGINE_NOAA) << "i18n summary string: "
                                 << i18nc("weather forecast", forecasts[i].summary.toUtf8().data());
                        i++;
                    }
                }
            }
        }
    }
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(noaa, NOAAIon, "ion-noaa.json")

#include "ion_noaa.moc"
