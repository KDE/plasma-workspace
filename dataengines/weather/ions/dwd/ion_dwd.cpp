/*
    SPDX-FileCopyrightText: 2021 Emily Ehlert

    Based upon BBC Weather Ion and ENV Canada Ion by Shawn Starr
    SPDX-FileCopyrightText: 2007-2009 Shawn Starr <shawn.starr@rogers.com>

    also

    the wetter.com Ion by Thilo-Alexander Ginkel
    SPDX-FileCopyrightText: 2009 Thilo-Alexander Ginkel <thilo@ginkel.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Ion for weather data from Deutscher Wetterdienst (DWD) / German Weather Service */

#include "ion_dwd.h"

#include "ion_dwddebug.h"

#include <KIO/TransferJob>
#include <KLocalizedString>
#include <KUnitConversion/Converter>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QVariant>

using namespace Qt::StringLiterals;

constexpr QLatin1String CATALOGUE_URL = "https://www.dwd.de/DE/leistungen/met_verfahren_mosmix/mosmix_stationskatalog.cfg?view=nasPublication&nn=16102"_L1;
constexpr QLatin1String FORECAST_URL = "https://app-prod-ws.warnwetter.de/v30/stationOverviewExtended?stationIds=%1"_L1;
constexpr QLatin1String MEASURE_URL = "https://s3.eu-central-1.amazonaws.com/app-prod-static.warnwetter.de/v16/current_measurement_%1.json"_L1;

DWDIon::DWDIon(QObject *parent)
    : IonInterface(parent)
{
    setInitialized(true);
}

void DWDIon::reset()
{
    m_sourcesToReset = sources();
    updateAllSources();
}

QMap<QString, IonInterface::ConditionIcons> DWDIon::getUniversalIcons() const
{
    return QMap<QString, ConditionIcons>{
        {u"4"_s, Overcast},      {u"5"_s, Mist},          {u"6"_s, Mist},         {u"7"_s, LightRain}, {u"8"_s, Rain},          {u"9"_s, Rain},
        {u"10"_s, LightRain},    {u"11"_s, Rain},         {u"12"_s, Flurries},    {u"13"_s, RainSnow}, {u"14"_s, LightSnow},    {u"15"_s, Snow},
        {u"16"_s, Snow},         {u"17"_s, Hail},         {u"18"_s, LightRain},   {u"19"_s, Rain},     {u"20"_s, Flurries},     {u"21"_s, RainSnow},
        {u"22"_s, LightSnow},    {u"23"_s, Snow},         {u"24"_s, Hail},        {u"25"_s, Hail},     {u"26"_s, Thunderstorm}, {u"27"_s, Thunderstorm},
        {u"28"_s, Thunderstorm}, {u"29"_s, Thunderstorm}, {u"30"_s, Thunderstorm}};
}

QMap<QString, IonInterface::ConditionIcons> DWDIon::setupDayIconMappings() const
{
    QMap<QString, ConditionIcons> universalIcons = getUniversalIcons();
    QMap<QString, ConditionIcons> dayIcons = {{u"1"_s, ClearDay}, {u"2"_s, FewCloudsDay}, {u"3"_s, PartlyCloudyDay}, {u"31"_s, ClearWindyDay}};
    dayIcons.insert(universalIcons);
    return dayIcons;
}

QMap<QString, IonInterface::ConditionIcons> DWDIon::setupNightIconMappings() const
{
    QMap<QString, ConditionIcons> universalIcons = getUniversalIcons();
    QMap<QString, ConditionIcons> nightIcons = {{u"1"_s, ClearNight}, {u"2"_s, FewCloudsNight}, {u"3"_s, PartlyCloudyNight}, {u"31"_s, ClearWindyNight}};
    nightIcons.insert(universalIcons);
    return nightIcons;
}

