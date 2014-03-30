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

/* Ion for BBC's Weather from the UK Met Office */

#include "ion_bbcukmet.h"

#include <KIO/Job>
#include <KUnitConversion/Converter>

// ctor, dtor
UKMETIon::UKMETIon(QObject *parent, const QVariantList &args)
        : IonInterface(parent, args)

{
    Q_UNUSED(args)
}

UKMETIon::~UKMETIon()
{
    deleteForecasts();
}

void UKMETIon::reset()
{
    deleteForecasts();
    m_sourcesToReset = sources();
    updateAllSources();
}

void UKMETIon::deleteForecasts()
{
    // Destroy each forecast stored in a QVector
    QHash<QString, WeatherData>::iterator
        it = m_weatherData.begin(),
        end = m_weatherData.end();
    for (; it != end; ++it) {
        qDeleteAll(it.value().forecasts);
        it.value().forecasts.clear();
    }
}


// Get the master list of locations to be parsed
void UKMETIon::init()
{
    m_timeEngine = dataEngine("time");
    setInitialized(true);
}

QMap<QString, IonInterface::ConditionIcons> UKMETIon::setupDayIconMappings(void) const
{
//    ClearDay, FewCloudsDay, PartlyCloudyDay, Overcast,
//    Showers, ScatteredShowers, Thunderstorm, Snow,
//    FewCloudsNight, PartlyCloudyNight, ClearNight,
//    Mist, NotAvailable

    QMap<QString, ConditionIcons> dayList;
    dayList["sunny"] = ClearDay;
    //dayList["sunny night"] = ClearNight;
    dayList["clear"] = ClearDay;
    dayList["clar sky"] = ClearDay;
    dayList["sunny intervals"] = PartlyCloudyDay;
    //dayList["sunny intervals night"] = ClearNight;
    dayList["partly cloudy"] = PartlyCloudyDay;
    dayList["cloudy"] = Overcast;
    dayList["white cloud"] = Overcast;
    dayList["grey cloud"] = Overcast;
    //dayList["low level cloud"] = NotAvailable;
    //dayList["medium level cloud"] = NotAvailable;
    //dayList["sandstorm"] = NotAvailable;
    dayList["drizzle"] = LightRain;
    dayList["misty"] = Mist;
    dayList["mist"] = Mist;
    dayList["fog"] = Mist;
    dayList["foggy"] = Mist;
    dayList["tropical storm"] = Thunderstorm;
    dayList["hazy"] = NotAvailable;
    dayList["light shower"] = Showers;
    dayList["light rain shower"] = Showers;
    dayList["light showers"] = Showers;
    dayList["light rain"] = Showers;
    dayList["heavy rain"] = Rain;
    dayList["heavy showers"] = Rain;
    dayList["heavy shower"] = Rain;
    dayList["heavy rain shower"] = Rain;
    dayList["thundery shower"] = Thunderstorm;
    dayList["thunder storm"] = Thunderstorm;
    dayList["cloudy with sleet"] = RainSnow;
    dayList["sleet shower"] = RainSnow;
    dayList["sleet showers"] = RainSnow;
    dayList["sleet"] = RainSnow;
    dayList["cloudy with hail"] = Hail;
    dayList["hail shower"] = Hail;
    dayList["hail showers"] = Hail;
    dayList["hail"] = Hail;
    dayList["light snow"] = LightSnow;
    dayList["light snow shower"] = Flurries;
    dayList["light snow showers"] = Flurries;
    dayList["cloudy with light snow"] = LightSnow;
    dayList["heavy snow"] = Snow;
    dayList["heavy snow shower"] = Snow;
    dayList["heavy snow showers"] = Snow;
    dayList["cloudy with heavy snow"] = Snow;
    dayList["na"] = NotAvailable;
    return dayList;
}

