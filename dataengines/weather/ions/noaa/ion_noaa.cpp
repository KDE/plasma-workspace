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

#include <KIO/Job>
#include <KLocalizedDate>

#include <KUnitConversion/Converter>

QMap<QString, IonInterface::WindDirections> NOAAIon::setupWindIconMappings(void) const
{
    QMap<QString, WindDirections> windDir;
    windDir["north"] = N;
    windDir["northeast"] = NE;
    windDir["south"] = S;
    windDir["southwest"] = SW;
    windDir["east"] = E;
    windDir["southeast"] = SE;
    windDir["west"] = W;
    windDir["northwest"] = NW;
    windDir["calm"] = VR;
    return windDir;
}

QMap<QString, IonInterface::ConditionIcons> NOAAIon::setupConditionIconMappings(void) const
{

    QMap<QString, ConditionIcons> conditionList;
    return conditionList;
}

QMap<QString, IonInterface::ConditionIcons> const& NOAAIon::conditionIcons(void) const
{
    static QMap<QString, ConditionIcons> const condval = setupConditionIconMappings();
    return condval;
}

QMap<QString, IonInterface::WindDirections> const& NOAAIon::windIcons(void) const
{
    static QMap<QString, WindDirections> const wval = setupWindIconMappings();
    return wval;
}

// ctor, dtor
NOAAIon::NOAAIon(QObject *parent, const QVariantList &args)
        : IonInterface(parent, args)
{
    Q_UNUSED(args)
}

void NOAAIon::reset()
{
    m_sourcesToReset = sources();
    getXMLSetup();
}

NOAAIon::~NOAAIon()
{
}

// Get the master list of locations to be parsed
void NOAAIon::init()
{
    // Get the real city XML URL so we can parse this
    getXMLSetup();

    m_timeEngine = dataEngine("time");
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
                placeList.append(QString("place|").append(it.key()));
            }
        } else if (it.key().toUpper().contains(sourceNormalized)) {
            placeList.append(QString("place|").append(it.key()));
        } else if (it.value().stationID == sourceNormalized) {
            station = QString("place|").append(it.key());
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

    QStringList sourceAction = source.split('|');

    // Guard: if the size of array is not 2 then we have bad data, return an error
    if (sourceAction.size() < 2) {
        setData(source, "validate", "noaa|malformed");
        return true;
    }

    if (sourceAction[1] == "validate" && sourceAction.size() > 2) {
        QStringList result = validate(sourceAction[2]);

        if (result.size() == 1) {
            setData(source, "validate", QString("noaa|valid|single|").append(result.join("|")));
            return true;
        } else if (result.size() > 1) {
            setData(source, "validate", QString("noaa|valid|multiple|").append(result.join("|")));
            return true;
        } else if (result.size() == 0) {
            setData(source, "validate", QString("noaa|invalid|single|").append(sourceAction[2]));
            return true;
        }
    } else if (sourceAction[1] == "weather" && sourceAction.size() > 2) {
        getXMLData(source);
        return true;
    } else {
        setData(source, "validate", "noaa|malformed");
        return true;
    }

    return false;
}

// Parses city list and gets the correct city based on ID number
void NOAAIon::getXMLSetup() const
{
    KIO::TransferJob *job = KIO::get(KUrl("http://www.weather.gov/data/current_obs/index.xml"), KIO::NoReload, KIO::HideProgressInfo);

    if (job) {
        connect(job, SIGNAL(data(KIO::Job*,QByteArray)), this,
                SLOT(setup_slotDataArrived(KIO::Job*,QByteArray)));
        connect(job, SIGNAL(result(KJob*)), this, SLOT(setup_slotJobFinished(KJob*)));
    } else {
        qDebug() << "Could not create place name list transfer job";
    }
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
    dataKey.remove("noaa|weather|");
    KUrl url = m_places[dataKey].XMLurl;

    // If this is empty we have no valid data, send out an error and abort.
    if (url.url().isEmpty()) {
        setData(source, "validate", QString("noaa|malformed"));
        return;
    }

    KIO::TransferJob * const m_job = KIO::get(url.url(), KIO::Reload, KIO::HideProgressInfo);
    m_jobXml.insert(m_job, new QXmlStreamReader);
    m_jobList.insert(m_job, source);

    if (m_job) {
        connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this,
                SLOT(slotDataArrived(KIO::Job*,QByteArray)));
        connect(m_job, SIGNAL(result(KJob*)), this, SLOT(slotJobFinished(KJob*)));
    }
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

