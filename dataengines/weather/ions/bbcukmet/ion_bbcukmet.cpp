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
#include <KLocalizedString>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamReader>
#include <QDebug>

WeatherData::WeatherData()
  : obsTime(QStringLiteral("N/A"))
  , iconPeriodHour(12)
  , iconPeriodMinute(0)
  , longitude(0.)
  , latitude(0.)
  , condition(QStringLiteral("N/A"))
{
}

// ctor, dtor
UKMETIon::UKMETIon(QObject *parent, const QVariantList &args)
        : IonInterface(parent, args)

{
    // not used while daytime not considered, see below
    // m_timeEngine = dataEngine("time");
    setInitialized(true);
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



QMap<QString, IonInterface::ConditionIcons> UKMETIon::setupDayIconMappings() const
{
//    ClearDay, FewCloudsDay, PartlyCloudyDay, Overcast,
//    Showers, ScatteredShowers, Thunderstorm, Snow,
//    FewCloudsNight, PartlyCloudyNight, ClearNight,
//    Mist, NotAvailable

    QMap<QString, ConditionIcons> dayList;
    dayList.insert(QStringLiteral("sunny"), ClearDay);
    //dayList.insert(QStringLiteral("sunny night"), ClearNight);
    dayList.insert(QStringLiteral("clear"), ClearDay);
    dayList.insert(QStringLiteral("clear sky"), ClearDay);
    dayList.insert(QStringLiteral("sunny intervals"), PartlyCloudyDay);
    //dayList.insert(QStringLiteral("sunny intervals night"), ClearNight);
    dayList.insert(QStringLiteral("partly cloudy"), PartlyCloudyDay);
    dayList.insert(QStringLiteral("cloudy"), PartlyCloudyDay);
    dayList.insert(QStringLiteral("light cloud"), PartlyCloudyDay);
    dayList.insert(QStringLiteral("white cloud"), PartlyCloudyDay);
    dayList.insert(QStringLiteral("grey cloud"), Overcast);
    dayList.insert(QStringLiteral("thick cloud"), Overcast);
    //dayList.insert(QStringLiteral("low level cloud"), NotAvailable);
    //dayList.insert(QStringLiteral("medium level cloud"), NotAvailable);
    //dayList.insert(QStringLiteral("sandstorm"), NotAvailable);
    dayList.insert(QStringLiteral("drizzle"), LightRain);
    dayList.insert(QStringLiteral("misty"), Mist);
    dayList.insert(QStringLiteral("mist"), Mist);
    dayList.insert(QStringLiteral("fog"), Mist);
    dayList.insert(QStringLiteral("foggy"), Mist);
    dayList.insert(QStringLiteral("tropical storm"), Thunderstorm);
    dayList.insert(QStringLiteral("hazy"), NotAvailable);
    dayList.insert(QStringLiteral("light shower"), Showers);
    dayList.insert(QStringLiteral("light rain shower"), Showers);
    dayList.insert(QStringLiteral("light showers"), Showers);
    dayList.insert(QStringLiteral("light rain"), Showers);
    dayList.insert(QStringLiteral("heavy rain"), Rain);
    dayList.insert(QStringLiteral("heavy showers"), Rain);
    dayList.insert(QStringLiteral("heavy shower"), Rain);
    dayList.insert(QStringLiteral("heavy rain shower"), Rain);
    dayList.insert(QStringLiteral("thundery shower"), Thunderstorm);
    dayList.insert(QStringLiteral("thunder storm"), Thunderstorm);
    dayList.insert(QStringLiteral("cloudy with sleet"), RainSnow);
    dayList.insert(QStringLiteral("sleet shower"), RainSnow);
    dayList.insert(QStringLiteral("sleet showers"), RainSnow);
    dayList.insert(QStringLiteral("sleet"), RainSnow);
    dayList.insert(QStringLiteral("cloudy with hail"), Hail);
    dayList.insert(QStringLiteral("hail shower"), Hail);
    dayList.insert(QStringLiteral("hail showers"), Hail);
    dayList.insert(QStringLiteral("hail"), Hail);
    dayList.insert(QStringLiteral("light snow"), LightSnow);
    dayList.insert(QStringLiteral("light snow shower"), Flurries);
    dayList.insert(QStringLiteral("light snow showers"), Flurries);
    dayList.insert(QStringLiteral("cloudy with light snow"), LightSnow);
    dayList.insert(QStringLiteral("heavy snow"), Snow);
    dayList.insert(QStringLiteral("heavy snow shower"), Snow);
    dayList.insert(QStringLiteral("heavy snow showers"), Snow);
    dayList.insert(QStringLiteral("cloudy with heavy snow"), Snow);
    dayList.insert(QStringLiteral("na"), NotAvailable);
    return dayList;
}

QMap<QString, IonInterface::ConditionIcons> UKMETIon::setupNightIconMappings() const
{
    QMap<QString, ConditionIcons> nightList;
    nightList.insert(QStringLiteral("clear"), ClearNight);
    nightList.insert(QStringLiteral("clear sky"), ClearNight);
    nightList.insert(QStringLiteral("clear intervals"), PartlyCloudyNight);
    nightList.insert(QStringLiteral("sunny intervals"), PartlyCloudyDay); // it's not really sunny
    nightList.insert(QStringLiteral("sunny"), ClearDay);
    nightList.insert(QStringLiteral("light cloud"), PartlyCloudyNight);
    nightList.insert(QStringLiteral("partly cloudy"), PartlyCloudyNight);
    nightList.insert(QStringLiteral("cloudy"), PartlyCloudyNight);
    nightList.insert(QStringLiteral("white cloud"), PartlyCloudyNight);
    nightList.insert(QStringLiteral("grey cloud"), Overcast);
    nightList.insert(QStringLiteral("thick cloud"), Overcast);
    nightList.insert(QStringLiteral("drizzle"), LightRain);
    nightList.insert(QStringLiteral("misty"), Mist);
    nightList.insert(QStringLiteral("mist"), Mist);
    nightList.insert(QStringLiteral("fog"), Mist);
    nightList.insert(QStringLiteral("foggy"), Mist);
    nightList.insert(QStringLiteral("tropical storm"), Thunderstorm);
    nightList.insert(QStringLiteral("hazy"), NotAvailable);
    nightList.insert(QStringLiteral("light shower"), Showers);
    nightList.insert(QStringLiteral("light rain shower"), Showers);
    nightList.insert(QStringLiteral("light showers"), Showers);
    nightList.insert(QStringLiteral("light rain"), Showers);
    nightList.insert(QStringLiteral("heavy rain"), Rain);
    nightList.insert(QStringLiteral("heavy showers"), Rain);
    nightList.insert(QStringLiteral("heavy shower"), Rain);
    nightList.insert(QStringLiteral("heavy rain shower"), Rain);
    nightList.insert(QStringLiteral("thundery shower"), Thunderstorm);
    nightList.insert(QStringLiteral("thunder storm"), Thunderstorm);
    nightList.insert(QStringLiteral("cloudy with sleet"), NotAvailable);
    nightList.insert(QStringLiteral("sleet shower"), NotAvailable);
    nightList.insert(QStringLiteral("sleet showers"), NotAvailable);
    nightList.insert(QStringLiteral("sleet"), NotAvailable);
    nightList.insert(QStringLiteral("cloudy with hail"), Hail);
    nightList.insert(QStringLiteral("hail shower"), Hail);
    nightList.insert(QStringLiteral("hail showers"), Hail);
    nightList.insert(QStringLiteral("hail"), Hail);
    nightList.insert(QStringLiteral("light snow"), LightSnow);
    nightList.insert(QStringLiteral("light snow shower"), Flurries);
    nightList.insert(QStringLiteral("light snow showers"), Flurries);
    nightList.insert(QStringLiteral("cloudy with light snow"), LightSnow);
    nightList.insert(QStringLiteral("heavy snow"), Snow);
    nightList.insert(QStringLiteral("heavy snow shower"), Snow);
    nightList.insert(QStringLiteral("heavy snow showers"), Snow);
    nightList.insert(QStringLiteral("cloudy with heavy snow"), Snow);
    nightList.insert(QStringLiteral("na"), NotAvailable);

    return nightList;
}

QMap<QString, IonInterface::ConditionIcons> const& UKMETIon::dayIcons() const
{
    static QMap<QString, ConditionIcons> const dval = setupDayIconMappings();
    return dval;
}

QMap<QString, IonInterface::ConditionIcons> const& UKMETIon::nightIcons() const
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

    const QStringList sourceAction = source.split(QLatin1Char('|'));

    // Guard: if the size of array is not 3 then we have bad data, return an error
    if (sourceAction.size() < 3) {
        setData(source, QStringLiteral("validate"), QStringLiteral("bbcukmet|malformed"));
        return true;
    }

    if (sourceAction[1] == QLatin1String("validate") && sourceAction.size() >= 3) {
        // Look for places to match
        findPlace(sourceAction[2], source);
        return true;
    }

    if (sourceAction[1] == QLatin1String("weather") && sourceAction.size() >= 3) {
        if (sourceAction.count() >= 3) {
            if (sourceAction[2].isEmpty()) {
                setData(source, QStringLiteral("validate"), QStringLiteral("bbcukmet|malformed"));
                return true;
            }
            m_place[QStringLiteral("bbcukmet|") +sourceAction[2]].XMLurl = sourceAction[3];
            getXMLData(sourceAction[0] + QLatin1Char('|') + sourceAction[2]);
            return true;
        }
        return false;

    }

    setData(source, QStringLiteral("validate"), QStringLiteral("bbcukmet|malformed"));
    return true;
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

    const QUrl url = m_place[source].XMLurl;

    KIO::TransferJob* getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none")); // Disable displaying cookies
    m_obsJobXml.insert(getJob, new QXmlStreamReader);
    m_obsJobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data,
            this, &UKMETIon::observation_slotDataArrived);
    connect(getJob, &KJob::result,
            this, &UKMETIon::observation_slotJobFinished);
}