QMap<QString, IonInterface::ConditionIcons> UKMETIon::setupNightIconMappings(void) const
{
    QMap<QString, ConditionIcons> nightList;
    nightList["clear"] = ClearNight;
    nightList["clear sky"] = ClearNight;
    nightList["clear intervals"] = PartlyCloudyNight;
    nightList["sunny intervals"] = PartlyCloudyDay; // it's not really sunny
    nightList["sunny"] = ClearDay;
    nightList["cloudy"] = Overcast;
    nightList["white cloud"] = Overcast;
    nightList["grey cloud"] = Overcast;
    nightList["partly cloudy"] = PartlyCloudyNight;
    nightList["drizzle"] = LightRain;
    nightList["misty"] = Mist;
    nightList["mist"] = Mist;
    nightList["fog"] = Mist;
    nightList["foggy"] = Mist;
    nightList["tropical storm"] = Thunderstorm;
    nightList["hazy"] = NotAvailable;
    nightList["light shower"] = Showers;
    nightList["light rain shower"] = Showers;
    nightList["light showers"] = Showers;
    nightList["light rain"] = Showers;
    nightList["heavy rain"] = Rain;
    nightList["heavy showers"] = Rain;
    nightList["heavy shower"] = Rain;
    nightList["heavy rain shower"] = Rain;
    nightList["thundery shower"] = Thunderstorm;
    nightList["thunder storm"] = Thunderstorm;
    nightList["cloudy with sleet"] = NotAvailable;
    nightList["sleet shower"] = NotAvailable;
    nightList["sleet showers"] = NotAvailable;
    nightList["sleet"] = NotAvailable;
    nightList["cloudy with hail"] = Hail;
    nightList["hail shower"] = Hail;
    nightList["hail showers"] = Hail;
    nightList["hail"] = Hail;
    nightList["light snow"] = LightSnow;
    nightList["light snow shower"] = Flurries;
    nightList["light snow showers"] = Flurries;
    nightList["cloudy with light snow"] = LightSnow;
    nightList["heavy snow"] = Snow;
    nightList["heavy snow shower"] = Snow;
    nightList["heavy snow showers"] = Snow;
    nightList["cloudy with heavy snow"] = Snow;
    nightList["na"] = NotAvailable;

    return nightList;
}

QMap<QString, IonInterface::ConditionIcons> const& UKMETIon::dayIcons(void) const
{
    static QMap<QString, ConditionIcons> const dval = setupDayIconMappings();
    return dval;
}

QMap<QString, IonInterface::ConditionIcons> const& UKMETIon::nightIcons(void) const
{
    static QMap<QString, ConditionIcons> const nval = setupNightIconMappings();
    return nval;
}

// Get a specific Ion's data
bool UKMETIon::updateIonSource(const QString& source)
{
    // We expect the applet to send the source in the following tokenization:
    // ionname|validate|place_name - Triggers validation of place
    // ionname|weather|place_name - Triggers receiving weather of place

    QStringList sourceAction = source.split('|');

    // Guard: if the size of array is not 3 then we have bad data, return an error
    if (sourceAction.size() < 3) {
        setData(source, "validate", "bbcukmet|malformed");
        return true;
    }

    if (sourceAction[1] == "validate" && sourceAction.size() >= 3) {
        // Look for places to match
        findPlace(sourceAction[2], source);
        return true;
    } else if (sourceAction[1] == "weather" && sourceAction.size() >= 3) {
        if (sourceAction.count() >= 3) {
            if (sourceAction[2].isEmpty()) {
                setData(source, "validate", "bbcukmet|malformed");
                return true;
            }
            m_place[QString("bbcukmet|%1").arg(sourceAction[2])].XMLurl = sourceAction[3];
            getXMLData(QString("%1|%2").arg(sourceAction[0]).arg(sourceAction[2]));
            return true;
        } else {
            return false;
        }
    } else {
        setData(source, "validate", "bbcukmet|malformed");
        return true;
    }

    return false;
}

// Gets specific city XML data
void UKMETIon::getXMLData(const QString& source)
{
    foreach (const QString &fetching, m_obsJobList) {
        if (fetching == source) {
            // already getting this source and awaiting the data
            return;
        }
    }

    KUrl url;
    url = m_place[source].XMLurl;

    m_job = KIO::get(url.url(), KIO::Reload, KIO::HideProgressInfo);
    m_job->addMetaData("cookies", "none"); // Disable displaying cookies
    m_obsJobXml.insert(m_job, new QXmlStreamReader);
    m_obsJobList.insert(m_job, source);

    if (m_job) {
        connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this,
                SLOT(observation_slotDataArrived(KIO::Job*,QByteArray)));
        connect(m_job, SIGNAL(result(KJob*)), this, SLOT(observation_slotJobFinished(KJob*)));
    }
}

