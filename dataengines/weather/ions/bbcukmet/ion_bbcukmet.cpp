/*
    SPDX-FileCopyrightText: 2007-2009 Shawn Starr <shawn.starr@rogers.com>
    SPDX-FileCopyrightText: 2024 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Ion for BBC's Weather from the UK Met Office */

#include "ion_bbcukmet.h"

#include "ion_bbcukmetdebug.h"

#include <KIO/TransferJob>
#include <KLocalizedString>
#include <KUnitConversion/Converter>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimeZone>
#include <QXmlStreamReader>

using namespace Qt::StringLiterals;

UKMETIon::UKMETIon(QObject *parent)
    : IonInterface(parent)

{
    setInitialized(true);
}

void UKMETIon::reset()
{
    m_sourcesToReset = sources();
    updateAllSources();
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
        if (sourceAction.count() < 3) {
            return false;
        }
        if (sourceAction[2].isEmpty()) {
            setData(source, QStringLiteral("validate"), QStringLiteral("bbcukmet|malformed"));
            return true;
        }

        qCDebug(IONENGINE_BBCUKMET) << "Update request for:" << sourceAction[2] << "stationId:" << sourceAction[3];

        const QString sourceKey = sourceAction[2];

        XMLMapInfo &place = m_place[sourceKey];
        place.place = sourceAction[2];
        place.stationId = sourceAction[3];

        getObservation(sourceKey);

        return true;
    }

    setData(source, QStringLiteral("validate"), QStringLiteral("bbcukmet|malformed"));
    return true;
}

KJob *UKMETIon::requestAPIJob(const QString &source, const QUrl &url)
{
    KIO::TransferJob *getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(u"cookies"_s, u"none"_s);

    m_jobData.insert(getJob, std::make_shared<QByteArray>());
    m_jobList.insert(getJob, source);

    qCDebug(IONENGINE_BBCUKMET) << "Requesting URL:" << url;

    connect(getJob, &KIO::TransferJob::data, this, [this](KIO::Job *job, const QByteArray &data) {
        if (data.isEmpty() || !m_jobData.contains(job)) {
            return;
        }
        m_jobData[job]->append(data);
    });

    return getJob;
}

void UKMETIon::getObservation(const QString &source)
{
    m_weatherData[source].isObservationDataPending = true;

    const QUrl url(u"https://weather-broker-cdn.api.bbci.co.uk/en/observation/rss/%1"_s.arg(m_place[source].stationId));
    const auto getJob = requestAPIJob(source, url);
    connect(getJob, &KJob::result, this, &UKMETIon::observation_slotJobFinished);
}

void UKMETIon::findPlace(const QString &place, const QString &source)
{
    // the API needs auto=true for partial-text searching
    // but unlike the normal query, using auto=true doesn't show locations which match the text but with different unicode
    // for example "hyderabad" with no auto matches "Hyderabad" and "Hyderābād"
    // but with auto matches only "Hyderabad"
    // so we merge the two results
    m_pendingSearchCount = 2;

    const QUrl url(u"https://open.live.bbc.co.uk/locator/locations?s=%1&format=json"_s.arg(place));
    const auto getJob = requestAPIJob(source, url);
    connect(getJob, &KJob::result, this, &UKMETIon::search_slotJobFinished);

    const QUrl autoUrl(u"https://open.live.bbc.co.uk/locator/locations?s=%1&format=json&auto=true"_s.arg(place));
    const auto getAutoJob = requestAPIJob(source, autoUrl);
    connect(getAutoJob, &KJob::result, this, &UKMETIon::search_slotJobFinished);
}

void UKMETIon::getForecast(const QString &source)
{
    m_weatherData[source].isForecastsDataPending = true;

    XMLMapInfo &place = m_place[source];
    const QUrl url(u"https://weather-broker-cdn.api.bbci.co.uk/en/forecast/rss/3day/%1"_s.arg(place.stationId));

    const auto getJob = requestAPIJob(source, url);
    connect(getJob, &KJob::result, this, &UKMETIon::forecast_slotJobFinished);
}