// Parses city list and gets the correct city based on ID number
void UKMETIon::findPlace(const QString& place, const QString& source)
{
    /* There's a page= parameter, results are limited to 10 by page */
    const QUrl url(QLatin1String("http://www.bbc.com/locator/default/en-GB/search.json?search=")+place+
                   QLatin1String("&filter=international&postcode_unit=false&postcode_district=true"));

    KIO::TransferJob* getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none")); // Disable displaying cookies
    m_jobHtml.insert(getJob, new QByteArray());
    m_jobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data,
            this, &UKMETIon::setup_slotDataArrived);
    connect(getJob, &KJob::result,
            this, &UKMETIon::setup_slotJobFinished);

/*
    // Handle redirects for direct hit places.
    connect(getJob, SIGNAL(redirection(KIO::Job*,KUrl)),
            this, SLOT(setup_slotRedirected(KIO::Job*,KUrl)));
*/
}

void UKMETIon::getFiveDayForecast(const QString& source)
{

    QUrl xmlMap(m_place[source].forecastHTMLUrl);

    const QString stationID = xmlMap.path().section(QLatin1Char('/'), -1);

    m_place[source].XMLforecastURL = QStringLiteral("http://open.live.bbc.co.uk/weather/feeds/en/") + stationID + QStringLiteral("/3dayforecast.rss") + xmlMap.query();

    const QUrl url(m_place[source].XMLforecastURL);

    KIO::TransferJob* getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none")); // Disable displaying cookies
    m_forecastJobXml.insert(getJob, new QXmlStreamReader);
    m_forecastJobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data,
            this, &UKMETIon::forecast_slotDataArrived);
    connect(getJob, &KJob::result,
            this, &UKMETIon::forecast_slotJobFinished);
}

