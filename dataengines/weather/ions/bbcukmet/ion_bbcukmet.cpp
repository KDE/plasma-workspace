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

#include "ion_bbcukmetdebug.h"

#include <KIO/Job>
#include <KLocalizedString>
#include <KUnitConversion/Converter>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimeZone>
#include <QXmlStreamReader>

WeatherData::WeatherData()
    : stationLatitude(qQNaN())
    , stationLongitude(qQNaN())
    , condition()
    , temperature_C(qQNaN())
    , windSpeed_miles(qQNaN())
    , humidity(qQNaN())
    , pressure(qQNaN())
{
}

WeatherData::ForecastInfo::ForecastInfo()
    : tempHigh(qQNaN())
    , tempLow(qQNaN())
    , windSpeed(qQNaN())
{
}

// ctor, dtor
UKMETIon::UKMETIon(QObject *parent, const QVariantList &args)
    : IonInterface(parent, args)

{
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
    QHash<QString, WeatherData>::iterator it = m_weatherData.begin(), end = m_weatherData.end();
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

    return QMap<QString, ConditionIcons>{
        {QStringLiteral("sunny"), ClearDay},
        //    { QStringLiteral("sunny night"), ClearNight },
        {QStringLiteral("clear"), ClearDay},
        {QStringLiteral("clear sky"), ClearDay},
        {QStringLiteral("sunny intervals"), PartlyCloudyDay},
        //    { QStringLiteral("sunny intervals night"), ClearNight },
        {QStringLiteral("light cloud"), PartlyCloudyDay},
        {QStringLiteral("partly cloudy"), PartlyCloudyDay},
        {QStringLiteral("cloudy"), PartlyCloudyDay},
        {QStringLiteral("white cloud"), PartlyCloudyDay},
        {QStringLiteral("grey cloud"), Overcast},
        {QStringLiteral("thick cloud"), Overcast},
        //    { QStringLiteral("low level cloud"), NotAvailable },
        //    { QStringLiteral("medium level cloud"), NotAvailable },
        //    { QStringLiteral("sandstorm"), NotAvailable },
        {QStringLiteral("drizzle"), LightRain},
        {QStringLiteral("misty"), Mist},
        {QStringLiteral("mist"), Mist},
        {QStringLiteral("fog"), Mist},
        {QStringLiteral("foggy"), Mist},
        {QStringLiteral("tropical storm"), Thunderstorm},
        {QStringLiteral("hazy"), NotAvailable},
        {QStringLiteral("light shower"), Showers},
        {QStringLiteral("light rain shower"), Showers},
        {QStringLiteral("light rain showers"), Showers},
        {QStringLiteral("light showers"), Showers},
        {QStringLiteral("light rain"), Showers},
        {QStringLiteral("heavy rain"), Rain},
        {QStringLiteral("heavy showers"), Rain},
        {QStringLiteral("heavy shower"), Rain},
        {QStringLiteral("heavy rain shower"), Rain},
        {QStringLiteral("heavy rain showers"), Rain},
        {QStringLiteral("thundery shower"), Thunderstorm},
        {QStringLiteral("thundery showers"), Thunderstorm},
        {QStringLiteral("thunderstorm"), Thunderstorm},
        {QStringLiteral("cloudy with sleet"), RainSnow},
        {QStringLiteral("sleet shower"), RainSnow},
        {QStringLiteral("sleet showers"), RainSnow},
        {QStringLiteral("sleet"), RainSnow},
        {QStringLiteral("cloudy with hail"), Hail},
        {QStringLiteral("hail shower"), Hail},
        {QStringLiteral("hail showers"), Hail},
        {QStringLiteral("hail"), Hail},
        {QStringLiteral("light snow"), LightSnow},
        {QStringLiteral("light snow shower"), Flurries},
        {QStringLiteral("light snow showers"), Flurries},
        {QStringLiteral("cloudy with light snow"), LightSnow},
        {QStringLiteral("heavy snow"), Snow},
        {QStringLiteral("heavy snow shower"), Snow},
        {QStringLiteral("heavy snow showers"), Snow},
        {QStringLiteral("cloudy with heavy snow"), Snow},
        {QStringLiteral("na"), NotAvailable},
    };
}