void UKMETIon::readSearchData(const QString &/*source*/, const QByteArray &json)
{
    QJsonObject jsonDocumentObject = QJsonDocument::fromJson(json).object().value(QStringLiteral("response")).toObject();

    if (jsonDocumentObject.isEmpty()) {
        return;
    }
    QJsonValue resultsVariant = jsonDocumentObject.value(QStringLiteral("locations"));

    if (resultsVariant.isUndefined()) {
        // this is a response from an auto=true query
        resultsVariant = jsonDocumentObject.value(QStringLiteral("results")).toObject().value(QStringLiteral("results"));
    }

    const QJsonArray results = resultsVariant.toArray();

    for (const QJsonValue &resultValue : results) {
        QJsonObject result = resultValue.toObject();
        const QString id = result.value(QStringLiteral("id")).toString();
        const QString name = result.value(QStringLiteral("name")).toString();
        const QString area = result.value(QStringLiteral("container")).toString();
        const QString country = result.value(QStringLiteral("country")).toString();

        if (id.isEmpty() || name.isEmpty() || area.isEmpty() || country.isEmpty()) {
            continue;
        }

        const QString fullName = u"%1, %2, %3"_s.arg(name, area, country);

        // Duplicate places can exist, show them too, but not if they have
        // the exact same id, which can happen since we're merging two results
        QString sourceKey = fullName;
        int duplicate = 1;
        while (m_locations.contains(sourceKey) && m_place.value(sourceKey).stationId != id) {
            duplicate++;
            sourceKey = u"%1 (#%2)"_s.arg(fullName).arg(duplicate);
        }
        if (m_locations.contains(sourceKey)) {
            continue;
        }

        XMLMapInfo &place = m_place[sourceKey];
        place.stationId = id;
        place.place = fullName;

        m_locations.append(sourceKey);
    }

    qCDebug(IONENGINE_BBCUKMET) << "Search results:" << results.count() << "Total unique locations:" << m_locations.count()
                                << "Pending calls:" << m_pendingSearchCount;
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

void UKMETIon::search_slotJobFinished(KJob *job)
{
    static std::mutex mtx;
    std::lock_guard<std::mutex> guard(mtx);

    m_pendingSearchCount--;

    const QString source = m_jobList.take(job);
    const auto data = m_jobData.take(job);

    // If Redirected, don't go to this routine
    if (!job->error() && !m_locations.contains(source)) {
        readSearchData(source, *data);
    }

    // Wait until the last search completes before serving the results
    if (m_pendingSearchCount == 0) {
        if (job->error() == KIO::ERR_SERVER_TIMEOUT && m_locations.count() == 0) {
            setData(source, QStringLiteral("validate"), QStringLiteral("bbcukmet|timeout"));
            disconnectSource(source, this);
            return;
        }

        validate(source);
    }
}

void UKMETIon::observation_slotJobFinished(KJob *job)
{
    const QString source = m_jobList.take(job);
    const auto data = m_jobData.take(job);

    if (!data->isEmpty()) {
        auto reader = QXmlStreamReader(*data);
        readObservationData(m_jobList[job], reader);
        getSolarData(source);
    }

    m_weatherData[source].isObservationDataPending = false;
    getForecast(source);

    if (m_sourcesToReset.contains(source)) {
        m_sourcesToReset.removeAll(source);
        Q_EMIT forceUpdate(this, source);
    }
}

void UKMETIon::forecast_slotJobFinished(KJob *job)
{
    const QString source = m_jobList.take(job);
    const auto data = m_jobData.take(job);

    if (!data->isEmpty()) {
        auto reader = QXmlStreamReader(*data);
        readForecast(source, reader);
    }

    m_weatherData[source].isForecastsDataPending = false;
    updateWeather(source);
}

void UKMETIon::parsePlaceObservation(const QString &source, WeatherData &data, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("rss"));

    while (!xml.atEnd()) {
        xml.readNext();

        const auto elementName = xml.name();

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

        const auto elementName = xml.name();

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

        const auto elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("channel")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("item")) {
                parseForecast(source, xml);
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

    WeatherData::Observation &current = data.current;

    while (!xml.atEnd()) {
        xml.readNext();

        const auto elementName = xml.name();

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
                    current.obsTime = conditionString.left(splitIndex);

                    if (current.obsTime.contains(QLatin1Char('-'))) {
                        // Saturday - 13:00 CET
                        // Saturday - 12:00 GMT
                        // timezone parsing is not yet supported by QDateTime, also is there just a dayname
                        // so try manually
                        // guess date from day
                        const QString dayString = current.obsTime.section(QLatin1Char('-'), 0, 0).trimmed();
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
                            const QString timeString = current.obsTime.section(QLatin1Char('-'), 1, 1).trimmed();
                            const QTime time = QTime::fromString(timeString.section(QLatin1Char(' '), 0, 0), QStringLiteral("hh:mm"));
                            const QTimeZone timeZone = QTimeZone(timeString.section(QLatin1Char(' '), 1, 1).toUtf8());
                            // TODO: if non-IANA timezone id is not known, try to guess timezone from other data

                            if (time.isValid() && timeZone.isValid()) {
                                current.observationDateTime = QDateTime(date, time, timeZone);
                            }
                        }
                    }

                    if (conditionData.contains(QLatin1Char(','))) {
                        current.condition = conditionData.section(QLatin1Char(','), 0, 0).trimmed();

                        if (current.condition == QLatin1String("null") || current.condition == QLatin1String("Not Available")) {
                            current.condition.clear();
                        }
                    }
                }

            } else if (elementName == QLatin1String("description")) {
                QString observeString = xml.readElementText();
                const QStringList observeData = observeString.split(QLatin1Char(':'));

                // FIXME: We should make this use a QRegExp but I need some help here :) -spstarr

                QString temperature_C = observeData[1].section(QChar(176), 0, 0).trimmed();
                parseFloat(current.temperature_C, temperature_C);

                current.windDirection = observeData[2].section(QLatin1Char(','), 0, 0).trimmed();
                if (current.windDirection.contains(QLatin1String("null"))) {
                    current.windDirection.clear();
                }

                QString windSpeed_miles = observeData[3].section(QLatin1Char(','), 0, 0).section(QLatin1Char(' '), 1, 1).remove(QStringLiteral("mph"));
                parseFloat(current.windSpeed_miles, windSpeed_miles);

                QString humidity = observeData[4].section(QLatin1Char(','), 0, 0).section(QLatin1Char(' '), 1, 1);
                if (humidity.endsWith(QLatin1Char('%'))) {
                    humidity.chop(1);
                }
                parseFloat(current.humidity, humidity);

                QString pressure = observeData[5].section(QLatin1Char(','), 0, 0).section(QLatin1Char(' '), 1, 1).section(QStringLiteral("mb"), 0, 0);
                parseFloat(current.pressure, pressure);

                current.pressureTendency = observeData[5].section(QLatin1Char(','), 1, 1).toLower().trimmed();
                if (current.pressureTendency == QLatin1String("no change")) {
                    current.pressureTendency = QStringLiteral("steady");
                }

                current.visibilityStr = observeData[6].trimmed();
                if (current.visibilityStr == QLatin1String("--")) {
                    current.visibilityStr.clear();
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

bool UKMETIon::readObservationData(const QString &source, QXmlStreamReader &xml)
{
    WeatherData &data = m_weatherData[source];
    data.current = WeatherData::Observation();

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("rss")) {
                parsePlaceObservation(source, data, xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }

    qCDebug(IONENGINE_BBCUKMET) << "Received observation data:" << m_weatherData[source].current.obsTime << m_weatherData[source].current.condition;

    return !xml.hasError();
}

void UKMETIon::getSolarData(const QString &source)
{
    WeatherData &data = m_weatherData[source];

    Plasma5Support::DataEngine *timeEngine = dataEngine(QStringLiteral("time"));
    const bool canCalculateElevation = (data.current.observationDateTime.isValid() && (!qIsNaN(data.stationLatitude) && !qIsNaN(data.stationLongitude)));

    if (!timeEngine || !canCalculateElevation) {
        return;
    }

    const QString oldTimeEngineSource = data.solarDataTimeEngineSourceName;
    data.solarDataTimeEngineSourceName = QStringLiteral("%1|Solar|Latitude=%2|Longitude=%3|DateTime=%4")
                                             .arg(QString::fromUtf8(data.current.observationDateTime.timeZone().id()))
                                             .arg(data.stationLatitude)
                                             .arg(data.stationLongitude)
                                             .arg(data.current.observationDateTime.toString(Qt::ISODate));

    // Check if we already have the data
    if (data.solarDataTimeEngineSourceName == oldTimeEngineSource) {
        return;
    }

    // Drop old elevation source
    if (!oldTimeEngineSource.isEmpty()) {
        timeEngine->disconnectSource(oldTimeEngineSource, this);
    }

    data.isSolarDataPending = true;
    timeEngine->connectSource(data.solarDataTimeEngineSourceName, this);
}

bool UKMETIon::readForecast(const QString &source, QXmlStreamReader &xml)
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

    return !xml.error();
}

void UKMETIon::parseForecast(const QString &source, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("item"));

    WeatherData &weatherData = m_weatherData[source];
    QList<WeatherData::ForecastInfo> &forecasts = weatherData.forecasts;

    // Flush out the old forecasts when updating.
    forecasts.clear();

    QString line;
    QString period;
    QString summary;
    const QRegularExpression high(QStringLiteral("Maximum Temperature: (-?\\d+).C"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression low(QStringLiteral("Minimum Temperature: (-?\\d+).C"), QRegularExpression::CaseInsensitiveOption);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.name() == QLatin1String("title")) {
            line = xml.readElementText().trimmed();

            WeatherData::ForecastInfo forecast;
            // FIXME: We should make this all use QRegExps in UKMETIon::parseFiveDayForecast() for forecast -spstarr

            const QString p = line.section(QLatin1Char(','), 0, 0);
            period = p.section(QLatin1Char(':'), 0, 0);
            summary = p.section(QLatin1Char(':'), 1, 1).trimmed();

            const QString temps = line.section(QLatin1Char(','), 1, 1);
            // Sometimes only one of min or max are reported
            QRegularExpressionMatch rmatch;
            if (temps.contains(high, &rmatch)) {
                parseFloat(forecast.tempHigh, rmatch.captured(1));
            }
            if (temps.contains(low, &rmatch)) {
                parseFloat(forecast.tempLow, rmatch.captured(1));
            }

            const QString summaryLC = summary.toLower();
            forecast.period = period;
            if (forecast.period == QLatin1String("Tonight")) {
                forecast.iconName = getWeatherIcon(nightIcons(), summaryLC);
            } else {
                forecast.iconName = getWeatherIcon(dayIcons(), summaryLC);
            }
            // db uses original strings normalized to lowercase, but we prefer the unnormalized if without translation
            const QString summaryTranslated = i18nc("weather forecast", summaryLC.toUtf8().data());
            forecast.summary = (summaryTranslated != summaryLC) ? summaryTranslated : summary;
            forecasts.append(forecast);
        }
    }
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
        if (m_place[invalidPlace].place.isEmpty()) {
            setData(source, QStringLiteral("validate"), QVariant(QString(u"bbcukmet|invalid|multiple|" + invalidPlace)));
        }
        return;
    }

    QString placeList;
    for (const QString &place : std::as_const(m_locations)) {
        placeList.append(u"|place|%1|extra|%2"_s.arg( //
            place.section(QLatin1Char('|'), 0, 1),
            m_place[place].stationId));
    }
    setData(source,
            u"validate"_s,
            QVariant(u"bbcukmet|valid|%1|%2"_s.arg( //
                m_locations.count() == 1 ? u"single"_s : u"multiple"_s,
                placeList)));

    m_locations.clear();
}