void UKMETIon::readSearchHTMLData(const QString& source, const QByteArray& html)
{
    int counter = 2;

    QJsonObject jsonDocumentObject = QJsonDocument::fromJson(html).object();

    if (!jsonDocumentObject.isEmpty()) {
        QJsonArray results = jsonDocumentObject.value(QStringLiteral("results")).toArray();

        foreach(const QJsonValue & resultValue, results) {
            QJsonObject result = resultValue.toObject();
            const QString id = result.value(QStringLiteral("id")).toString();
            const QString fullName = result.value(QStringLiteral("fullName")).toString();

            if (!id.isEmpty() && !fullName.isEmpty()) {
                const QString url = QStringLiteral("http://open.live.bbc.co.uk/weather/feeds/en/") + id + QStringLiteral("/observations.rss");

                QString tmp = QStringLiteral("bbcukmet|") + fullName;

                // Duplicate places can exist
                if (m_locations.contains(tmp)) {
                    tmp += QStringLiteral(" (#") + QString::number(counter) + QLatin1Char(')');
                    counter++;
                }
                m_place[tmp].XMLurl = url;
                m_place[tmp].place = fullName;
                m_locations.append(tmp);
            }
       }
    }

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
        setData(m_jobList[job], QStringLiteral("validate"), QStringLiteral("bbcukmet|timeout"));
        disconnectSource(m_jobList[job], this);
        m_jobList.remove(job);
        delete m_jobHtml[job];
        m_jobHtml.remove(job);
        return;
    }

    // If Redirected, don't go to this routine
    if (!m_locations.contains(QStringLiteral("bbcukmet|") + m_jobList[job])) {
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
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("rss"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("rss")) {
            break;
        }

        if (xml.isStartElement() && elementName == QLatin1String("channel")) {
            parseWeatherChannel(source, data, xml);
        }
    }
}