QMap<QString, IonInterface::ConditionIcons> UKMETIon::setupNightIconMappings() const
{
    return QMap<QString, ConditionIcons>{
        {QStringLiteral("clear"), ClearNight},
        {QStringLiteral("clear sky"), ClearNight},
        {QStringLiteral("clear intervals"), PartlyCloudyNight},
        {QStringLiteral("sunny intervals"), PartlyCloudyDay}, // it's not really sunny
        {QStringLiteral("sunny"), ClearDay},
        {QStringLiteral("light cloud"), PartlyCloudyNight},
        {QStringLiteral("partly cloudy"), PartlyCloudyNight},
        {QStringLiteral("cloudy"), PartlyCloudyNight},
        {QStringLiteral("white cloud"), PartlyCloudyNight},
        {QStringLiteral("grey cloud"), Overcast},
        {QStringLiteral("thick cloud"), Overcast},
        {QStringLiteral("drizzle"), LightRain},
        {QStringLiteral("misty"), Mist},
        {QStringLiteral("mist"), Mist},
        {QStringLiteral("fog"), Mist},
        {QStringLiteral("foggy"), Mist},
        {QStringLiteral("tropical storm"), Thunderstorm},
        {QStringLiteral("hazy"), NotAvailable},
        {QStringLiteral("light shower"), Showers},
        {QStringLiteral("light rain shower"), Showers},
        {QStringLiteral("light rain showers"), Showers},
        {QStringLiteral("light showers"), Showers},
        {QStringLiteral("light rain"), Showers},
        {QStringLiteral("heavy rain"), Rain},
        {QStringLiteral("heavy showers"), Rain},
        {QStringLiteral("heavy shower"), Rain},
        {QStringLiteral("heavy rain shower"), Rain},
        {QStringLiteral("heavy rain showers"), Rain},
        {QStringLiteral("thundery shower"), Thunderstorm},
        {QStringLiteral("thundery showers"), Thunderstorm},
        {QStringLiteral("thunderstorm"), Thunderstorm},
        {QStringLiteral("cloudy with sleet"), RainSnow},
        {QStringLiteral("sleet shower"), RainSnow},
        {QStringLiteral("sleet showers"), RainSnow},
        {QStringLiteral("sleet"), RainSnow},
        {QStringLiteral("cloudy with hail"), Hail},
        {QStringLiteral("hail shower"), Hail},
        {QStringLiteral("hail showers"), Hail},
        {QStringLiteral("hail"), Hail},
        {QStringLiteral("light snow"), LightSnow},
        {QStringLiteral("light snow shower"), Flurries},
        {QStringLiteral("light snow showers"), Flurries},
        {QStringLiteral("cloudy with light snow"), LightSnow},
        {QStringLiteral("heavy snow"), Snow},
        {QStringLiteral("heavy snow shower"), Snow},
        {QStringLiteral("heavy snow showers"), Snow},
        {QStringLiteral("cloudy with heavy snow"), Snow},
        {QStringLiteral("na"), NotAvailable},
    };
}

QMap<QString, IonInterface::WindDirections> UKMETIon::setupWindIconMappings() const
{
    return QMap<QString, WindDirections>{
        {QStringLiteral("northerly"), N},
        {QStringLiteral("north north easterly"), NNE},
        {QStringLiteral("north easterly"), NE},
        {QStringLiteral("east north easterly"), ENE},
        {QStringLiteral("easterly"), E},
        {QStringLiteral("east south easterly"), ESE},
        {QStringLiteral("south easterly"), SE},
        {QStringLiteral("south south easterly"), SSE},
        {QStringLiteral("southerly"), S},
        {QStringLiteral("south south westerly"), SSW},
        {QStringLiteral("south westerly"), SW},
        {QStringLiteral("west south westerly"), WSW},
        {QStringLiteral("westerly"), W},
        {QStringLiteral("west north westerly"), WNW},
        {QStringLiteral("north westerly"), NW},
        {QStringLiteral("north north westerly"), NNW},
        {QStringLiteral("calm"), VR},
    };
}

QMap<QString, IonInterface::ConditionIcons> const &UKMETIon::dayIcons() const
{
    static QMap<QString, ConditionIcons> const dval = setupDayIconMappings();
    return dval;
}