QMap<QString, IonInterface::WindDirections> DWDIon::setupWindIconMappings() const
{
    return QMap<QString, WindDirections>{
        {u"0"_s, N},   {u"10"_s, N},    {u"20"_s, NNE},  {u"30"_s, NNE},  {u"40"_s, NE},  {u"50"_s, NE},   {u"60"_s, ENE},  {u"70"_s, ENE},  {u"80"_s, E},
        {u"90"_s, E},  {u"100"_s, E},   {u"120"_s, ESE}, {u"130"_s, ESE}, {u"140"_s, SE}, {u"150"_s, SE},  {u"160"_s, SSE}, {u"170"_s, SSE}, {u"180"_s, S},
        {u"190"_s, S}, {u"200"_s, SSW}, {u"210"_s, SSW}, {u"220"_s, SW},  {u"230"_s, SW}, {u"240"_s, WSW}, {u"250"_s, WSW}, {u"260"_s, W},   {u"270"_s, W},
        {u"280"_s, W}, {u"290"_s, WNW}, {u"300"_s, WNW}, {u"310"_s, NW},  {u"320"_s, NW}, {u"330"_s, NNW}, {u"340"_s, NNW}, {u"350"_s, N},   {u"360"_s, N},
    };
}

QMap<QString, IonInterface::ConditionIcons> const &DWDIon::dayIcons() const
{
    static QMap<QString, ConditionIcons> const dval = setupDayIconMappings();
    return dval;
}

QMap<QString, IonInterface::ConditionIcons> const &DWDIon::nightIcons() const
{
    static QMap<QString, ConditionIcons> const dval = setupNightIconMappings();
    return dval;
}

QMap<QString, IonInterface::WindDirections> const &DWDIon::windIcons() const
{
    static QMap<QString, WindDirections> const wval = setupWindIconMappings();
    return wval;
}

bool DWDIon::updateIonSource(const QString &source)
{
    // We expect the applet to send the source in the following tokenization:
    // ionname|validate|place_name - Triggers validation (search) of place
    // ionname|weather|place_name|extra - Triggers receiving weather of place
    const QList<QStringView> sourceAction = QStringView(source).split('|'_L1);

    if (sourceAction.size() < 3 || sourceAction[2].isEmpty()) {
        setData(source, u"validate"_s, u"dwd|malformed"_s);
        return true;
    }

    const QString placeName = sourceAction[2].toString();

    if (sourceAction[1] == "validate"_L1) {
        // Look for places to match
        findPlace(placeName);
        return true;
    }

    if (sourceAction[1] == "weather"_L1) {
        if (sourceAction.count() < 4) {
            setData(source, u"validate"_s, u"dwd|malformed"_s);
            return false;
        }

        const QString stationId = sourceAction[3].toString();
        m_place[placeName] = stationId;

        qCDebug(IONENGINE_dwd) << "About to retrieve forecast for source: " << placeName << stationId;
        fetchWeather(placeName, stationId);

        return true;
    }

    setData(source, u"validate"_s, u"dwd|malformed"_s);
    return true;
}

void DWDIon::findPlace(const QString &searchText)
{
    // Checks if the stations have already been loaded, always contains the currently active one
    if (m_place.size() > 1) {
        setData(QString(u"dwd|validate|" + searchText), Data());
        searchInStationList(searchText);
    } else {
        const QUrl forecastURL(CATALOGUE_URL);
        KIO::TransferJob *getJob = KIO::get(forecastURL, KIO::Reload, KIO::HideProgressInfo);
        getJob->addMetaData(u"cookies"_s, u"none"_s);

        m_searchJobList.insert(getJob, searchText);
        m_searchJobData.insert(getJob, QByteArray(""));

        connect(getJob, &KIO::TransferJob::data, this, &DWDIon::setup_slotDataArrived);
        connect(getJob, &KJob::result, this, &DWDIon::setup_slotJobFinished);
    }
}

