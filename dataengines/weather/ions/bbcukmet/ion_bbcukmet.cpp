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
#include <QTimeZone>
#include <QTimer>

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
        place.forecastHTMLUrl = u"https://www.bbc.com/weather/%1"_s.arg(sourceAction[3]);

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

int UKMETIon::secondsToRetry()
{
    constexpr int MAX_RETRY_ATTEMPS = 5;

    m_retryAttemps++;

    if (m_retryAttemps > MAX_RETRY_ATTEMPS) {
        qCWarning(IONENGINE_BBCUKMET) << "Coudn't get a valid response after" << MAX_RETRY_ATTEMPS << "attemps";
        return -1;
    }

    const int delay_sec = 2 << m_retryAttemps; // exponential increase, starting at 4 seconds
    qCDebug(IONENGINE_BBCUKMET) << "Retry in" << delay_sec << "seconds";

    return delay_sec;
}

void UKMETIon::getObservation(const QString &source)
{
    m_weatherData[source].isObservationDataPending = true;

    const QUrl url(u"https://weather-broker-cdn.api.bbci.co.uk/en/observation/%1"_s.arg(m_place[source].stationId));
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
    const QUrl url(u"https://weather-broker-cdn.api.bbci.co.uk/en/forecast/aggregated/%1"_s.arg(place.stationId));

    const auto getJob = requestAPIJob(source, url);
    connect(getJob, &KJob::result, this, &UKMETIon::forecast_slotJobFinished);
}