QMap<QString, IonInterface::ConditionIcons> const &UKMETIon::nightIcons() const
{
    static QMap<QString, ConditionIcons> const nval = setupNightIconMappings();
    return nval;
}

QMap<QString, IonInterface::WindDirections> const &UKMETIon::windIcons() const
{
    static QMap<QString, WindDirections> const wval = setupWindIconMappings();
    return wval;
}

// Get a specific Ion's data
bool UKMETIon::updateIonSource(const QString &source)
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

            XMLMapInfo &place = m_place[QLatin1String("bbcukmet|") + sourceAction[2]];

            // backward compatibility after rss feed url change in 2018/03
            place.sourceExtraArg = sourceAction[3];
            if (place.sourceExtraArg.startsWith(QLatin1String("http://open.live.bbc.co.uk/"))) {
                // Old data source id stored the full (now outdated) observation feed url
                // http://open.live.bbc.co.uk/weather/feeds/en/STATIOID/observations.rss
                // as extra argument, so extract the id from that
                place.stationId = place.sourceExtraArg.section(QLatin1Char('/'), -2, -2);
            } else {
                place.stationId = place.sourceExtraArg;
            }
            getXMLData(sourceAction[0] + QLatin1Char('|') + sourceAction[2]);
            return true;
        }
        return false;
    }

    setData(source, QStringLiteral("validate"), QStringLiteral("bbcukmet|malformed"));
    return true;
}

// Gets specific city XML data
void UKMETIon::getXMLData(const QString &source)
{
    for (const QString &fetching : qAsConst(m_obsJobList)) {
        if (fetching == source) {
            // already getting this source and awaiting the data
            return;
        }
    }

    const QUrl url(QStringLiteral("https://weather-broker-cdn.api.bbci.co.uk/en/observation/rss/") + m_place[source].stationId);

    KIO::TransferJob *getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none")); // Disable displaying cookies
    m_obsJobXml.insert(getJob, new QXmlStreamReader);
    m_obsJobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data, this, &UKMETIon::observation_slotDataArrived);
    connect(getJob, &KJob::result, this, &UKMETIon::observation_slotJobFinished);
}

// Parses city list and gets the correct city based on ID number
void UKMETIon::findPlace(const QString &place, const QString &source)
{
    /* There's a page= parameter, results are limited to 10 by page */
    const QUrl url(QLatin1String("https://www.bbc.com/locator/default/en-GB/search.json?search=") + place
                   + QLatin1String("&filter=international&postcode_unit=false&postcode_district=true"));

    KIO::TransferJob *getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none")); // Disable displaying cookies
    m_jobHtml.insert(getJob, new QByteArray());
    m_jobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data, this, &UKMETIon::setup_slotDataArrived);
    connect(getJob, &KJob::result, this, &UKMETIon::setup_slotJobFinished);
}

void UKMETIon::getFiveDayForecast(const QString &source)
{
    XMLMapInfo &place = m_place[source];

    const QUrl url(QStringLiteral("https://weather-broker-cdn.api.bbci.co.uk/en/forecast/rss/3day/") + place.stationId);

    KIO::TransferJob *getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none")); // Disable displaying cookies
    m_forecastJobXml.insert(getJob, new QXmlStreamReader);
    m_forecastJobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data, this, &UKMETIon::forecast_slotDataArrived);
    connect(getJob, &KJob::result, this, &UKMETIon::forecast_slotJobFinished);
}

void UKMETIon::readSearchHTMLData(const QString &source, const QByteArray &html)
{
    int counter = 2;

    QJsonObject jsonDocumentObject = QJsonDocument::fromJson(html).object();

    if (!jsonDocumentObject.isEmpty()) {
        const QJsonArray results = jsonDocumentObject.value(QStringLiteral("results")).toArray();

        for (const QJsonValue &resultValue : results) {
            QJsonObject result = resultValue.toObject();
            const QString id = result.value(QStringLiteral("id")).toString();
            const QString fullName = result.value(QStringLiteral("fullName")).toString();

            if (!id.isEmpty() && !fullName.isEmpty()) {
                QString tmp = QLatin1String("bbcukmet|") + fullName;

                // Duplicate places can exist
                if (m_locations.contains(tmp)) {
                    tmp += QLatin1String(" (#") + QString::number(counter) + QLatin1Char(')');
                    counter++;
                }
                XMLMapInfo &place = m_place[tmp];
                place.stationId = id;
                place.place = fullName;
                m_locations.append(tmp);
            }
        }
    }

    validate(source);
}