void DWDIon::fetchWeather(const QString &placeName, const QString &placeID)
{
    for (const QString &fetching : std::as_const(m_forecastJobList)) {
        if (fetching == placeName) {
            // already fetching!
            return;
        }
    }

    // Fetch forecast data
    const QUrl forecastURL(FORECAST_URL.arg(placeID));
    KIO::TransferJob *getJob = KIO::get(forecastURL, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(u"cookies"_s, u"none"_s);

    m_forecastJobList.insert(getJob, placeName);
    m_forecastJobJSON.insert(getJob, QByteArray(""));

    qCDebug(IONENGINE_dwd) << "Requesting URL: " << forecastURL;

    connect(getJob, &KIO::TransferJob::data, this, &DWDIon::forecast_slotDataArrived);
    connect(getJob, &KJob::result, this, &DWDIon::forecast_slotJobFinished);
    m_weatherData[placeName].isForecastsDataPending = true;

    // Fetch current measurements (different url AND different API, AMAZING)
    const QUrl measureURL(MEASURE_URL.arg(placeID));
    KIO::TransferJob *getMeasureJob = KIO::get(measureURL, KIO::Reload, KIO::HideProgressInfo);
    getMeasureJob->addMetaData(u"cookies"_s, u"none"_s);

    m_measureJobList.insert(getMeasureJob, placeName);
    m_measureJobJSON.insert(getMeasureJob, QByteArray(""));

    qCDebug(IONENGINE_dwd) << "Requesting URL: " << measureURL;

    connect(getMeasureJob, &KIO::TransferJob::data, this, &DWDIon::measure_slotDataArrived);
    connect(getMeasureJob, &KJob::result, this, &DWDIon::measure_slotJobFinished);
    m_weatherData[placeName].isMeasureDataPending = true;
}

void DWDIon::setup_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    QByteArray local = data;

    if (data.isEmpty() || !m_searchJobData.contains(job)) {
        return;
    }

    m_searchJobData[job].append(local);
}

void DWDIon::measure_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    QByteArray local = data;

    if (data.isEmpty() || !m_measureJobJSON.contains(job)) {
        return;
    }

    m_measureJobJSON[job].append(local);
}

void DWDIon::forecast_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    QByteArray local = data;

    if (data.isEmpty() || !m_forecastJobJSON.contains(job)) {
        return;
    }

    m_forecastJobJSON[job].append(local);
}

void DWDIon::setup_slotJobFinished(KJob *job)
{
    if (!job->error()) {
        const QString searchText(m_searchJobList.value(job));
        setData(u"dwd|validate|" + searchText, Data());

        QByteArray catalogueData = m_searchJobData[job];
        if (!catalogueData.isEmpty()) {
            parseStationData(catalogueData);
            searchInStationList(searchText);
        }
    } else {
        qCWarning(IONENGINE_dwd) << "error during setup" << job->errorText();
    }

    m_searchJobList.remove(job);
    m_searchJobData.remove(job);
}

void DWDIon::measure_slotJobFinished(KJob *job)
{
    const QString source(m_measureJobList.value(job));
    const QByteArray &jsonData = m_measureJobJSON.value(job);

    if (!job->error() && !jsonData.isEmpty()) {
        setData(source, Data());
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        parseMeasureData(source, doc);
    } else {
        qCWarning(IONENGINE_dwd) << "no measurements received" << job->errorText();
        m_weatherData[source].isMeasureDataPending = false;
        updateWeather(source);
    }

    m_measureJobList.remove(job);
    m_measureJobJSON.remove(job);
}

void DWDIon::forecast_slotJobFinished(KJob *job)
{
    if (!job->error()) {
        const QString source(m_forecastJobList.value(job));
        setData(source, Data());

        QJsonDocument doc = QJsonDocument::fromJson(m_forecastJobJSON.value(job));

        if (!doc.isEmpty()) {
            parseForecastData(source, doc);
        }

        if (m_sourcesToReset.contains(source)) {
            m_sourcesToReset.removeAll(source);
            const QString weatherSource = u"dwd|weather|%1|%2"_s.arg(source, m_place[source]);

            // so the weather engine updates it's data
            forceImmediateUpdateOfAllVisualizations();

            // update the clients of our engine
            Q_EMIT forceUpdate(this, weatherSource);
        }
    } else {
        qCWarning(IONENGINE_dwd) << "error during forecast" << job->errorText();
    }

    m_forecastJobList.remove(job);
    m_forecastJobJSON.remove(job);
}