// Parses city list and gets the correct city based on ID number
void UKMETIon::findPlace(const QString& place, const QString& source)
{
    KUrl url;
    url = "http://news.bbc.co.uk/weather/util/search/SearchResultsNode.xhtml?&search=" + place + "&region=world&startIndex=0&count=500";

    m_job = KIO::get(url.url(), KIO::Reload, KIO::HideProgressInfo);
    m_job->addMetaData("cookies", "none"); // Disable displaying cookies
    m_jobHtml.insert(m_job, new QByteArray());
    m_jobList.insert(m_job, source);

    if (m_job) {
        connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this,
                SLOT(setup_slotDataArrived(KIO::Job*,QByteArray)));
        connect(m_job, SIGNAL(result(KJob*)), this, SLOT(setup_slotJobFinished(KJob*)));

/*
        // Handle redirects for direct hit places.
        connect(m_job, SIGNAL(redirection(KIO::Job*,KUrl)), this,
                SLOT(setup_slotRedirected(KIO::Job*,KUrl)));
*/
    }
}

void UKMETIon::getFiveDayForecast(const QString& source)
{

    KUrl xmlMap(m_place[source].forecastHTMLUrl);    
    
    QString xmlPath = xmlMap.path();
    
    int splitIDPos = xmlPath.lastIndexOf('/');
    QString stationID = xmlPath.midRef(splitIDPos + 1).toString();
    m_place[source].XMLforecastURL = "http://newsrss.bbc.co.uk/weather/forecast/" + stationID + "/Next3DaysRSS.xml" + xmlMap.query();
    KUrl url(m_place[source].XMLforecastURL);

    m_job = KIO::get(url.url(), KIO::Reload, KIO::HideProgressInfo);
    m_job->addMetaData("cookies", "none"); // Disable displaying cookies
    m_forecastJobXml.insert(m_job, new QXmlStreamReader);
    m_forecastJobList.insert(m_job, source);

    if (m_job) {
        connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this,
                SLOT(forecast_slotDataArrived(KIO::Job*,QByteArray)));
        connect(m_job, SIGNAL(result(KJob*)), this, SLOT(forecast_slotJobFinished(KJob*)));
    }
}

void UKMETIon::readSearchHTMLData(const QString& source, const QByteArray& html)
{
    QTextStream stream(html.data());
    QString line;
    QStringList tokens;
    QString url;
    QString tmp;
    int flag = 0;
    int counter = 2;

    // "<p><a id="result_40" href ="/weather/forecast/4160?count=200">Vitoria, Brazil</a></p>"
    QRegExp grabURL("/[a-z]+/[a-z]+/([0-9]+)(\\?[^\"]+)?");
    QRegExp grabPlace(">([^<]*[a-z()])"); // FIXME: It would be better to strip away the extra '>'

    while (!stream.atEnd()) {
       line = stream.readLine();
       if (line.contains("<p class=\"response\">") > 0) {
           flag = 1;
       }

       if (line.contains("There are no forecasts matching") > 0) {
           break;
       }

       if (flag) {

            if (grabURL.indexIn(line.trimmed()) > 0) {
                url = "http://newsrss.bbc.co.uk/weather/forecast/" + grabURL.cap(1) + "/ObservationsRSS.xml";
                if (grabURL.captureCount() > 1) {
                    url += grabURL.cap(2);
                }
                grabPlace.indexIn(line.trimmed());
                tmp = QString("bbcukmet|").append(grabPlace.cap(1));

                // Duplicate places can exist
                if (m_locations.contains(tmp)) {
                    tmp = QString("bbcukmet|").append(QString("%1 (#%2)").arg(grabPlace.cap(1)).arg(counter));
                    counter++;
                }

                m_place[tmp].XMLurl = url;
                m_place[tmp].place = grabPlace.cap(1);
                m_locations.append(tmp);
            }
       }

       if (line.contains("<div class=\"line\">") > 0) {
           flag = 0;
       }
    }

    // I stream ok?
    //if (stream.status() == QTextStream::Ok) {
        //return true;
    //}

    validate(source);
}

// handle when no XML tag is found
void UKMETIon::parseUnknownElement(QXmlStreamReader& xml) const
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

void UKMETIon::setup_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    if (data.isEmpty() || !m_jobHtml.contains(job)) {
        return;
    }

    m_jobHtml[job]->append(data);
}