// handle when no XML tag is found
void UKMETIon::parseUnknownElement(QXmlStreamReader &xml) const
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
    if (job->error() == KIO::ERR_SERVER_TIMEOUT) {
        setData(m_jobList[job], QStringLiteral("validate"), QStringLiteral("bbcukmet|timeout"));
        disconnectSource(m_jobList[job], this);
        m_jobList.remove(job);
        delete m_jobHtml[job];
        m_jobHtml.remove(job);
        return;
    }

    // If Redirected, don't go to this routine
    if (!m_locations.contains(QLatin1String("bbcukmet|") + m_jobList[job])) {
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

void UKMETIon::parsePlaceObservation(const QString &source, WeatherData &data, QXmlStreamReader &xml)
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

void UKMETIon::parsePlaceForecast(const QString &source, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("rss"));

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QLatin1String("channel")) {
            parseWeatherForecast(source, xml);
        }
    }
}

void UKMETIon::parseWeatherChannel(const QString &source, WeatherData &data, QXmlStreamReader &xml)
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

void UKMETIon::parseWeatherForecast(const QString &source, QXmlStreamReader &xml)
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
            } else if (elementName == QLatin1String("link") && xml.namespaceUri().isEmpty()) {
                m_place[source].forecastHTMLUrl = xml.readElementText();
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

void UKMETIon::parseWeatherObservation(const QString &source, WeatherData &data, QXmlStreamReader &xml)
{
    Q_UNUSED(source);

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

                    if (data.obsTime.contains(QLatin1Char('-'))) {
                        // Saturday - 13:00 CET
                        // Saturday - 12:00 GMT
                        // timezone parsing is not yet supported by QDateTime, also is there just a dayname
                        // so try manually
                        // guess date from day
                        const QString dayString = data.obsTime.section(QLatin1Char('-'), 0, 0).trimmed();
                        QDate date = QDate::currentDate();
                        const QString dayFormat = QStringLiteral("dddd");
                        const int testDayJumps[4] = {
                            -1, // first to weekday yesterday
                            2, // then to weekday tomorrow
                            -3, // then to weekday before yesterday, not sure if such day offset can happen?
                            4, // then to weekday after tomorrow, not sure if such day offset can happen?
                        };
                        const int dayJumps = sizeof(testDayJumps) / sizeof(testDayJumps[0]);
                        QLocale cLocale = QLocale::c();
                        int dayJump = 0;
                        while (true) {
                            if (cLocale.toString(date, dayFormat) == dayString) {
                                break;
                            }

                            if (dayJump >= dayJumps) {
                                // no weekday found near-by, set date invalid
                                date = QDate();
                                break;
                            }
                            date = date.addDays(testDayJumps[dayJump]);
                            ++dayJump;
                        }

                        if (date.isValid()) {
                            const QString timeString = data.obsTime.section(QLatin1Char('-'), 1, 1).trimmed();
                            const QTime time = QTime::fromString(timeString.section(QLatin1Char(' '), 0, 0), QStringLiteral("hh:mm"));
                            const QTimeZone timeZone = QTimeZone(timeString.section(QLatin1Char(' '), 1, 1).toUtf8());
                            // TODO: if non-IANA timezone id is not known, try to guess timezone from other data

                            if (time.isValid() && timeZone.isValid()) {
                                data.observationDateTime = QDateTime(date, time, timeZone);
                            }
                        }
                    }

                    if (conditionData.contains(QLatin1Char(','))) {
                        data.condition = conditionData.section(QLatin1Char(','), 0, 0).trimmed();

                        if (data.condition == QLatin1String("null") || data.condition == QLatin1String("Not Available")) {
                            data.condition.clear();
                        }
                    }
                }

            } else if (elementName == QLatin1String("description")) {
                QString observeString = xml.readElementText();
                const QStringList observeData = observeString.split(QLatin1Char(':'));

                // FIXME: We should make this use a QRegExp but I need some help here :) -spstarr

                QString temperature_C = observeData[1].section(QChar(176), 0, 0).trimmed();
                parseFloat(data.temperature_C, temperature_C);

                data.windDirection = observeData[2].section(QLatin1Char(','), 0, 0).trimmed();
                if (data.windDirection.contains(QLatin1String("null"))) {
                    data.windDirection.clear();
                }

                QString windSpeed_miles = observeData[3].section(QLatin1Char(','), 0, 0).section(QLatin1Char(' '), 1, 1).remove(QStringLiteral("mph"));
                parseFloat(data.windSpeed_miles, windSpeed_miles);

                QString humidity = observeData[4].section(QLatin1Char(','), 0, 0).section(QLatin1Char(' '), 1, 1);
                if (humidity.endsWith(QLatin1Char('%'))) {
                    humidity.chop(1);
                }
                parseFloat(data.humidity, humidity);

                QString pressure = observeData[5].section(QLatin1Char(','), 0, 0).section(QLatin1Char(' '), 1, 1).section(QStringLiteral("mb"), 0, 0);
                parseFloat(data.pressure, pressure);

                data.pressureTendency = observeData[5].section(QLatin1Char(','), 1, 1).toLower().trimmed();
                if (data.pressureTendency == QLatin1String("no change")) {
                    data.pressureTendency = QStringLiteral("steady");
                }

                data.visibilityStr = observeData[6].trimmed();
                if (data.visibilityStr == QLatin1String("--")) {
                    data.visibilityStr.clear();
                }

            } else if (elementName == QLatin1String("lat")) {
                const QString ordinate = xml.readElementText();
                data.stationLatitude = ordinate.toDouble();
            } else if (elementName == QLatin1String("long")) {
                const QString ordinate = xml.readElementText();
                data.stationLongitude = ordinate.toDouble();
            } else if (elementName == QLatin1String("point") && xml.namespaceUri() == QLatin1String("http://www.georss.org/georss")) {
                const QStringList ordinates = xml.readElementText().split(QLatin1Char(' '));
                data.stationLatitude = ordinates[0].toDouble();
                data.stationLongitude = ordinates[1].toDouble();
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

bool UKMETIon::readObservationXMLData(const QString &source, QXmlStreamReader &xml)
{
    WeatherData data;
    data.isForecastsDataPending = true;
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

    bool solarDataSourceNeedsConnect = false;
    Plasma::DataEngine *timeEngine = dataEngine(QStringLiteral("time"));
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

    // Get the 5 day forecast info next.
    getFiveDayForecast(source);

    return !xml.error();
}

bool UKMETIon::readFiveDayForecastXMLData(const QString &source, QXmlStreamReader &xml)
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
    if (!haveFiveDay)
        return false;
    updateWeather(source);
    return !xml.error();
}