void DWDIon::calculatePositions(const QStringList &lines, QList<int> &namePositionalInfo, QList<int> &stationIdPositionalInfo) const
{
    QStringList stringLengths = lines[1].split(QChar::Space);
    QList<int> lengths;
    for (const QString &length : std::as_const(stringLengths)) {
        lengths.append(length.count());
    }

    int curpos = 0;

    for (int labelLength : lengths) {
        QString label = lines[0].mid(curpos, labelLength).toLower();

        if (label.contains(u"name"_s)) {
            namePositionalInfo[0] = curpos;
            namePositionalInfo[1] = labelLength;
        } else if (label.contains(u"id"_s)) {
            stationIdPositionalInfo[0] = curpos;
            stationIdPositionalInfo[1] = labelLength;
        }

        curpos += labelLength + 1;
    }
}

void DWDIon::parseStationData(const QByteArray &data)
{
    QString stringData = QString::fromLatin1(data);
    QStringList lines = stringData.split(QChar::LineFeed);

    QList<int> namePositionalInfo(2);
    QList<int> stationIdPositionalInfo(2);
    calculatePositions(lines, namePositionalInfo, stationIdPositionalInfo);

    // This loop parses the station file (https://www.dwd.de/DE/leistungen/met_verfahren_mosmix/mosmix_stationskatalog.cfg)
    // ID    ICAO NAME                 LAT    LON     ELEV
    // ----- ---- -------------------- -----  ------- -----
    // 01001 ENJA JAN MAYEN             70.56   -8.40    10
    // 01008 ENSB SVALBARD              78.15   15.28    29
    int lineIndex = 0;
    for (const QString &line : std::as_const(lines)) {
        QString name = line.mid(namePositionalInfo[0], namePositionalInfo[1]).trimmed();
        QString id = line.mid(stationIdPositionalInfo[0], stationIdPositionalInfo[1]).trimmed();

        // This checks if this station is a station we know is working
        // With this we remove all non working but also a lot of working ones.
        if (id.startsWith(QLatin1Char('0')) || id.startsWith(QLatin1Char('1'))) {
            m_place.insert(camelCaseString(name), id);
        } else if (lineIndex > 10) {
            // After header is passed and some more lines for safety, abort parse if filter fails, all acceptable stations were found
            break;
        }

        lineIndex += 1;
    }
    qCDebug(IONENGINE_dwd) << "Number of parsed stations: " << m_place.size();
}

void DWDIon::searchInStationList(const QString &searchText)
{
    QString flatSearchText = searchText;
    flatSearchText // The station list does not contains umlauts
        .replace(u"ä"_s, u"ae"_s)
        .replace(u"ö"_s, u"oe"_s)
        .replace(u"ü"_s, u"ue"_s)
        .replace(u"ß"_s, u"ss"_s);

    qCDebug(IONENGINE_dwd) << "Searching in station list:" << flatSearchText;

    for (const auto [name, id] : m_place.asKeyValueRange()) {
        if (name.contains(flatSearchText, Qt::CaseInsensitive)) {
            m_locations.append(name);
        }
    }

    validate(searchText);
}