void NOAAIon::parseStationID()
{
    QString state;
    QString stationName;
    QString stationID;
    QString xmlurl;

    while (!m_xmlSetup.atEnd()) {
        m_xmlSetup.readNext();

        if (m_xmlSetup.isEndElement() && m_xmlSetup.name() == "station") {
            if (!xmlurl.isEmpty()) {
                NOAAIon::XMLMapInfo info;
                info.stateName = state;
                info.stationName = stationName;
                info.stationID = stationID;
                info.XMLurl = xmlurl;

                QString tmp = stationName + ", " + state; // Build the key name.
                m_places[tmp] = info;
            }
            break;
        }

        if (m_xmlSetup.isStartElement()) {
            if (m_xmlSetup.name() == "station_id") {
                stationID = m_xmlSetup.readElementText();
            } else if (m_xmlSetup.name() == "state") {
                state = m_xmlSetup.readElementText();
            } else if (m_xmlSetup.name() == "station_name") {
                stationName = m_xmlSetup.readElementText();
            } else if (m_xmlSetup.name() == "xml_url") {
                xmlurl = m_xmlSetup.readElementText().replace("http://", "http://www.");
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
            if (m_xmlSetup.name() == "station") {
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
            if (m_xmlSetup.name() == "wx_station_index") {
                parseStationList();
                success = true;
            }
        }
    }
    return (!m_xmlSetup.error() && success);
}

void NOAAIon::parseWeatherSite(WeatherData& data, QXmlStreamReader& xml)
{
    data.temperature_C = i18n("N/A");
    data.temperature_F = i18n("N/A");
    data.dewpoint_C = i18n("N/A");
    data.dewpoint_F = i18n("N/A");
    data.weather = i18n("N/A");
    data.stationID = i18n("N/A");
    data.pressure = i18n("N/A");
    data.visibility = i18n("N/A");
    data.humidity = i18n("N/A");
    data.windSpeed = i18n("N/A");
    data.windGust = i18n("N/A");
    data.windchill_F = i18n("N/A");
    data.windchill_C = i18n("N/A");
    data.heatindex_F = i18n("N/A");
    data.heatindex_C = i18n("N/A");

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            if (xml.name() == "location") {
                data.locationName = xml.readElementText();
            } else if (xml.name() == "station_id") {
                data.stationID = xml.readElementText();
            } else if (xml.name() == "latitude") {
                data.stationLat = xml.readElementText();
            } else if (xml.name() == "longitude") {
                data.stationLon = xml.readElementText();
            } else if (xml.name() == "observation_time") {
                data.observationTime = xml.readElementText();
                QStringList tmpDateStr = data.observationTime.split(' ');
                data.observationTime = QString("%1 %2").arg(tmpDateStr[6]).arg(tmpDateStr[7]);
                m_dateFormat = QDateTime::fromString(data.observationTime, "h:mm ap");
                data.iconPeriodHour = m_dateFormat.toString("HH");
                data.iconPeriodAP = m_dateFormat.toString("ap");

            } else if (xml.name() == "weather") {
                data.weather = xml.readElementText();
                // Pick which icon set depending on period of day
            } else if (xml.name() == "temp_f") {
                data.temperature_F = xml.readElementText();
            } else if (xml.name() == "temp_c") {
                data.temperature_C = xml.readElementText();
            } else if (xml.name() == "relative_humidity") {
                data.humidity = xml.readElementText();
            } else if (xml.name() == "wind_dir") {
                data.windDirection = xml.readElementText();
            } else if (xml.name() == "wind_mph") {
                data.windSpeed = xml.readElementText();
            } else if (xml.name() == "wind_gust_mph") {
                data.windGust = xml.readElementText();
            } else if (xml.name() == "pressure_in") {
                data.pressure = xml.readElementText();
            } else if (xml.name() == "dewpoint_f") {
                data.dewpoint_F = xml.readElementText();
            } else if (xml.name() == "dewpoint_c") {
                data.dewpoint_C = xml.readElementText();
            } else if (xml.name() == "heat_index_f") {
                data.heatindex_F = xml.readElementText();
            } else if (xml.name() == "heat_index_c") {
                data.heatindex_C = xml.readElementText();
            } else if (xml.name() == "windchill_f") {
                data.windchill_F = xml.readElementText();
            } else if (xml.name() == "windchill_c") {
                data.windchill_C = xml.readElementText();
            } else if (xml.name() == "visibility_mi") {
                data.visibility = xml.readElementText();
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
            if (xml.name() == "current_observation") {
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
    QMap<QString, QString> dataFields;
    Plasma::DataEngine::Data data;

    data.insert("Country", country(source));
    data.insert("Place", place(source));
    data.insert("Station", station(source));

    data.insert("Latitude", latitude(source));
    data.insert("Longitude", longitude(source));

    // Real weather - Current conditions
    data.insert("Observation Period", observationTime(source));
    data.insert("Current Conditions", conditionI18n(source));
    qDebug() << "i18n condition string: " << qPrintable(conditionI18n(source));

    // Determine the weather icon based on the current time and computed sunrise/sunset time.
    const Plasma::DataEngine::Data timeData = m_timeEngine->query(
            QString("Local|Solar|Latitude=%1|Longitude=%2")
                .arg(latitude(source)).arg(longitude(source)));

    QTime sunriseTime = timeData["Sunrise"].toDateTime().time();
    QTime sunsetTime = timeData["Sunset"].toDateTime().time();
    QTime currentTime = QDateTime::currentDateTime().time();

    // Provide mapping for the condition-type to the icons to display
    if (currentTime > sunriseTime && currentTime < sunsetTime) {
        // Day
        QString weather = condition(source).toLower();
        ConditionIcons condition = getConditionIcon(weather, true);
        data.insert("Condition Icon", getWeatherIcon(condition));
        qDebug() << "Using daytime icons\n";
    } else {
        // Night
        QString weather = condition(source).toLower();
        ConditionIcons condition = getConditionIcon(weather, false);
        data.insert("Condition Icon", getWeatherIcon(condition));
        qDebug() << "Using nighttime icons\n";
    }

    dataFields = temperature(source);
    data.insert("Temperature", dataFields["temperature"]);
    data.insert("Temperature Unit", dataFields["temperatureUnit"]);

    // Do we have a comfort temperature? if so display it
    if (dataFields["comfortTemperature"] != "N/A") {
        if (m_weatherData[source].windchill_F != "NA") {
            data.insert("Windchill", QString("%1").arg(dataFields["comfortTemperature"]));
            data.insert("Humidex", i18n("N/A"));
        }
        if (m_weatherData[source].heatindex_F != "NA" && m_weatherData[source].temperature_F.toInt() != m_weatherData[source].heatindex_F.toInt()) {
            data.insert("Humidex", QString("%1").arg(dataFields["comfortTemperature"]));
            data.insert("Windchill", i18n("N/A"));
        }
    } else {
        data.insert("Windchill", i18n("N/A"));
        data.insert("Humidex", i18n("N/A"));
    }

    data.insert("Dewpoint", dewpoint(source));
    dataFields = pressure(source);
    data.insert("Pressure", dataFields["pressure"]);
    data.insert("Pressure Unit", dataFields["pressureUnit"]);

    dataFields = visibility(source);
    data.insert("Visibility", dataFields["visibility"]);
    data.insert("Visibility Unit", dataFields["visibilityUnit"]);

    dataFields = humidity(source);
    data.insert("Humidity", dataFields["humidity"]);
    data.insert("Humidity Unit", dataFields["humidityUnit"]);

    // Set number of forecasts per day/night supported, none for this ion right now
    data.insert(QString("Total Weather Days"), 0);

    dataFields = wind(source);
    data.insert("Wind Speed", dataFields["windSpeed"]);
    data.insert("Wind Speed Unit", dataFields["windUnit"]);

    data.insert("Wind Gust", dataFields["windGust"]);
    data.insert("Wind Gust Unit", dataFields["windGustUnit"]);
    data.insert("Wind Direction", getWindDirectionIcon(windIcons(), dataFields["windDirection"].toLower()));
    data.insert("Credit", i18n("Data provided by NOAA National Weather Service"));

    int dayIndex = 0;
    foreach(const WeatherData::Forecast &forecast, m_weatherData[source].forecasts) {

        ConditionIcons icon = getConditionIcon(forecast.summary.toLower(), true);
        QString iconName = getWeatherIcon(icon);

        /* Sometimes the forecast for the later days is unavailable, if so skip remianing days
         * since their forecast data is probably unavailable.
         */
        if (forecast.low.isEmpty() || forecast.high.isEmpty()) {
            break;
        }

        // Get the short day name for the forecast
        data.insert(QString("Short Forecast Day %1").arg(dayIndex), QString("%1|%2|%3|%4|%5|%6")
                .arg(forecast.day).arg(iconName)
                .arg(i18nc("weather forecast", forecast.summary.toUtf8()))
                .arg(forecast.high).arg(forecast.low).arg("N/U"));
        dayIndex++;
    }

    // Set number of forecasts per day/night supported
    data.insert("Total Weather Days", dayIndex);



    setData(source, data);
}

QString const NOAAIon::country(const QString& source) const
{
    Q_UNUSED(source);
    return QString("USA");
}

QString NOAAIon::place(const QString& source) const
{
    return m_weatherData[source].locationName;
}

QString NOAAIon::station(const QString& source) const
{
    return m_weatherData[source].stationID;
}

QString NOAAIon::latitude(const QString& source) const
{
    return m_weatherData[source].stationLat;
}

QString NOAAIon::longitude(const QString& source) const
{
    return m_weatherData[source].stationLon;
}

QString NOAAIon::observationTime(const QString& source) const
{
    return m_weatherData[source].observationTime;
}

int NOAAIon::periodHour(const QString& source) const
{
    return m_weatherData[source].iconPeriodHour.toInt();
}

QString NOAAIon::condition(const QString& source)
{
    if (m_weatherData[source].weather.isEmpty() || m_weatherData[source].weather == "NA") {
        m_weatherData[source].weather = "N/A";
    }
    return m_weatherData[source].weather;
}

QString NOAAIon::conditionI18n(const QString& source)
{
    if (condition(source) == "N/A") {
        return i18n("N/A");
    } else {
        return i18nc("weather condition", condition(source).toUtf8());
    }
}

QString NOAAIon::dewpoint(const QString& source) const
{
    return m_weatherData[source].dewpoint_F;
}

QMap<QString, QString> NOAAIon::humidity(const QString& source) const
{
    QMap<QString, QString> humidityInfo;
    if (m_weatherData[source].humidity == "NA") {
        humidityInfo.insert("humidity", QString(i18n("N/A")));
        humidityInfo.insert("humidityUnit", QString::number(KUnitConversion::NoUnit));
        return humidityInfo;
    } else {
        humidityInfo.insert("humidity", m_weatherData[source].humidity);
        humidityInfo.insert("humidityUnit", QString::number(KUnitConversion::Percent));
    }

    return humidityInfo;
}

QMap<QString, QString> NOAAIon::visibility(const QString& source) const
{
    QMap<QString, QString> visibilityInfo;
    if (m_weatherData[source].visibility.isEmpty()) {
        visibilityInfo.insert("visibility", QString(i18n("N/A")));
        visibilityInfo.insert("visibilityUnit", QString::number(KUnitConversion::NoUnit));
        return visibilityInfo;
    }
    if (m_weatherData[source].visibility == "NA") {
        visibilityInfo.insert("visibility", QString(i18n("N/A")));
        visibilityInfo.insert("visibilityUnit", QString::number(KUnitConversion::NoUnit));
    } else {
        visibilityInfo.insert("visibility", m_weatherData[source].visibility);
        visibilityInfo.insert("visibilityUnit", QString::number(KUnitConversion::Mile));
    }
    return visibilityInfo;
}

QMap<QString, QString> NOAAIon::temperature(const QString& source) const
{
    QMap<QString, QString> temperatureInfo;
    temperatureInfo.insert("temperature", m_weatherData[source].temperature_F);
    temperatureInfo.insert("temperatureUnit", QString::number(KUnitConversion::Fahrenheit));
    temperatureInfo.insert("comfortTemperature", i18n("N/A"));

    if (m_weatherData[source].heatindex_F != "NA" && m_weatherData[source].windchill_F == "NA") {
        temperatureInfo.insert("comfortTemperature", m_weatherData[source].heatindex_F);
    }

    if (m_weatherData[source].windchill_F != "NA" && m_weatherData[source].heatindex_F == "NA") {
        temperatureInfo.insert("comfortTemperature", m_weatherData[source].windchill_F);
    }

    return temperatureInfo;
}

QMap<QString, QString> NOAAIon::pressure(const QString& source) const
{
    QMap<QString, QString> pressureInfo;
    if (m_weatherData[source].pressure.isEmpty()) {
        pressureInfo.insert("pressure", i18n("N/A"));
        pressureInfo.insert("pressureUnit", QString::number(KUnitConversion::NoUnit));
        return pressureInfo;
    }

    if (m_weatherData[source].pressure == "NA") {
        pressureInfo.insert("pressure", i18n("N/A"));
        pressureInfo.insert("visibilityUnit", QString::number(KUnitConversion::NoUnit));
    } else {
        pressureInfo.insert("pressure", m_weatherData[source].pressure);
        pressureInfo.insert("pressureUnit", QString::number(KUnitConversion::InchesOfMercury));
    }
    return pressureInfo;
}

QMap<QString, QString> NOAAIon::wind(const QString& source) const
{
    QMap<QString, QString> windInfo;

    // May not have any winds
    if (m_weatherData[source].windSpeed == "NA") {
        windInfo.insert("windSpeed", i18nc("wind speed", "Calm"));
        windInfo.insert("windUnit", QString::number(KUnitConversion::NoUnit));
    } else {
        windInfo.insert("windSpeed", QString::number(m_weatherData[source].windSpeed.toFloat(), 'f', 1));
        windInfo.insert("windUnit", QString::number(KUnitConversion::MilePerHour));
    }

    // May not always have gusty winds
    if (m_weatherData[source].windGust == "NA" || m_weatherData[source].windGust == "N/A") {
        windInfo.insert("windGust", i18n("N/A"));
        windInfo.insert("windGustUnit", QString::number(KUnitConversion::NoUnit));
    } else {
        windInfo.insert("windGust", QString::number(m_weatherData[source].windGust.toFloat(), 'f', 1));
        windInfo.insert("windGustUnit", QString::number(KUnitConversion::MilePerHour));
    }

    if (m_weatherData[source].windDirection.isEmpty()) {
        windInfo.insert("windDirection", i18n("N/A"));
    } else {
        windInfo.insert("windDirection", i18nc("wind direction", m_weatherData[source].windDirection.toUtf8()));
    }
    return windInfo;
}

/**
  * Determine the condition icon based on the list of possible NOAA weather conditions as defined at
  * <http://www.weather.gov/xml/current_obs/weather.php> and <http://www.weather.gov/mdl/XML/Design/MDL_XML_Design.htm#_Toc141760783>
  * Since the number of NOAA weather conditions need to be fitted into the narowly defined groups in IonInterface::ConditionIcons, we
  * try to group the NOAA conditions as best as we can based on their priorities/severity.
  */
IonInterface::ConditionIcons NOAAIon::getConditionIcon(const QString& weather, bool isDayTime) const
{
    // Consider any type of storm, tornado or funnel to be a thunderstorm.
    if (weather.contains("thunderstorm") || weather.contains("funnel") ||
        weather.contains("tornado") || weather.contains("storm") || weather.contains("tstms")) {

        if (weather.contains("vicinity") || weather.contains("chance")) {
            if (isDayTime) {
                return IonInterface::ChanceThunderstormDay;
            } else {
                return IonInterface::ChanceThunderstormNight;
            }
        }
        return IonInterface::Thunderstorm;

    } else if (weather.contains("pellets") || weather.contains("crystals") ||
             weather.contains("hail")) {
        return IonInterface::Hail;

    } else if (((weather.contains("rain") || weather.contains("drizzle") ||
              weather.contains("showers")) && weather.contains("snow")) || weather.contains("wintry mix")) {
        return IonInterface::RainSnow;

    } else if (weather.contains("snow") && weather.contains("light")) {
        return IonInterface::LightSnow;

    } else if (weather.contains("snow")) {

        if (weather.contains("vicinity") || weather.contains("chance")) {
            if (isDayTime) {
                return IonInterface::ChanceSnowDay;
            } else {
                return IonInterface::ChanceSnowNight;
            }
        }
        return IonInterface::Snow;

    } else if (weather.contains("freezing rain")) {
        return IonInterface::FreezingRain;

    } else if (weather.contains("freezing drizzle")) {
        return IonInterface::FreezingDrizzle;

    } else if (weather.contains("showers")) {

        if (weather.contains("vicinity") || weather.contains("chance")) {
            if (isDayTime) {
                return IonInterface::ChanceShowersDay;
            } else {
                return IonInterface::ChanceShowersNight;
            }
        }
        return IonInterface::Showers;

    } else if (weather.contains("light rain") || weather.contains("drizzle")) {
        return IonInterface::LightRain;

    } else if (weather.contains("rain")) {
        return IonInterface::Rain;

    } else if (weather.contains("few clouds") || weather.contains("mostly sunny") ||
               weather.contains("mostly clear") || weather.contains("increasing clouds") ||
               weather.contains("becoming cloudy") || weather.contains("clearing") ||
               weather.contains("decreasing clouds") || weather.contains("becoming sunny")) {
        if(isDayTime) {
            return IonInterface::FewCloudsDay;
        } else {
            return IonInterface::FewCloudsNight;
        }
    } else if (weather.contains("partly cloudy") || weather.contains("partly sunny") ||
               weather.contains("partly clear")) {
        if(isDayTime) {
            return IonInterface::PartlyCloudyDay;
        } else {
            return IonInterface::PartlyCloudyNight;
        }
    } else if (weather.contains("overcast") || weather.contains("cloudy")) {
        return IonInterface::Overcast;

    } else if (weather.contains("haze") || weather.contains("smoke") ||
             weather.contains("dust") || weather.contains("sand")) {
        return IonInterface::Haze;

    } else if (weather.contains("fair") || weather.contains("clear") || weather.contains("sunny")) {
        if (isDayTime) {
            return IonInterface::ClearDay;
        } else {
            return IonInterface::ClearNight;
        }
    } else if (weather.contains("fog")) {
        return IonInterface::Mist;

    } else {
        return IonInterface::NotAvailable;

    }
}

void NOAAIon::getForecast(const QString& source)
{
    /* Assuming that we have the latitude and longitude data at this point, get the 7-day
     * forecast.
     */
    KUrl url = QString("http://www.weather.gov/forecasts/xml/sample_products/browser_interface/"
                       "ndfdBrowserClientByDay.php?lat=%1&lon=%2&format=24+hourly&numDays=7")
                        .arg(latitude(source)).arg(longitude(source));

    KIO::TransferJob * const m_job = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    m_jobXml.insert(m_job, new QXmlStreamReader);
    m_jobList.insert(m_job, source);

    if (m_job) {
        connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this,
                SLOT(forecast_slotDataArrived(KIO::Job*,QByteArray)));
        connect(m_job, SIGNAL(result(KJob*)), this, SLOT(forecast_slotJobFinished(KJob*)));
    }
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
            if (xml.name() == "layout-key" && xml.readElementText() == "k-p24h-n7-1") {

                // Read days until we get to end of parent (<time-layout>)tag
                while (! (xml.isEndElement() && xml.name() == "time-layout")) {

                    xml.readNext();

                    if (xml.name() == "start-valid-time") {
                        QString data = xml.readElementText();
                        QDateTime date = QDateTime::fromString(data, Qt::ISODate);

                        WeatherData::Forecast forecast;
                        forecast.day = KLocalizedDate(date.date()).formatDate(KLocale::DayName, KLocale::ShortName);
                        forecasts.append(forecast);
                        //qDebug() << forecast.day;
                    }
                }

            } else if (xml.name() == "temperature" && xml.attributes().value("type") == "maximum") {

                // Read max temps until we get to end tag
                int i = 0;
                while (! (xml.isEndElement() && xml.name() == "temperature") &&
                       i < forecasts.count()) {

                    xml.readNext();

                    if (xml.name() == "value") {
                        forecasts[i].high = xml.readElementText();
                        //qDebug() << forecasts[i].high;
                        i++;
                    }
                }
            } else if (xml.name() == "temperature" && xml.attributes().value("type") == "minimum") {

                // Read min temps until we get to end tag
                int i = 0;
                while (! (xml.isEndElement() && xml.name() == "temperature") &&
                       i < forecasts.count()) {

                    xml.readNext();

                    if (xml.name() == "value") {
                        forecasts[i].low = xml.readElementText();
                        //qDebug() << forecasts[i].low;
                        i++;
                    }
                }
            } else if (xml.name() == "weather") {

                // Read weather conditions until we get to end tag
                int i = 0;
                while (! (xml.isEndElement() && xml.name() == "weather") &&
                       i < forecasts.count()) {

                    xml.readNext();

                    if (xml.name() == "weather-conditions" && xml.isStartElement()) {
                        QString summary = xml.attributes().value("weather-summary").toString();
                        forecasts[i].summary = summary;
                        //qDebug() << forecasts[i].summary;
			qDebug() << "i18n summary string: "
                                 << qPrintable(i18nc("weather forecast", forecasts[i].summary.toUtf8()));
                        i++;
                    }
                }
            }
        }
    }
}

#include "ion_noaa.moc"