void UKMETIon::parseFiveDayForecast(const QString &source, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("item"));

    WeatherData &weatherData = m_weatherData[source];
    QVector<WeatherData::ForecastInfo *> &forecasts = weatherData.forecasts;

    // Flush out the old forecasts when updating.
    forecasts.clear();

    WeatherData::ForecastInfo *forecast = new WeatherData::ForecastInfo;
    QString line;
    QString period;
    QString summary;
    const QRegularExpression high(QStringLiteral("Maximum Temperature: (-?\\d+).C"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression low(QStringLiteral("Minimum Temperature: (-?\\d+).C"), QRegularExpression::CaseInsensitiveOption);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.name() == QLatin1String("title")) {
            line = xml.readElementText().trimmed();

            // FIXME: We should make this all use QRegExps in UKMETIon::parseFiveDayForecast() for forecast -spstarr

            const QString p = line.section(QLatin1Char(','), 0, 0);
            period = p.section(QLatin1Char(':'), 0, 0);
            summary = p.section(QLatin1Char(':'), 1, 1).trimmed();

            const QString temps = line.section(QLatin1Char(','), 1, 1);
            // Sometimes only one of min or max are reported
            QRegularExpressionMatch rmatch;
            if (temps.contains(high, &rmatch)) {
                parseFloat(forecast->tempHigh, rmatch.captured(1));
            }
            if (temps.contains(low, &rmatch)) {
                parseFloat(forecast->tempLow, rmatch.captured(1));
            }

            const QString summaryLC = summary.toLower();
            forecast->period = period;
            if (forecast->period == QLatin1String("Tonight")) {
                forecast->iconName = getWeatherIcon(nightIcons(), summaryLC);
            } else {
                forecast->iconName = getWeatherIcon(dayIcons(), summaryLC);
            }
            // db uses original strings normalized to lowercase, but we prefer the unnormalized if without translation
            const QString summaryTranslated = i18nc("weather forecast", summaryLC.toUtf8().data());
            forecast->summary = (summaryTranslated != summaryLC) ? summaryTranslated : summary;
            qCDebug(IONENGINE_BBCUKMET) << "i18n summary string: " << forecast->summary;
            forecasts.append(forecast);
            // prepare next
            forecast = new WeatherData::ForecastInfo;
        }
    }

    weatherData.isForecastsDataPending = false;

    // remove unused
    delete forecast;
}