void DWDIon::parseForecastData(const QString &source, const QJsonDocument &doc)
{
    QVariantMap weatherMap = doc.object().toVariantMap();
    if (weatherMap.isEmpty()) {
        return;
    }
    weatherMap = weatherMap.first().toMap(); // Mind the .first(). It needs guarding against isEmpty.
    if (!weatherMap.isEmpty()) {
        // Forecast data
        QVariantList daysList = weatherMap[u"days"_s].toList();

        WeatherData &weatherData = m_weatherData[source];
        QList<WeatherData::ForecastInfo> &forecasts = weatherData.forecasts;

        // Flush out the old forecasts when updating.
        forecasts.clear();

        int dayNumber = 0;

        for (const QVariant &day : daysList) {
            QMap dayMap = day.toMap();
            QString period = dayMap[u"dayDate"_s].toString();
            QString cond = dayMap[u"icon"_s].toString();

            WeatherData::ForecastInfo forecast;
            forecast.period = QDateTime::fromString(period, u"yyyy-MM-dd"_s);
            forecast.tempHigh = parseNumber(dayMap[u"temperatureMax"_s]);
            forecast.tempLow = parseNumber(dayMap[u"temperatureMin"_s]);
            forecast.precipitation = dayMap[u"precipitation"_s].toInt();
            forecast.iconName = getWeatherIcon(dayIcons(), cond);

            if (dayNumber == 0) {
                // These alternative measurements are used, when the stations doesn't have it's own measurements, uses forecast data from the current day
                weatherData.windSpeedAlt = parseNumber(dayMap[u"windSpeed"_s]);
                weatherData.gustSpeedAlt = parseNumber(dayMap[u"windGust"_s]);
                QString windDirection = roundWindDirections(dayMap[u"windDirection"_s].toInt());
                weatherData.windDirectionAlt = getWindDirectionIcon(windIcons(), windDirection);

                // Also fetch today's sunrise and sunset times to determine whether to pick day or night icons
                weatherData.sunriseTime = parseDateFromMSecs(dayMap[u"sunrise"_s].toLongLong());
                weatherData.sunsetTime = parseDateFromMSecs(dayMap[u"sunset"_s].toLongLong());
            }

            forecasts.append(forecast);

            dayNumber++;
            // Only get the next 7 days (including today)
            if (dayNumber == 7) {
                break;
            }
        }

        // Warnings data
        QVariantList warningData = weatherMap[u"warnings"_s].toList();

        QList<WeatherData::WarningInfo> &warningList = weatherData.warnings;

        // Flush out the old forecasts when updating.
        warningList.clear();

        for (const QVariant &warningElement : warningData) {
            QMap warningMap = warningElement.toMap();

            WeatherData::WarningInfo warning;
            warning.headline = warningMap[u"headline"_s].toString();
            warning.description = warningMap[u"description"_s].toString();
            warning.priority = warningMap[u"level"_s].toInt();
            warning.type = warningMap[u"event"_s].toString();
            warning.timestamp = QDateTime::fromMSecsSinceEpoch(warningMap[u"start"_s].toLongLong());

            warningList.append(warning);
        }

        weatherData.isForecastsDataPending = false;

        updateWeather(source);
    }
}

void DWDIon::parseMeasureData(const QString &source, const QJsonDocument &doc)
{
    WeatherData &weatherData = m_weatherData[source];
    QVariantMap weatherMap = doc.object().toVariantMap();

    if (!weatherMap.isEmpty()) {
        weatherData.observationDateTime = parseDateFromMSecs(weatherMap[u"time"_s]);

        weatherData.condIconNumber = weatherMap[u"icon"_s].toString();

        bool windIconValid = false;
        const int windDirection = weatherMap[u"winddirection"_s].toInt(&windIconValid);
        if (windIconValid) {
            weatherData.windDirection = getWindDirectionIcon(windIcons(), roundWindDirections(windDirection));
        }

        weatherData.temperature = parseNumber(weatherMap[u"temperature"_s]);
        weatherData.humidity = parseNumber(weatherMap[u"humidity"_s]);
        weatherData.pressure = parseNumber(weatherMap[u"pressure"_s]);
        weatherData.windSpeed = parseNumber(weatherMap[u"meanwind"_s]);
        weatherData.gustSpeed = parseNumber(weatherMap[u"maxwind"_s]);
        weatherData.dewpoint = parseNumber(weatherMap[u"dewpoint"_s]);
    }

    weatherData.isMeasureDataPending = false;

    updateWeather(source);
}