void UKMETIon::readSearchData(const QString & /*source*/, const QByteArray &json)
{
    QJsonObject jsonDocumentObject = QJsonDocument::fromJson(json).object().value(u"response").toObject();

    if (jsonDocumentObject.isEmpty()) {
        return;
    }
    QJsonValue resultsVariant = jsonDocumentObject.value(u"locations");

    if (resultsVariant.isUndefined()) {
        // this is a response from an auto=true query
        resultsVariant = jsonDocumentObject.value(QStringLiteral("results")).toObject().value(QStringLiteral("results"));
    }

    const QJsonArray results = resultsVariant.toArray();

    for (const QJsonValue &resultValue : results) {
        QJsonObject result = resultValue.toObject();
        const QString id = result.value(u"id").toString();
        const QString name = result.value(u"name").toString();
        const QString area = result.value(u"container").toString();
        const QString country = result.value(u"country").toString();

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
        if (job->error() == KIO::ERR_SERVER_TIMEOUT && m_locations.isEmpty()) {
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

    QJsonParseError jsonError;
    const auto doc = QJsonDocument::fromJson(*data, &jsonError);

    if (doc.isNull()) {
        qCWarning(IONENGINE_BBCUKMET) << "Received invalid data:" << jsonError.errorString();
    } else if (const auto response = doc[u"response"_s].toObject(); !response.isEmpty()) {
        // Server returns some HTTP states as JSON data.
        const int errorCode = response[u"code"_s].toInt();
        qCWarning(IONENGINE_BBCUKMET) << "Received server error:" << errorCode << response[u"message"_s].toString();
        if (errorCode == 202) {
            // State "202 Accepted" means it's getting the data ready. Retry
            if (const int delay_sec = secondsToRetry(); delay_sec > 0) {
                QTimer::singleShot(delay_sec * 1000, [this, source]() {
                    getObservation(source);
                });
                return;
            }
        }
    } else {
        readObservationData(source, doc);
        getSolarData(source);
    }

    m_retryAttemps = 0;
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

    QJsonParseError jsonError;
    const auto doc = QJsonDocument::fromJson(*data, &jsonError);

    if (doc.isNull()) {
        qCWarning(IONENGINE_BBCUKMET) << "Received invalid data:" << jsonError.errorString();
    } else if (const auto response = doc[u"response"_s].toObject(); !response.isEmpty()) {
        // Server returns some HTTP states as JSON data.
        const int errorCode = response[u"code"_s].toInt();
        qCWarning(IONENGINE_BBCUKMET) << "Received server error:" << errorCode << response[u"message"_s].toString();
        if (errorCode == 202) {
            // State "202 Accepted" means it's getting the data ready. Retry
            if (const int delay_sec = secondsToRetry(); delay_sec > 0) {
                QTimer::singleShot(delay_sec * 1000, [this, source]() {
                    getForecast(source);
                });
                return;
            }
        }
    } else {
        readForecast(source, doc);
    }

    m_retryAttemps = 0;
    m_weatherData[source].isForecastsDataPending = false;
    updateWeather(source);
}

bool UKMETIon::readObservationData(const QString &source, const QJsonDocument &doc)
{
    WeatherData &data = m_weatherData[source];
    WeatherData::Observation &current = data.current;

    // Station data
    const QJsonObject station = doc[u"station"].toObject();
    if (!station.isEmpty()) {
        data.stationName = station[u"name"].toString();
        data.stationLatitude = station[u"latitude"].toDouble(qQNaN());
        data.stationLongitude = station[u"longitude"].toDouble(qQNaN());
    }

    // Observation data
    const QJsonArray observations = doc[u"observations"].toArray();
    if (observations.isEmpty()) {
        qCDebug(IONENGINE_BBCUKMET) << "Malformed observation report" << doc;
        return false;
    }
    const QJsonObject observation = observations.first().toObject();

    current = WeatherData::Observation(); // Clean-up

    current.observationDateTime = QDateTime::fromString(observation[u"updateTimestamp"].toString(), Qt::ISODate);
    current.obsTime = observation[u"localDate"].toString() + u" " + observation[u"localTime"].toString();

    current.condition = observation[u"weatherTypeText"].toString();
    if (current.condition == "null"_L1 || current.condition == "Not Available"_L1) {
        current.condition.clear();
    }

    current.temperature_C = observation[u"temperature"][u"C"].toDouble(qQNaN());
    current.humidity = observation[u"humidityPercent"].toDouble(qQNaN());
    current.pressure = observation[u"pressureMb"].toDouble(qQNaN());

    // <ion.h> "Pressure Tendency": string, "rising", "falling", "steady"
    current.pressureTendency = observation[u"pressureDirection"].toString().toLower();
    if (current.pressureTendency == "no change"_L1) {
        current.pressureTendency = u"steady"_s;
    }

    current.windSpeed_miles = observation[u"wind"][u"windSpeedMph"].toDouble(qQNaN());
    current.windDirection = (current.windSpeed_miles > 0) ? observation[u"wind"][u"windDirectionAbbreviation"].toString() : u"VR"_s;

    current.visibilityStr = observation[u"visibility"].toString();

    qCDebug(IONENGINE_BBCUKMET) << "Read observation data:" << m_weatherData[source].current.obsTime << m_weatherData[source].current.condition;

    return true;
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

bool UKMETIon::readForecast(const QString &source, const QJsonDocument &doc)
{
    const QJsonArray info = doc[u"forecasts"_s].toArray();
    if (info.isEmpty()) {
        qCDebug(IONENGINE_BBCUKMET) << "Malformed forecast report" << doc;
    }

    WeatherData &weatherData = m_weatherData[source];
    weatherData.forecasts.clear();

    // Indicates if the forecast includes a first report for tonight
    bool isTonight = doc[u"isNight"_s].toBool(false);

    for (const QJsonValue &forecast : info) {
        const QJsonObject report = forecast[u"summary"_s][u"report"].toObject();
        if (report.isEmpty()) {
            continue;
        }

        weatherData.forecasts << parseForecastReport(report, isTonight);
        isTonight = false; // Just use it on the first report
    }

    qCDebug(IONENGINE_BBCUKMET) << "Read forecast data:" << m_weatherData[source].forecasts.count() << "days";

    return true;
}

WeatherData::ForecastInfo UKMETIon::parseForecastReport(const QJsonObject &report, bool isNight) const
{
    WeatherData::ForecastInfo forecast;

    forecast.period = QDate::fromString(report[u"localDate"].toString(), Qt::ISODate); // "YYYY-MM-DD" (ISO8601)
    forecast.isNight = isNight;

    forecast.summary = report[u"weatherTypeText"].toString().toLower();
    forecast.iconName = getWeatherIcon(isNight ? nightIcons() : dayIcons(), forecast.summary);

    forecast.tempLow = report[u"minTempC"].toDouble(qQNaN());
    if (!isNight) { // Don't include max temperatures in a nightly report
        forecast.tempHigh = report[u"maxTempC"].toDouble(qQNaN());
    }
    forecast.precipitationPct = report[u"precipitationProbabilityInPercent"].toInt();
    forecast.windSpeed = report[u"windSpeedKph"].toDouble(qQNaN());
    forecast.windDirection = report[u"windDirectionAbbreviation"].toString();

    return forecast;
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
    data.insert(QStringLiteral("Place"), place.place);
    data.insert(QStringLiteral("Station"), weatherData.stationName);

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
        data.insert(QStringLiteral("Wind Direction"), current.windDirection);
    }

    // Daily forecast info
    int day = 0;
    for (const WeatherData::ForecastInfo &forecast : std::as_const(weatherData.forecasts)) {
        QString weekDayLabel = QLocale().toString(forecast.period, u"ddd"_s);
        if (day == 0 && forecast.period <= QDate::currentDate()) {
            weekDayLabel = (forecast.isNight) ? i18nc("Short for Tonight", "Tonight") : i18nc("Short for Today", "Today");
        }

        data.insert(u"Short Forecast Day %1"_s.arg(day),
                    u"%1|%2|%3|%4|%5|%6"_s.arg(weekDayLabel)
                        .arg(forecast.iconName)
                        .arg(i18nc("weather forecast", forecast.summary.toUtf8().data()))
                        .arg(qIsNaN(forecast.tempHigh) ? QString() : QString::number(forecast.tempHigh))
                        .arg(qIsNaN(forecast.tempLow) ? QString() : QString::number(forecast.tempLow))
                        .arg(forecast.precipitationPct));

        ++day;

        // Limit the forecast days to 7 days (the applet is not ready for more)
        if (day >= 7) {
            break;
        }
    }

    data.insert(QStringLiteral("Total Weather Days"), day);

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