void UKMETIon::parseFloat(float &value, const QString &string)
{
    bool ok = false;
    const float result = string.toFloat(&ok);
    if (ok) {
        value = result;
    }
}

void UKMETIon::validate(const QString &source)
{
    if (m_locations.isEmpty()) {
        const QString invalidPlace = source.section(QLatin1Char('|'), 2, 2);
        if (m_place[QStringLiteral("bbcukmet|") + invalidPlace].place.isEmpty()) {
            setData(source, QStringLiteral("validate"), QVariant(QStringLiteral("bbcukmet|invalid|multiple|") + invalidPlace));
        }
        return;
    }

    QString placeList;
    for (const QString &place : qAsConst(m_locations)) {
        const QString p = place.section(QLatin1Char('|'), 1, 1);
        placeList.append(QStringLiteral("|place|") + p + QStringLiteral("|extra|") + m_place[place].stationId);
    }
    if (m_locations.count() > 1) {
        setData(source, QStringLiteral("validate"), QVariant(QStringLiteral("bbcukmet|valid|multiple") + placeList));
    } else {
        placeList[7] = placeList[7].toUpper();
        setData(source, QStringLiteral("validate"), QVariant(QStringLiteral("bbcukmet|valid|single") + placeList));
    }
    m_locations.clear();
}