void UKMETIon::setup_slotJobFinished(KJob *job)
{
    if (job->error() == 149) {
        setData(m_jobList[job], "validate", QString("bbcukmet|timeout"));
        disconnectSource(m_jobList[job], this);
        m_jobList.remove(job);
        delete m_jobHtml[job];
        m_jobHtml.remove(job);
        return;
    }

    // If Redirected, don't go to this routine
    if (!m_locations.contains(QString("bbcukmet|%1").arg(m_jobList[job]))) {
        QByteArray *reader = m_jobHtml.value(job);
        if (reader) {
            readSearchHTMLData(m_jobList[job], *reader);
        }
    }
    m_jobList.remove(job);
    delete m_jobHtml[job];
    m_jobHtml.remove(job);
}

void UKMETIon::observation_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    QByteArray local = data;
    if (data.isEmpty() || !m_obsJobXml.contains(job)) {
        return;
    }

    // Send to xml.
    m_obsJobXml[job]->addData(local);
}

void UKMETIon::observation_slotJobFinished(KJob *job)
{
    const QString source = m_obsJobList.value(job);
    setData(source, Data());

    QXmlStreamReader *reader = m_obsJobXml.value(job);
    if (reader) {
        readObservationXMLData(m_obsJobList[job], *reader);
    }

    m_obsJobList.remove(job);
    delete m_obsJobXml[job];
    m_obsJobXml.remove(job);

    if (m_sourcesToReset.contains(source)) {
        m_sourcesToReset.removeAll(source);
        emit forceUpdate(this, source);
    }
}

void UKMETIon::forecast_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    QByteArray local = data;
    if (data.isEmpty() || !m_forecastJobXml.contains(job)) {
        return;
    }

    // Send to xml.
    m_forecastJobXml[job]->addData(local);
}

void UKMETIon::forecast_slotJobFinished(KJob *job)
{
    setData(m_forecastJobList[job], Data());
    QXmlStreamReader *reader = m_forecastJobXml.value(job);
    if (reader) {
        readFiveDayForecastXMLData(m_forecastJobList[job], *reader);
    }

    m_forecastJobList.remove(job);
    delete m_forecastJobXml[job];
    m_forecastJobXml.remove(job);
}

void UKMETIon::parsePlaceObservation(const QString &source, WeatherData& data, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "rss");

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == "rss") {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == "channel") {
                parseWeatherChannel(source, data, xml);
            }
        }
    }
}

void UKMETIon::parsePlaceForecast(const QString &source, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "rss");

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == "channel") {
            parseWeatherForecast(source, xml);
        }
    }
}

void UKMETIon::parseWeatherChannel(const QString& source, WeatherData& data, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "channel");

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == "channel") {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == "title") {
                data.stationName = xml.readElementText().split("Observations for")[1].trimmed();
                data.stationName.replace("United Kingdom", i18n("UK"));
                data.stationName.replace("United States of America", i18n("USA"));

            } else if (xml.name() == "item") {
                parseWeatherObservation(source, data, xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

void UKMETIon::parseWeatherForecast(const QString& source, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "channel");

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == "channel") {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == "item") {
                parseFiveDayForecast(source, xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

void UKMETIon::parseWeatherObservation(const QString& source, WeatherData& data, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "item");

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == "item") {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == "title") {
                QString conditionString = xml.readElementText();

                // Get the observation time and condition
                int splitIndex = conditionString.lastIndexOf(':');
                QStringRef conditionData = conditionString.midRef(splitIndex + 1); // Include ':'
                data.obsTime = conditionString.midRef(0, splitIndex).toString();

                // Friday at 0200 GMT
                m_dateFormat =  QDateTime::fromString(data.obsTime.split("at")[1].trimmed(), "hhmm 'GMT'");
                data.iconPeriodHour = m_dateFormat.toString("hh").toInt();
                data.iconPeriodMinute = m_dateFormat.toString("mm").toInt();

                data.condition = conditionData.toString().split('.')[0].trimmed();

            } else if (xml.name() == "link") {
                m_place[source].forecastHTMLUrl = xml.readElementText();

            } else if (xml.name() == "description") {
                QString observeString = xml.readElementText();
                QStringList observeData = observeString.split(':');
#ifdef __GNUC__
#warning FIXME: We should make this use a QRegExp but I need some help here :) -spstarr
#endif

                data.temperature_C = observeData[1].split(QChar(176))[0].trimmed();

                // Temperature might be not available
                if (data.temperature_C.contains("N/A")) {
                    data.temperature_C = i18n("N/A");
                }

                data.windDirection = observeData[2].split(',')[0].trimmed();
                data.windSpeed_miles = observeData[3].split(',')[0].split(' ')[1].remove("mph");

                data.humidity = observeData[4].split(',')[0].split(' ')[1];
                if (data.humidity.endsWith('%')) {
                    data.humidity.chop(1);
                }

                data.pressure = observeData[5].split(',')[0].split(' ')[1].split("mb")[0];
                data.pressureTendency = observeData[5].split(',')[1].trimmed();

                data.visibilityStr = observeData[6].trimmed();

            } else if (xml.name() == "lat") {
                const QString ordinate = xml.readElementText();
                data.latitude = ordinate.toDouble();
            } else if (xml.name() == "long") {
                const QString ordinate = xml.readElementText();
                data.longitude = ordinate.toDouble();
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

bool UKMETIon::readObservationXMLData(const QString& source, QXmlStreamReader& xml)
{
    WeatherData data;
    bool haveObservation = false;
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == "rss") {
                parsePlaceObservation(source, data, xml);
                haveObservation = true;
            } else {
                parseUnknownElement(xml);
            }
        }

    }

    if (!haveObservation) {
        return false;
    }
    m_weatherData[source] = data;

    // Get the 5 day forecast info next.
    getFiveDayForecast(source);

    return !xml.error();
}