void UKMETIon::parsePlaceForecast(const QString &source, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("rss"));

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QLatin1String("channel")) {
            parseWeatherForecast(source, xml);
        }
    }
}

void UKMETIon::parseWeatherChannel(const QString& source, WeatherData& data, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("channel"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("channel")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("title")) {
                data.stationName = xml.readElementText().section(QStringLiteral("Observations for"), 1, 1).trimmed();
                data.stationName.replace(QStringLiteral("United Kingdom"), i18n("UK"));
                data.stationName.replace(QStringLiteral("United States of America"), i18n("USA"));

            } else if (elementName == QLatin1String("item")) {
                parseWeatherObservation(source, data, xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

void UKMETIon::parseWeatherForecast(const QString& source, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("channel"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("channel")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("item")) {
                parseFiveDayForecast(source, xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

void UKMETIon::parseWeatherObservation(const QString& source, WeatherData& data, QXmlStreamReader& xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("item"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("item")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("title")) {
                QString conditionString = xml.readElementText();

                // Get the observation time and condition
                int splitIndex = conditionString.lastIndexOf(QLatin1Char(':'));
                if (splitIndex >= 0) {
                    QString conditionData = conditionString.mid(splitIndex + 1); // Skip ':'
                    data.obsTime = conditionString.left(splitIndex);

//TODO: timezone parsing is not yet supported by QDateTime
#if 0
                    if (data.obsTime.contains('-')) {
                        // Saturday - 13:00 CET
                        // Saturday - 12:00 GMT
                        m_dateFormat = QDateTime::fromString(data.obsTime.section('-', 1, 1).trimmed(), "hh:mm ZZZ");
                        if (m_dateFormat.isValid()) {
                            data.iconPeriodHour = m_dateFormat.toString("hh").toInt();
                            data.iconPeriodMinute = m_dateFormat.toString("mm").toInt();
                        }
                    } else {
#endif
                        m_dateFormat = QDateTime();
#if 0
                    }
#endif

                    if (conditionData.contains(QLatin1Char(','))) {
                        data.condition = conditionData.section(QLatin1Char(','), 0, 0).trimmed();

                        if (data.condition == QLatin1String("null")) {
                            data.condition = QStringLiteral("N/A");
                        }
                    }
                }

            } else if (elementName == QLatin1String("link")) {
                m_place[source].forecastHTMLUrl = xml.readElementText();

            } else if (elementName == QLatin1String("description")) {
                QString observeString = xml.readElementText();
                const QStringList observeData = observeString.split(QLatin1Char(':'));
#ifdef __GNUC__
#warning FIXME: We should make this use a QRegExp but I need some help here :) -spstarr
#endif

                data.temperature_C = observeData[1].section(QChar(176), 0, 0).trimmed();

                if (data.temperature_C.contains(QStringLiteral("N/A")) ||
                    data.temperature_C.contains(QStringLiteral("null"))) {
                    data.temperature_C = i18n("N/A");
                }

                data.windDirection = observeData[2].section(QLatin1Char(','), 0, 0).trimmed();
                if (data.windDirection.contains(QStringLiteral("null"))) {
                    data.windDirection.clear();
                }

                data.windSpeed_miles = observeData[3].section(QLatin1Char(','), 0, 0).section(QLatin1Char(' '),1 ,1).remove(QStringLiteral("mph"));
                if (data.windSpeed_miles.contains(QStringLiteral("null"))) {
                    data.windSpeed_miles = QStringLiteral("N/A");
                }

                data.humidity = observeData[4].section(QLatin1Char(','), 0, 0).section(QLatin1Char(' '),1 ,1);
                if (data.humidity.endsWith(QLatin1Char('%'))) {
                    data.humidity.chop(1);
                }
                if (data.humidity.contains(QStringLiteral("null"))) {
                    data.humidity = QStringLiteral("N/A");
                }

                data.pressure = observeData[5].section(QLatin1Char(','), 0, 0).section(QLatin1Char(' '),1 ,1).section(QStringLiteral("mb"), 0, 0);
                if (data.pressure.contains(QStringLiteral("null"))) {
                    data.pressure = QStringLiteral("N/A");
                }

                data.pressureTendency = observeData[5].section(QLatin1Char(','), 1, 1).trimmed();

                data.visibilityStr = observeData[6].trimmed();

            } else if (elementName == QLatin1String("lat")) {
                const QString ordinate = xml.readElementText();
                data.latitude = ordinate.toDouble();
            } else if (elementName == QLatin1String("long")) {
                const QString ordinate = xml.readElementText();
                data.longitude = ordinate.toDouble();
            } else if (elementName == QLatin1String("georss:point")) {
                const QStringList ordinates = xml.readElementText().split(QLatin1Char(' '));
                data.latitude = ordinates[0].toDouble();
                data.longitude = ordinates[1].toDouble();
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
            if (xml.name() == QLatin1String("rss")) {
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
            if (xml.name() == QLatin1String("rss")) {
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
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("item"));

    // Flush out the old forecasts when updating.
    m_weatherData[source].forecasts.clear();

    WeatherData::ForecastInfo *forecast = new WeatherData::ForecastInfo;
    QString line;
    QString period;
    QString summary;
    QRegExp high(QStringLiteral("Maximum Temperature: (-?\\d+).C"), Qt::CaseInsensitive);
    QRegExp  low(QStringLiteral("Minimum Temperature: (-?\\d+).C"), Qt::CaseInsensitive);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.name() == QLatin1String("title")) {
            line = xml.readElementText().trimmed();
#ifdef __GNUC__
#warning FIXME: We should make this all use QRegExps in UKMETIon::parseFiveDayForecast() for forecast -spstarr
#endif

            const QString p = line.section(QLatin1Char(','), 0, 0);
            period = p.section(QLatin1Char(':'), 0, 0);
            summary = p.section(QLatin1Char(':'), 1, 1).trimmed();

            const QString temps = line.section(QLatin1Char(','), 1, 1);
            // Sometimes only one of min or max are reported
            if (high.indexIn(temps) == -1) {
                forecast->tempHigh = UNKNOWN_TEMPERATURE;
            } else {
                forecast->tempHigh = high.cap(1).toInt();
            }
            if (low.indexIn(temps) == -1) {
                forecast->tempLow = UNKNOWN_TEMPERATURE;
            } else {
                forecast->tempLow = low.cap(1).toInt();
            }

            forecast->period = period;
            forecast->iconName = getWeatherIcon(dayIcons(), summary.toLower());
            forecast->summary = i18nc("weather forecast", summary.toUtf8().data());
            qDebug() << "i18n summary string: " << forecast->summary;
            m_weatherData[source].forecasts.append(forecast);
            // prepare next
            forecast = new WeatherData::ForecastInfo;
        }
    }
    // remove unused
    delete forecast;
}

void UKMETIon::validate(const QString& source)
{
    if (m_locations.isEmpty()) {
        const QString invalidPlace = source.section(QLatin1Char('|'), 2, 2);
        if (m_place[QStringLiteral("bbcukmet|")+invalidPlace].place.isEmpty()) {
            setData(source, QStringLiteral("validate"),
                    QVariant(QStringLiteral("bbcukmet|invalid|multiple|") + invalidPlace));
        }
        return;
    }

    QString placeList;
    foreach(const QString &place, m_locations) {
        const QString p = place.section(QLatin1Char('|'), 1, 1);
        placeList.append(QStringLiteral("|place|") + p + QStringLiteral("|extra|") + m_place[place].XMLurl);
    }
    if (m_locations.count() > 1) {
        setData(source, QStringLiteral("validate"),
                QVariant(QStringLiteral("bbcukmet|valid|multiple") + placeList));
    } else {
        placeList[7] = placeList[7].toUpper();
        setData(source, QStringLiteral("validate"),
                QVariant(QStringLiteral("bbcukmet|valid|single") + placeList));
    }
    m_locations.clear();
}

void UKMETIon::updateWeather(const QString& source)
{
    QString weatherSource = source;
    // TODO: why the replacement here instead of just a new string?
    weatherSource.replace(QStringLiteral("bbcukmet|"), QStringLiteral("bbcukmet|weather|"));
    weatherSource.append(QLatin1Char('|') + m_place[source].XMLurl);

    QMap<QString, QString> dataFields;

    Plasma::DataEngine::Data data;

    data.insert(QStringLiteral("Place"), place(source));
    data.insert(QStringLiteral("Station"), station(source));
    data.insert(QStringLiteral("Observation Period"), observationTime(source));
    data.insert(QStringLiteral("Current Conditions"), i18nc("weather condition", condition(source).toUtf8().data()));
    qDebug() << "i18n condition string: " << i18nc("weather condition", condition(source).toUtf8().data());

    const double lati = periodLatitude(source);
    const double longi = periodLongitude(source);
//TODO: Port to Plasma5, needs also fix of m_dateFormat estimation
#if 0
    if (m_dateFormat.isValid()) {
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
    } else {
#endif
        data.insert(QStringLiteral("Condition Icon"), getWeatherIcon(dayIcons(), condition(source)));
#if 0
    }
#endif

    data.insert(QStringLiteral("Latitude"), lati);
    data.insert(QStringLiteral("Longitude"), longi);

    dataFields = humidity(source);
    data.insert(QStringLiteral("Humidity"), dataFields[QStringLiteral("humidity")]);
    data.insert(QStringLiteral("Humidity Unit"), dataFields[QStringLiteral("humidityUnit")]);

    data.insert(QStringLiteral("Visibility"), visibility(source));

    dataFields = temperature(source);
    data.insert(QStringLiteral("Temperature"), dataFields[QStringLiteral("temperature")]);
    data.insert(QStringLiteral("Temperature Unit"), dataFields[QStringLiteral("temperatureUnit")]);

    dataFields = pressure(source);
    data.insert(QStringLiteral("Pressure"), dataFields[QStringLiteral("pressure")]);
    data.insert(QStringLiteral("Pressure Unit"), dataFields[QStringLiteral("pressureUnit")]);
    data.insert(QStringLiteral("Pressure Tendency"), dataFields[QStringLiteral("pressureTendency")]);

    dataFields = wind(source);
    data.insert(QStringLiteral("Wind Speed"), dataFields[QStringLiteral("windSpeed")]);
    data.insert(QStringLiteral("Wind Speed Unit"), dataFields[QStringLiteral("windUnit")]);
    data.insert(QStringLiteral("Wind Direction"), dataFields[QStringLiteral("windDirection")]);

    // 5 Day forecast info
    const QVector<QString> forecastList = forecasts(source);

    // Set number of forecasts per day/night supported
    data.insert(QStringLiteral("Total Weather Days"), m_weatherData[source].forecasts.size());

    int i = 0;
    foreach(const QString &forecastItem, forecastList) {
        data.insert(QStringLiteral("Short Forecast Day %1").arg(i), forecastItem);
        i++;
    }

    data.insert(QStringLiteral("Credit"), i18n("Data from BBC Weather"));
    data.insert(QStringLiteral("Credit Url"), m_place[source].forecastHTMLUrl);

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

    temperatureInfo.insert(QStringLiteral("temperature"), QString(m_weatherData[source].temperature_C));
    temperatureInfo.insert(QStringLiteral("temperatureUnit"), QString::number(KUnitConversion::Celsius));
    return temperatureInfo;
}

QMap<QString, QString> UKMETIon::wind(const QString& source) const
{
    QMap<QString, QString> windInfo;
    if (m_weatherData[source].windSpeed_miles == QLatin1String("N/A")) {
        windInfo.insert(QStringLiteral("windSpeed"), i18n("N/A"));
        windInfo.insert(QStringLiteral("windUnit"), QString::number(KUnitConversion::NoUnit));
    } else {
        windInfo.insert(QStringLiteral("windSpeed"), m_weatherData[source].windSpeed_miles);
        windInfo.insert(QStringLiteral("windUnit"), QString::number(KUnitConversion::MilePerHour));
    }
    if (m_weatherData[source].windDirection.isEmpty()) {
        windInfo.insert(QStringLiteral("windDirection"), i18n("N/A"));
    } else {
        windInfo.insert(QStringLiteral("windDirection"), i18nc("wind direction", m_weatherData[source].windDirection.toUtf8().data()));
    }
    return windInfo;
}

QMap<QString, QString> UKMETIon::humidity(const QString& source) const
{
    QMap<QString, QString> humidityInfo;
    if (m_weatherData[source].humidity != QLatin1String("N/A")) {
        humidityInfo.insert(QStringLiteral("humidity"), m_weatherData[source].humidity);
        humidityInfo.insert(QStringLiteral("humidityUnit"), QString::number(KUnitConversion::Percent));
    } else {
        humidityInfo.insert(QStringLiteral("humidity"), i18n("N/A"));
        humidityInfo.insert(QStringLiteral("humidityUnit"), QString::number(KUnitConversion::NoUnit));
    }

    return humidityInfo;
}

QString UKMETIon::visibility(const QString& source) const
{
    return i18nc("visibility", m_weatherData[source].visibilityStr.toUtf8().data());
}

QMap<QString, QString> UKMETIon::pressure(const QString& source) const
{
    QMap<QString, QString> pressureInfo;
    if (m_weatherData[source].pressure == QLatin1String("N/A")) {
        pressureInfo.insert(QStringLiteral("pressure"), i18n("N/A"));
        pressureInfo.insert(QStringLiteral("pressureUnit"), QString::number(KUnitConversion::NoUnit));
        pressureInfo.insert(QStringLiteral("pressureTendency"), i18n("N/A"));
        return pressureInfo;
    }

    pressureInfo.insert(QStringLiteral("pressure"), QString(m_weatherData[source].pressure));
    pressureInfo.insert(QStringLiteral("pressureUnit"), QString::number(KUnitConversion::Millibar));

    pressureInfo.insert(QStringLiteral("pressureTendency"), i18nc("pressure tendency", m_weatherData[source].pressureTendency.toUtf8().data()));
    return pressureInfo;
}

QVector<QString> UKMETIon::forecasts(const QString& source)
{
    QVector<QString> forecastData;

    foreach(const WeatherData::ForecastInfo *forecastInfo, m_weatherData[source].forecasts) {
        QString period = forecastInfo->period;
        period.replace(QStringLiteral("Saturday"), i18nc("Short for Saturday", "Sat"));
        period.replace(QStringLiteral("Sunday"), i18nc("Short for Sunday", "Sun"));
        period.replace(QStringLiteral("Monday"), i18nc("Short for Monday", "Mon"));
        period.replace(QStringLiteral("Tuesday"), i18nc("Short for Tuesday", "Tue"));
        period.replace(QStringLiteral("Wednesday"), i18nc("Short for Wednesday", "Wed"));
        period.replace(QStringLiteral("Thursday"), i18nc("Short for Thursday", "Thu"));
        period.replace(QStringLiteral("Friday"), i18nc("Short for Friday", "Fri"));

        int tempHigh = forecastInfo->tempHigh;
        QString tempHighStr = (tempHigh == UNKNOWN_TEMPERATURE)
                              ? QStringLiteral("N/A")
                              : QString::number(tempHigh);

        int tempLow = forecastInfo->tempLow;
        QString tempLowStr = (tempLow == UNKNOWN_TEMPERATURE)
                             ? QStringLiteral("N/A")
                             : QString::number(tempLow);

        forecastData.append(QStringLiteral("%1|%2|%3|%4|%5|%6")
                            .arg(period, forecastInfo->iconName, forecastInfo->summary,
                                 tempHighStr, tempLowStr, QStringLiteral("N/U")));
        //.arg(forecastInfo->windSpeed)
        //arg(forecastInfo->windDirection));
    }

    return forecastData;
}


K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(bbcukmet, UKMETIon, "ion-bbcukmet.json")

#include "ion_bbcukmet.moc"