void DWDIon::validate(const QString &searchText)
{
    const QString source(u"dwd|validate|" + searchText);

    if (m_locations.isEmpty()) {
        const QString invalidPlace = searchText;
        setData(source, u"validate"_s, QVariant(QString(u"dwd|invalid|multiple|" + invalidPlace)));
        return;
    }

    QString placeList;
    for (const QString &place : std::as_const(m_locations)) {
        placeList.append(u"|place|" + place + u"|extra|" + m_place[place]);
    }
    if (m_locations.count() > 1) {
        setData(source, u"validate"_s, QVariant(QString(u"dwd|valid|multiple" + placeList)));
    } else {
        placeList[7] = placeList[7].toUpper();
        setData(source, u"validate"_s, QVariant(QString(u"dwd|valid|single" + placeList)));
    }
    m_locations.clear();
}

void DWDIon::updateWeather(const QString &source)
{
    const WeatherData &weatherData = m_weatherData[source];

    if (weatherData.isForecastsDataPending || weatherData.isMeasureDataPending) {
        return;
    }

    QString placeCode = m_place[source];
    QString weatherSource = u"dwd|weather|%1|%2"_s.arg(source, placeCode);

    Plasma5Support::DataEngine::Data data;

    data.insert(u"Place"_s, source);
    data.insert(u"Station"_s, source);

    data.insert(u"Temperature Unit"_s, KUnitConversion::Celsius);
    data.insert(u"Wind Speed Unit"_s, KUnitConversion::KilometerPerHour);
    data.insert(u"Humidity Unit"_s, KUnitConversion::Percent);
    data.insert(u"Pressure Unit"_s, KUnitConversion::Hectopascal);

    if (!weatherData.observationDateTime.isNull()) {
        data.insert(u"Observation Timestamp"_s, weatherData.observationDateTime);
    } else {
        data.insert(u"Observation Timestamp"_s, QDateTime::currentDateTime());
    }

    if (!weatherData.condIconNumber.isEmpty()) {
        data.insert(u"Condition Icon"_s, getWeatherIcon(isNightTime(weatherData) ? nightIcons() : dayIcons(), weatherData.condIconNumber));
    }

    if (!qIsNaN(weatherData.temperature)) {
        data.insert(u"Temperature"_s, weatherData.temperature);
    }
    if (!qIsNaN(weatherData.humidity)) {
        data.insert(u"Humidity"_s, weatherData.humidity);
    }
    if (!qIsNaN(weatherData.pressure)) {
        data.insert(u"Pressure"_s, weatherData.pressure);
    }
    if (!qIsNaN(weatherData.dewpoint)) {
        data.insert(u"Dewpoint"_s, weatherData.dewpoint);
    }

    if (!qIsNaN(weatherData.windSpeed)) {
        data.insert(u"Wind Speed"_s, weatherData.windSpeed);
    } else {
        data.insert(u"Wind Speed"_s, weatherData.windSpeedAlt);
    }

    if (!qIsNaN(weatherData.gustSpeed)) {
        data.insert(u"Wind Gust Speed"_s, weatherData.gustSpeed);
    } else {
        data.insert(u"Wind Gust Speed"_s, weatherData.gustSpeedAlt);
    }

    if (!weatherData.windDirection.isEmpty()) {
        data.insert(u"Wind Direction"_s, weatherData.windDirection);
    } else {
        data.insert(u"Wind Direction"_s, weatherData.windDirectionAlt);
    }

    int dayNumber = 0;
    for (const auto &dayForecast : weatherData.forecasts) {
        QString period;
        if (dayNumber == 0) {
            period = i18nc("Short for Today", "Today");
        } else {
            period = dayForecast.period.toString(u"dddd"_s);

            period.replace(u"Saturday"_s, i18nc("Short for Saturday", "Sat"));
            period.replace(u"Sunday"_s, i18nc("Short for Sunday", "Sun"));
            period.replace(u"Monday"_s, i18nc("Short for Monday", "Mon"));
            period.replace(u"Tuesday"_s, i18nc("Short for Tuesday", "Tue"));
            period.replace(u"Wednesday"_s, i18nc("Short for Wednesday", "Wed"));
            period.replace(u"Thursday"_s, i18nc("Short for Thursday", "Thu"));
            period.replace(u"Friday"_s, i18nc("Short for Friday", "Fri"));
        }

        data.insert(u"Short Forecast Day %1"_s.arg(dayNumber),
                    u"%1|%2|%3|%4|%5|%6"_s.arg(period, dayForecast.iconName, ""_L1)
                        .arg(dayForecast.tempHigh)
                        .arg(dayForecast.tempLow)
                        .arg(""_L1)); // dayForecast.precipitation is a quantity, not a probability
        dayNumber++;
    }

    int k = 0;

    for (const auto &warning : weatherData.warnings) {
        const QString number = QString::number(k);

        data.insert(QString(u"Warning Priority " + number), warning.priority);
        data.insert(QString(u"Warning Description " + number), u"<p><b>%1</b></p>%2"_s.arg(warning.headline, warning.description));
        data.insert(QString(u"Warning Timestamp " + number), warning.timestamp.toString(u"dd.MM.yyyy"_s));

        ++k;
    }

    data.insert(u"Total Weather Days"_s, weatherData.forecasts.size());
    data.insert(u"Total Warnings Issued"_s, weatherData.warnings.size());
    data.insert(u"Credit"_s, i18nc("credit line, don't change name!", "Source: Deutscher Wetterdienst"));
    data.insert(u"Credit Url"_s, u"https://www.dwd.de/"_s);

    Q_EMIT cleanUpData(source);
    setData(weatherSource, data);
}