void UKMETIon::updateWeather(const QString &source)
{
    const WeatherData &weatherData = m_weatherData[source];

    if (weatherData.isForecastsDataPending || weatherData.isSolarDataPending) {
        return;
    }

    const XMLMapInfo &place = m_place[source];

    QString weatherSource = source;
    // TODO: why the replacement here instead of just a new string?
    weatherSource.replace(QStringLiteral("bbcukmet|"), QStringLiteral("bbcukmet|weather|"));
    weatherSource.append(QLatin1Char('|') + place.sourceExtraArg);

    Plasma::DataEngine::Data data;

    // work-around for buggy observation RSS feed missing the station name
    QString stationName = weatherData.stationName;
    if (stationName.isEmpty() || stationName == QLatin1Char(',')) {
        stationName = source.section(QLatin1Char('|'), 1, 1);
    }

    data.insert(QStringLiteral("Place"), stationName);
    data.insert(QStringLiteral("Station"), stationName);
    if (weatherData.observationDateTime.isValid()) {
        data.insert(QStringLiteral("Observation Timestamp"), weatherData.observationDateTime);
    }
    if (!weatherData.obsTime.isEmpty()) {
        data.insert(QStringLiteral("Observation Period"), weatherData.obsTime);
    }
    if (!weatherData.condition.isEmpty()) {
        // db uses original strings normalized to lowercase, but we prefer the unnormalized if without translation
        const QString conditionLC = weatherData.condition.toLower();
        const QString conditionTranslated = i18nc("weather condition", conditionLC.toUtf8().data());
        data.insert(QStringLiteral("Current Conditions"), (conditionTranslated != conditionLC) ? conditionTranslated : weatherData.condition);
    }
    //     qCDebug(IONENGINE_BBCUKMET) << "i18n condition string: " << i18nc("weather condition", weatherData.condition.toUtf8().data());

    const bool stationCoordsValid = (!qIsNaN(weatherData.stationLatitude) && !qIsNaN(weatherData.stationLongitude));

    if (stationCoordsValid) {
        data.insert(QStringLiteral("Latitude"), weatherData.stationLatitude);
        data.insert(QStringLiteral("Longitude"), weatherData.stationLongitude);
    }

    data.insert(QStringLiteral("Condition Icon"), getWeatherIcon(weatherData.isNight ? nightIcons() : dayIcons(), weatherData.condition));

    if (!qIsNaN(weatherData.humidity)) {
        data.insert(QStringLiteral("Humidity"), weatherData.humidity);
        data.insert(QStringLiteral("Humidity Unit"), KUnitConversion::Percent);
    }

    if (!weatherData.visibilityStr.isEmpty()) {
        data.insert(QStringLiteral("Visibility"), i18nc("visibility", weatherData.visibilityStr.toUtf8().data()));
        data.insert(QStringLiteral("Visibility Unit"), KUnitConversion::NoUnit);
    }

    if (!qIsNaN(weatherData.temperature_C)) {
        data.insert(QStringLiteral("Temperature"), weatherData.temperature_C);
    }

    // Used for all temperatures
    data.insert(QStringLiteral("Temperature Unit"), KUnitConversion::Celsius);

    if (!qIsNaN(weatherData.pressure)) {
        data.insert(QStringLiteral("Pressure"), weatherData.pressure);
        data.insert(QStringLiteral("Pressure Unit"), KUnitConversion::Millibar);
        if (!weatherData.pressureTendency.isEmpty()) {
            data.insert(QStringLiteral("Pressure Tendency"), weatherData.pressureTendency);
        }
    }

    if (!qIsNaN(weatherData.windSpeed_miles)) {
        data.insert(QStringLiteral("Wind Speed"), weatherData.windSpeed_miles);
        data.insert(QStringLiteral("Wind Speed Unit"), KUnitConversion::MilePerHour);
        if (!weatherData.windDirection.isEmpty()) {
            data.insert(QStringLiteral("Wind Direction"), getWindDirectionIcon(windIcons(), weatherData.windDirection.toLower()));
        }
    }

    // 5 Day forecast info
    const QVector<WeatherData::ForecastInfo *> &forecasts = weatherData.forecasts;

    // Set number of forecasts per day/night supported
    data.insert(QStringLiteral("Total Weather Days"), forecasts.size());

    int i = 0;
    for (const WeatherData::ForecastInfo *forecastInfo : forecasts) {
        QString period = forecastInfo->period;
        // same day
        period.replace(QStringLiteral("Today"), i18nc("Short for Today", "Today"));
        period.replace(QStringLiteral("Tonight"), i18nc("Short for Tonight", "Tonight"));
        // upcoming days
        period.replace(QStringLiteral("Saturday"), i18nc("Short for Saturday", "Sat"));
        period.replace(QStringLiteral("Sunday"), i18nc("Short for Sunday", "Sun"));
        period.replace(QStringLiteral("Monday"), i18nc("Short for Monday", "Mon"));
        period.replace(QStringLiteral("Tuesday"), i18nc("Short for Tuesday", "Tue"));
        period.replace(QStringLiteral("Wednesday"), i18nc("Short for Wednesday", "Wed"));
        period.replace(QStringLiteral("Thursday"), i18nc("Short for Thursday", "Thu"));
        period.replace(QStringLiteral("Friday"), i18nc("Short for Friday", "Fri"));

        const QString tempHigh = qIsNaN(forecastInfo->tempHigh) ? QString() : QString::number(forecastInfo->tempHigh);
        const QString tempLow = qIsNaN(forecastInfo->tempLow) ? QString() : QString::number(forecastInfo->tempLow);

        data.insert(QStringLiteral("Short Forecast Day %1").arg(i),
                    QStringLiteral("%1|%2|%3|%4|%5|%6").arg(period, forecastInfo->iconName, forecastInfo->summary, tempHigh, tempLow, QString()));
        //.arg(forecastInfo->windSpeed)
        // arg(forecastInfo->windDirection));

        ++i;
    }

    data.insert(QStringLiteral("Credit"), i18nc("credit line, keep string short", "Data from BBC\302\240Weather"));
    data.insert(QStringLiteral("Credit Url"), place.forecastHTMLUrl);

    setData(weatherSource, data);
}

void UKMETIon::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
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

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(bbcukmet, UKMETIon, "ion-bbcukmet.json")

#include "ion_bbcukmet.moc"