bool UKMETIon::readFiveDayForecastXMLData(const QString& source, QXmlStreamReader& xml)
{
    bool haveFiveDay = false;
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == "rss") {
                parsePlaceForecast(source, xml);
                haveFiveDay = true;
            } else {
                parseUnknownElement(xml);
            }
        }
    }
    if (!haveFiveDay) return false;
    updateWeather(source);
    return !xml.error();
}

void UKMETIon::parseFiveDayForecast(const QString& source, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "item");

    // Flush out the old forecasts when updating.
    m_weatherData[source].forecasts.clear();

    WeatherData::ForecastInfo *forecast = new WeatherData::ForecastInfo;
    QString line;
    QString period;
    QString summary;
    QRegExp high("-?\\d+");
    QRegExp low("-?\\d+");
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.name() == "title") {
            line = xml.readElementText().trimmed();
#ifdef __GNUC__
#warning FIXME: We should make this all use QRegExps in UKMETIon::parseFiveDayForecast() for forecast -spstarr
#endif

            period = line.split(',')[0].split(':')[0];
            summary = line.split(',')[0].split(':')[1].trimmed();
            high.indexIn(line.split(',')[1]);
            low.indexIn(line.split(',')[2]);

            forecast->period = period;
            forecast->iconName = getWeatherIcon(dayIcons(), summary.toLower());
            forecast->summary = i18nc("weather forecast", summary.toUtf8());
            qDebug() << "i18n summary string: " << qPrintable(forecast->summary);
            forecast->tempHigh = high.cap(0).toInt();
            forecast->tempLow = low.cap(0).toInt();
            m_weatherData[source].forecasts.append(forecast);
            forecast = new WeatherData::ForecastInfo;
        }
    }
    delete forecast;
}

void UKMETIon::validate(const QString& source)
{
    bool beginflag = true;

    if (!m_locations.count()) {
        QStringList invalidPlace = source.split('|');
        if (m_place[QString("bbcukmet|%1").arg(invalidPlace[2])].place.isEmpty()) {
            setData(source, "validate", QString("bbcukmet|invalid|multiple|%1").arg(invalidPlace[2]));
        }
        m_locations.clear();
        return;
    } else {
        QString placeList;
        foreach(const QString &place, m_locations) {
            if (beginflag) {
                placeList.append(QString("%1|extra|%2").arg(place.split('|')[1]).arg(m_place[place].XMLurl));
                beginflag = false;
            } else {
                placeList.append(QString("|place|%1|extra|%2").arg(place.split('|')[1]).arg(m_place[place].XMLurl));
            }
        }
        if (m_locations.count() > 1) {
            setData(source, "validate", QString("bbcukmet|valid|multiple|place|%1").arg(placeList));
        } else {
            placeList[0] = placeList[0].toUpper();
            setData(source, "validate", QString("bbcukmet|valid|single|place|%1").arg(placeList));
        }
    }
    m_locations.clear();
}