void UKMETIon::updateWeather(const QString &source)
{
    const WeatherData &weatherData = m_weatherData[source];

    if (weatherData.isForecastsDataPending || weatherData.isObservationDataPending || weatherData.isSolarDataPending) {
        return;
    }

    Plasma5Support::DataEngine::Data data;

    const XMLMapInfo &place = m_place[source];
    // work-around for buggy observation RSS feed missing the station name
    QString stationName = weatherData.stationName;
    if (stationName.isEmpty() || stationName == QLatin1Char(',')) {
        stationName = place.place;
    }

    data.insert(QStringLiteral("Place"), stationName);
    data.insert(QStringLiteral("Station"), stationName);

    const WeatherData::Observation &current = weatherData.current;
    if (current.observationDateTime.isValid()) {
        data.insert(QStringLiteral("Observation Timestamp"), current.observationDateTime);
    }
    if (!current.obsTime.isEmpty()) {
        data.insert(QStringLiteral("Observation Period"), current.obsTime);
    }

    if (!current.condition.isEmpty()) {
        data.insert(QStringLiteral("Current Conditions"), i18nc("weather condition", current.condition.toUtf8().data()));
    }

    const bool stationCoordsValid = (!qIsNaN(weatherData.stationLatitude) && !qIsNaN(weatherData.stationLongitude));

    if (stationCoordsValid) {
        data.insert(QStringLiteral("Latitude"), weatherData.stationLatitude);
        data.insert(QStringLiteral("Longitude"), weatherData.stationLongitude);
    }

    data.insert(QStringLiteral("Condition Icon"), getWeatherIcon(weatherData.isNight ? nightIcons() : dayIcons(), current.condition));

    if (!qIsNaN(current.humidity)) {
        data.insert(QStringLiteral("Humidity"), current.humidity);
        data.insert(QStringLiteral("Humidity Unit"), KUnitConversion::Percent);
    }

    if (!current.visibilityStr.isEmpty()) {
        data.insert(QStringLiteral("Visibility"), i18nc("visibility", current.visibilityStr.toUtf8().data()));
        data.insert(QStringLiteral("Visibility Unit"), KUnitConversion::NoUnit);
    }

    if (!qIsNaN(current.temperature_C)) {
        data.insert(QStringLiteral("Temperature"), current.temperature_C);
    }

    // Used for all temperatures
    data.insert(QStringLiteral("Temperature Unit"), KUnitConversion::Celsius);

    if (!qIsNaN(current.pressure)) {
        data.insert(QStringLiteral("Pressure"), current.pressure);
        data.insert(QStringLiteral("Pressure Unit"), KUnitConversion::Millibar);
        if (!current.pressureTendency.isEmpty()) {
            data.insert(QStringLiteral("Pressure Tendency"), current.pressureTendency);
        }
    }

    if (!qIsNaN(current.windSpeed_miles)) {
        data.insert(QStringLiteral("Wind Speed"), current.windSpeed_miles);
        data.insert(QStringLiteral("Wind Speed Unit"), KUnitConversion::MilePerHour);
        if (!current.windDirection.isEmpty()) {
            data.insert(QStringLiteral("Wind Direction"), getWindDirectionIcon(windIcons(), current.windDirection.toLower()));
        }
    }

    // Set number of forecasts per day/night supported
    data.insert(QStringLiteral("Total Weather Days"), weatherData.forecasts.size());

    int i = 0;
    for (const auto &forecastInfo : weatherData.forecasts) {
        QString period = forecastInfo.period;
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

        const QString tempHigh = qIsNaN(forecastInfo.tempHigh) ? QString() : QString::number(forecastInfo.tempHigh);
        const QString tempLow = qIsNaN(forecastInfo.tempLow) ? QString() : QString::number(forecastInfo.tempLow);

        data.insert(QStringLiteral("Short Forecast Day %1").arg(i),
                    QStringLiteral("%1|%2|%3|%4|%5|%6").arg(period, forecastInfo.iconName, forecastInfo.summary, tempHigh, tempLow, QString()));
        //.arg(forecastInfo->windSpeed)
        // arg(forecastInfo->windDirection));

        ++i;
    }

    data.insert(QStringLiteral("Credit"), i18nc("credit line, keep string short", "Data from BBC\302\240Weather"));
    data.insert(QStringLiteral("Credit Url"), place.forecastHTMLUrl);

    QString weatherSource = u"bbcukmet|weather|%1|%2"_s.arg(source, place.stationId);

    Q_EMIT cleanUpData(weatherSource);
    setData(weatherSource, data);

    qCDebug(IONENGINE_BBCUKMET) << "Updated weather data for" << weatherSource;
}

void UKMETIon::dataUpdated(const QString &sourceName, const Plasma5Support::DataEngine::Data &data)
{
    const bool isNight = (data.value(QStringLiteral("Corrected Elevation")).toDouble() < 0.0);

    for (auto [weatherSource, weatherData] : m_weatherData.asKeyValueRange()) {
        if (weatherData.solarDataTimeEngineSourceName == sourceName) {
            weatherData.isNight = isNight;
            weatherData.isSolarDataPending = false;
            updateWeather(weatherSource);
        }
    }
}

K_PLUGIN_CLASS_WITH_JSON(UKMETIon, "ion-bbcukmet.json")

#include "ion_bbcukmet.moc"