/*
 * Helper methods
 */
float DWDIon::parseNumber(const QVariant &number) const
{
    bool isValid = false;
    const int intValue = number.toInt(&isValid);
    if (!isValid) {
        return NAN;
    }
    if (intValue == 0x7fff) { // DWD uses 32767 to mark an error value
        return NAN;
    }
    // e.g. DWD API int 17 equals 1.7
    return static_cast<float>(intValue) / 10;
}

QDateTime DWDIon::parseDateFromMSecs(const QVariant &timestamp) const
{
    bool isValid = false;
    const qint64 msecs = timestamp.toLongLong(&isValid);

    return isValid ? QDateTime::fromMSecsSinceEpoch(msecs) : QDateTime();
}

QString DWDIon::roundWindDirections(int windDirection) const
{
    QString roundedWindDirection = QString::number(qRound(((float)windDirection) / 100) * 10);
    return roundedWindDirection;
}

QString DWDIon::extractString(const QByteArray &array, int start, int length) const
{
    QString string;

    for (int i = start; i < start + length; i++) {
        string.append(QLatin1Char(array[i]));
    }

    return string;
}

QString DWDIon::camelCaseString(const QString &text) const
{
    QString result;
    bool nextBig = true;

    for (QChar c : text) {
        if (c.isLetter()) {
            if (nextBig) {
                result.append(c.toUpper());
                nextBig = false;
            } else {
                result.append(c.toLower());
            }
        } else {
            if (c == QChar::Space || c == '-'_L1) {
                nextBig = true;
            }
            result.append(c);
        }
    }

    return result;
}

bool DWDIon::isNightTime(const WeatherData &weatherData) const
{
    if (weatherData.sunriseTime.isNull() || weatherData.sunsetTime.isNull()) {
        // default to daytime icons if we're missing sunrise/sunset times
        return false;
    }

    return weatherData.observationDateTime < weatherData.sunriseTime || weatherData.observationDateTime > weatherData.sunsetTime;
}

K_PLUGIN_CLASS_WITH_JSON(DWDIon, "ion-dwd.json")

#include "ion_dwd.moc"