void UKMETIon::updateWeather(const QString& source)
{
    QString weatherSource = source;
    weatherSource.replace("bbcukmet|", "bbcukmet|weather|");
    weatherSource.append(QString("|%1").arg(m_place[source].XMLurl));

    QMap<QString, QString> dataFields;
    QStringList fieldList;
    QVector<QString> forecastList;
    int i = 0;

    Plasma::DataEngine::Data data;

    data.insert("Place", place(source));
    data.insert("Station", station(source));
    data.insert("Observation Period", observationTime(source));
    data.insert("Current Conditions", i18nc("weather condition", condition(source).toUtf8()));
    qDebug() << "i18n condition string: " << qPrintable(i18nc("weather condition", condition(source).toUtf8()));

    const double lati = periodLatitude(source);
    const double longi = periodLongitude(source);
    const Plasma::DataEngine::Data timeData = m_timeEngine->query(
            QString("Local|Solar|Latitude=%1|Longitude=%2|DateTime=%3")
                .arg(lati).arg(longi).arg(m_dateFormat.toString(Qt::ISODate)));

    // Tell applet which icon to use for conditions and provide mapping for condition type to the icons to display
    if (timeData["Corrected Elevation"].toDouble() >= 0.0) {
        //qDebug() << "Using daytime icons\n";
        data.insert("Condition Icon", getWeatherIcon(dayIcons(), condition(source)));
    } else {
        data.insert("Condition Icon", getWeatherIcon(nightIcons(), condition(source)));
    }

    data.insert("Latitude", lati);
    data.insert("Longitude", longi);

    dataFields = humidity(source);
    data.insert("Humidity", dataFields["humidity"]);
    data.insert("Humidity Field", dataFields["humidityUnit"]);

    data.insert("Visibility", visibility(source));

    dataFields = temperature(source);
    data.insert("Temperature", dataFields["temperature"]);
    data.insert("Temperature Unit", dataFields["temperatureUnit"]);

    dataFields = pressure(source);
    data.insert("Pressure", dataFields["pressure"]);
    data.insert("Pressure Unit", dataFields["pressureUnit"]);
    data.insert("Pressure Tendency", dataFields["pressureTendency"]);

    dataFields = wind(source);
    data.insert("Wind Speed", dataFields["windSpeed"]);
    data.insert("Wind Speed Unit", dataFields["windUnit"]);
    data.insert("Wind Direction", dataFields["windDirection"]);

    // 5 Day forecast info
    forecastList = forecasts(source);

    // Set number of forecasts per day/night supported
    data.insert("Total Weather Days", m_weatherData[source].forecasts.size());

    foreach(const QString &forecastItem, forecastList) {
        fieldList = forecastItem.split('|');

        data.insert(QString("Short Forecast Day %1").arg(i), QString("%1|%2|%3|%4|%5|%6") \
                .arg(fieldList[0]).arg(fieldList[1]).arg(fieldList[2]).arg(fieldList[3]) \
                .arg(fieldList[4]).arg(fieldList[5]));
        i++;
    }

    data.insert("Credit", i18n("Supported by backstage.bbc.co.uk / Data from UK MET Office"));
    data.insert("Credit Url", m_place[source].forecastHTMLUrl);

    setData(weatherSource, data);
}

QString UKMETIon::place(const QString& source) const
{
    return m_weatherData[source].stationName;
}

QString UKMETIon::station(const QString& source) const
{
    return m_weatherData[source].stationName;
}

QString UKMETIon::observationTime(const QString& source) const
{
    return m_weatherData[source].obsTime;
}

int UKMETIon::periodHour(const QString& source) const
{
    return m_weatherData[source].iconPeriodHour;
}

int UKMETIon::periodMinute(const QString& source) const
{
    return m_weatherData[source].iconPeriodMinute;
}

double UKMETIon::periodLatitude(const QString& source) const
{
    return m_weatherData[source].latitude;
}

double UKMETIon::periodLongitude(const QString& source) const
{
    return m_weatherData[source].longitude;
}

QString UKMETIon::condition(const QString& source) const
{
    return (m_weatherData[source].condition);
}

QMap<QString, QString> UKMETIon::temperature(const QString& source) const
{
    QMap<QString, QString> temperatureInfo;

    temperatureInfo.insert("temperature", QString(m_weatherData[source].temperature_C));
    temperatureInfo.insert("temperatureUnit", QString::number(KUnitConversion::Celsius));
    return temperatureInfo;
}

QMap<QString, QString> UKMETIon::wind(const QString& source) const
{
    QMap<QString, QString> windInfo;
    if (m_weatherData[source].windSpeed_miles == "N/A") {
        windInfo.insert("windSpeed", i18n("N/A"));
        windInfo.insert("windUnit", QString::number(KUnitConversion::NoUnit));
    } else {
        windInfo.insert("windSpeed", QString(m_weatherData[source].windSpeed_miles));
        windInfo.insert("windUnit", QString::number(KUnitConversion::MilePerHour));
    }
    if (m_weatherData[source].windDirection.isEmpty()) {
        windInfo.insert("windDirection", i18n("N/A"));
    } else {
        windInfo.insert("windDirection", i18nc("wind direction", m_weatherData[source].windDirection.toUtf8()));
    }
    return windInfo;
}

QMap<QString, QString> UKMETIon::humidity(const QString& source) const
{
    QMap<QString, QString> humidityInfo;
    if (m_weatherData[source].humidity != "N/A") {
        humidityInfo.insert("humidity", m_weatherData[source].humidity);
        humidityInfo.insert("humidityUnit", QString::number(KUnitConversion::Percent));
    } else {
        humidityInfo.insert("humidity", i18n("N/A"));
        humidityInfo.insert("humidityUnit", QString::number(KUnitConversion::NoUnit));
    }

    return humidityInfo;
}

QString UKMETIon::visibility(const QString& source) const
{
    return i18nc("visibility", m_weatherData[source].visibilityStr.toUtf8());
}

QMap<QString, QString> UKMETIon::pressure(const QString& source) const
{
    QMap<QString, QString> pressureInfo;
    if (m_weatherData[source].pressure == "N/A") {
        pressureInfo.insert("pressure", i18n("N/A"));
        pressureInfo.insert("pressureUnit", QString::number(KUnitConversion::NoUnit));
        pressureInfo.insert("pressureTendency", i18n("N/A"));
        return pressureInfo;
    }

    pressureInfo.insert("pressure", QString(m_weatherData[source].pressure));
    pressureInfo.insert("pressureUnit", QString::number(KUnitConversion::Millibar));

    pressureInfo.insert("pressureTendency", i18nc("pressure tendency", m_weatherData[source].pressureTendency.toUtf8()));
    return pressureInfo;
}

QVector<QString> UKMETIon::forecasts(const QString& source)
{
    QVector<QString> forecastData;

    for (int i = 0; i < m_weatherData[source].forecasts.size(); ++i) {

        if (m_weatherData[source].forecasts[i]->period.contains("Saturday")) {
            m_weatherData[source].forecasts[i]->period.replace("Saturday", i18nc("Short for Saturday", "Sat"));
        }

        if (m_weatherData[source].forecasts[i]->period.contains("Sunday")) {
            m_weatherData[source].forecasts[i]->period.replace("Sunday", i18nc("Short for Sunday", "Sun"));
        }

        if (m_weatherData[source].forecasts[i]->period.contains("Monday")) {
            m_weatherData[source].forecasts[i]->period.replace("Monday", i18nc("Short for Monday", "Mon"));
        }

        if (m_weatherData[source].forecasts[i]->period.contains("Tuesday")) {
            m_weatherData[source].forecasts[i]->period.replace("Tuesday", i18nc("Short for Tuesday", "Tue"));
        }

        if (m_weatherData[source].forecasts[i]->period.contains("Wednesday")) {
            m_weatherData[source].forecasts[i]->period.replace("Wednesday", i18nc("Short for Wednesday", "Wed"));
        }

        if (m_weatherData[source].forecasts[i]->period.contains("Thursday")) {
            m_weatherData[source].forecasts[i]->period.replace("Thursday", i18nc("Short for Thursday", "Thu"));
        }
        if (m_weatherData[source].forecasts[i]->period.contains("Friday")) {
            m_weatherData[source].forecasts[i]->period.replace("Friday", i18nc("Short for Friday", "Fri"));
        }

        forecastData.append(QString("%1|%2|%3|%4|%5|%6") \
                            .arg(m_weatherData[source].forecasts[i]->period) \
                            .arg(m_weatherData[source].forecasts[i]->iconName) \
                            .arg(m_weatherData[source].forecasts[i]->summary) \
                            .arg(m_weatherData[source].forecasts[i]->tempHigh) \
                            .arg(m_weatherData[source].forecasts[i]->tempLow) \
                            .arg("N/U"));
        //.arg(m_weatherData[source].forecasts[i]->windSpeed)
        //arg(m_weatherData[source].forecasts[i]->windDirection));
    }

    return forecastData;
}

#include "ion_bbcukmet.moc"
