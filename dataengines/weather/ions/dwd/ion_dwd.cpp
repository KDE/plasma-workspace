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

#include <KIO/Job>
#include <KLocalizedString>
#include <KUnitConversion/Converter>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QVariant>

/*
 * Initialization
 */

WeatherData::WeatherData()
    : temperature(qQNaN())
    , humidity(qQNaN())
    , pressure(qQNaN())
    , windSpeed(qQNaN())
    , gustSpeed(qQNaN())
    , dewpoint(qQNaN())
    , windSpeedAlt(qQNaN())
    , gustSpeedAlt(qQNaN())
{
}

WeatherData::ForecastInfo::ForecastInfo()
    : tempHigh(qQNaN())
    , tempLow(qQNaN())
    , windSpeed(qQNaN())
{
}

DWDIon::DWDIon(QObject *parent, const QVariantList &args)
    : IonInterface(parent, args)

{
    setInitialized(true);
}

DWDIon::~DWDIon()
{
    deleteForecasts();
}

void DWDIon::reset()
{
    deleteForecasts();
    m_sourcesToReset = sources();
    updateAllSources();
}

void DWDIon::deleteForecasts()
{
    // Destroy each forecast stored in a QVector
    for (auto it = m_weatherData.begin(), end = m_weatherData.end(); it != end; ++it) {
        qDeleteAll(it.value().forecasts);
        it.value().forecasts.clear();
    }
}

QMap<QString, IonInterface::ConditionIcons> DWDIon::setupDayIconMappings() const
{
    //    DWD supplies it's own icon number which we can use to determine a condition

    return QMap<QString, ConditionIcons>{{QStringLiteral("1"), ClearDay},
                                         {QStringLiteral("2"), PartlyCloudyDay},
                                         {QStringLiteral("3"), PartlyCloudyDay},
                                         {QStringLiteral("4"), Overcast},
                                         {QStringLiteral("5"), Mist},
                                         {QStringLiteral("6"), Mist},
                                         {QStringLiteral("7"), LightRain},
                                         {QStringLiteral("8"), Rain},
                                         {QStringLiteral("9"), Rain},
                                         {QStringLiteral("10"), LightRain},
                                         {QStringLiteral("11"), Rain},
                                         {QStringLiteral("12"), Flurries},
                                         {QStringLiteral("13"), RainSnow},
                                         {QStringLiteral("14"), LightSnow},
                                         {QStringLiteral("15"), Snow},
                                         {QStringLiteral("16"), Snow},
                                         {QStringLiteral("17"), Hail},
                                         {QStringLiteral("18"), LightRain},
                                         {QStringLiteral("19"), Rain},
                                         {QStringLiteral("20"), Flurries},
                                         {QStringLiteral("21"), RainSnow},
                                         {QStringLiteral("22"), LightSnow},
                                         {QStringLiteral("23"), Snow},
                                         {QStringLiteral("24"), Hail},
                                         {QStringLiteral("25"), Hail},
                                         {QStringLiteral("26"), Thunderstorm},
                                         {QStringLiteral("27"), Thunderstorm},
                                         {QStringLiteral("28"), Thunderstorm},
                                         {QStringLiteral("29"), Thunderstorm},
                                         {QStringLiteral("30"), Thunderstorm},
                                         {QStringLiteral("31"), ClearWindyDay}};
}

QMap<QString, IonInterface::WindDirections> DWDIon::setupWindIconMappings() const
{
    return QMap<QString, WindDirections>{
        {QStringLiteral("0"), N},     {QStringLiteral("10"), N},    {QStringLiteral("20"), NNE},  {QStringLiteral("30"), NNE},  {QStringLiteral("40"), NE},
        {QStringLiteral("50"), NE},   {QStringLiteral("60"), ENE},  {QStringLiteral("70"), ENE},  {QStringLiteral("80"), E},    {QStringLiteral("90"), E},
        {QStringLiteral("100"), E},   {QStringLiteral("120"), ESE}, {QStringLiteral("130"), ESE}, {QStringLiteral("140"), SE},  {QStringLiteral("150"), SE},
        {QStringLiteral("160"), SSE}, {QStringLiteral("170"), SSE}, {QStringLiteral("180"), S},   {QStringLiteral("190"), S},   {QStringLiteral("200"), SSW},
        {QStringLiteral("210"), SSW}, {QStringLiteral("220"), SW},  {QStringLiteral("230"), SW},  {QStringLiteral("240"), WSW}, {QStringLiteral("250"), WSW},
        {QStringLiteral("260"), W},   {QStringLiteral("270"), W},   {QStringLiteral("280"), W},   {QStringLiteral("290"), WNW}, {QStringLiteral("300"), WNW},
        {QStringLiteral("310"), NW},  {QStringLiteral("320"), NW},  {QStringLiteral("330"), NNW}, {QStringLiteral("340"), NNW}, {QStringLiteral("350"), N},
        {QStringLiteral("360"), N},
    };
}

QMap<QString, IonInterface::ConditionIcons> const &DWDIon::dayIcons() const
{
    static QMap<QString, ConditionIcons> const dval = setupDayIconMappings();
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
    // ionname|validate|place_name|extra - Triggers validation (search) of place
    // ionname|weather|place_name|extra - Triggers receiving weather of place
    const QStringList sourceAction = source.split(QLatin1Char('|'));

    if (sourceAction.size() < 3) {
        setData(source, QStringLiteral("validate"), QStringLiteral("dwd|malformed"));
        return true;
    }

    if (sourceAction[1] == QLatin1String("validate") && sourceAction.size() >= 3) {
        // Look for places to match
        findPlace(sourceAction[2]);
        return true;
    }
    if (sourceAction[1] == QLatin1String("weather") && sourceAction.size() >= 3) {
        if (sourceAction.count() >= 4) {
            if (sourceAction[2].isEmpty()) {
                setData(source, QStringLiteral("validate"), QStringLiteral("dwd|malformed"));
                return true;
            }

            // Extra data: station_id
            m_place[sourceAction[2]] = sourceAction[3];

            qCDebug(IONENGINE_dwd) << "About to retrieve forecast for source: " << sourceAction[2];

            fetchWeather(sourceAction[2], m_place[sourceAction[2]]);

            return true;
        }

        return false;
    }

    setData(source, QStringLiteral("validate"), QStringLiteral("dwd|malformed"));
    return true;
}

void DWDIon::findPlace(const QString &searchText)
{
    // Checks if the stations have already been loaded, always contains the currently active one
    if (m_place.size() > 1) {
        setData(QStringLiteral("dwd|validate|") + searchText, Data());
        searchInStationList(searchText);
    } else {
        const QUrl forecastURL(QStringLiteral(CATALOGUE_URL));
        KIO::TransferJob *getJob = KIO::get(forecastURL, KIO::Reload, KIO::HideProgressInfo);
        getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none"));

        m_searchJobList.insert(getJob, searchText);
        m_searchJobData.insert(getJob, QByteArray(""));

        connect(getJob, &KIO::TransferJob::data, this, &DWDIon::setup_slotDataArrived);
        connect(getJob, &KJob::result, this, &DWDIon::setup_slotJobFinished);
    }
}

void DWDIon::fetchWeather(QString placeName, QString placeID)
{
    for (const QString &fetching : qAsConst(m_forecastJobList)) {
        if (fetching == placeName) {
            // already fetching!
            return;
        }
    }

    // Fetch forecast data

    const QUrl forecastURL(QStringLiteral(FORECAST_URL).arg(placeID));
    KIO::TransferJob *getJob = KIO::get(forecastURL, KIO::Reload, KIO::HideProgressInfo);
    getJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none"));

    m_forecastJobList.insert(getJob, placeName);
    m_forecastJobJSON.insert(getJob, QByteArray(""));

    qCDebug(IONENGINE_dwd) << "Requesting URL: " << forecastURL;

    connect(getJob, &KIO::TransferJob::data, this, &DWDIon::forecast_slotDataArrived);
    connect(getJob, &KJob::result, this, &DWDIon::forecast_slotJobFinished);
    m_weatherData[placeName].isForecastsDataPending = true;

    // Fetch current measurements (different url AND different API, AMAZING)

    const QUrl measureURL(QStringLiteral(MEASURE_URL).arg(placeID));
    KIO::TransferJob *getMeasureJob = KIO::get(measureURL, KIO::Reload, KIO::HideProgressInfo);
    getMeasureJob->addMetaData(QStringLiteral("cookies"), QStringLiteral("none"));

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
        setData(QStringLiteral("dwd|validate|") + searchText, Data());

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
    if (!job->error()) {
        const QString source(m_measureJobList.value(job));
        setData(source, Data());

        QJsonDocument doc = QJsonDocument::fromJson(m_measureJobJSON.value(job));

        // Not all stations have current measurements
        if (!doc.isEmpty()) {
            parseMeasureData(source, doc);
        } else {
            m_weatherData[source].isMeasureDataPending = false;
            updateWeather(source);
        }
    } else {
        qCWarning(IONENGINE_dwd) << "error during measurement" << job->errorText();
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
            const QString weatherSource = QStringLiteral("dwd|weather|%1|%2").arg(source, m_place[source]);

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

void DWDIon::calculatePositions(QStringList lines, QVector<int> &namePositionalInfo, QVector<int> &stationIdPositionalInfo)
{
    QStringList stringLengths = lines[1].split(QChar::Space);
    QVector<int> lengths;
    for (const QString &length : qAsConst(stringLengths)) {
        lengths.append(length.count());
    }

    int curpos = 0;

    for (int labelLength : lengths) {
        QString label = lines[0].mid(curpos, labelLength).toLower();

        if (label.contains(QStringLiteral("name"))) {
            namePositionalInfo[0] = curpos;
            namePositionalInfo[1] = labelLength;
        } else if (label.contains(QStringLiteral("id"))) {
            stationIdPositionalInfo[0] = curpos;
            stationIdPositionalInfo[1] = labelLength;
        }

        curpos += labelLength + 1;
    }
}

void DWDIon::parseStationData(QByteArray data)
{
    QString stringData = QString::fromLatin1(data);
    QStringList lines = stringData.split(QChar::LineFeed);

    QVector<int> namePositionalInfo(2);
    QVector<int> stationIdPositionalInfo(2);
    calculatePositions(lines, namePositionalInfo, stationIdPositionalInfo);

    // This loop parses the station file (https://www.dwd.de/DE/leistungen/met_verfahren_mosmix/mosmix_stationskatalog.cfg)
    // ID    ICAO NAME                 LAT    LON     ELEV
    // ----- ---- -------------------- -----  ------- -----
    // 01001 ENJA JAN MAYEN             70.56   -8.40    10
    // 01008 ENSB SVALBARD              78.15   15.28    29
    int lineIndex = 0;
    for (const QString &line : qAsConst(lines)) {

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

void DWDIon::searchInStationList(const QString searchText)
{
    qCDebug(IONENGINE_dwd) << searchText;

    QMap<QString, QString>::const_iterator it = m_place.constBegin();
    auto end = m_place.constEnd();

    while (it != end) {
        QString name = it.key();
        if (name.contains(searchText, Qt::CaseInsensitive)) {
            m_locations.append(it.key());
        }
        ++it;
    }

    validate(searchText);
}

void DWDIon::parseForecastData(const QString source, QJsonDocument doc)
{
    QVariantMap weatherMap = doc.object().toVariantMap().first().toMap();
    if (!weatherMap.isEmpty()) {
        // Forecast data
        QVariantList daysList = weatherMap[QStringLiteral("days")].toList();

        WeatherData &weatherData = m_weatherData[source];
        QVector<WeatherData::ForecastInfo *> &forecasts = weatherData.forecasts;

        // Flush out the old forecasts when updating.
        forecasts.clear();

        WeatherData::ForecastInfo *forecast = new WeatherData::ForecastInfo;

        int dayNumber = 0;

        for (const QVariant &day : daysList) {
            QMap dayMap = day.toMap();
            QString period = dayMap[QStringLiteral("dayDate")].toString();
            QString cond = dayMap[QStringLiteral("icon")].toString();

            forecast->period = QDateTime::fromString(period, QStringLiteral("yyyy-MM-dd"));
            forecast->tempHigh = parseNumber(dayMap[QStringLiteral("temperatureMax")].toInt());
            forecast->tempLow = parseNumber(dayMap[QStringLiteral("temperatureMin")].toInt());
            forecast->iconName = getWeatherIcon(dayIcons(), cond);
            ;

            if (dayNumber == 0) {
                // These alternative measurements are used, when the stations doesn't have it's own measurements, uses forecast data from the current day
                weatherData.windSpeedAlt = parseNumber(dayMap[QStringLiteral("windSpeed")].toInt());
                weatherData.gustSpeedAlt = parseNumber(dayMap[QStringLiteral("windGust")].toInt());
                QString windDirection = roundWindDirections(dayMap[QStringLiteral("windDirection")].toInt());
                weatherData.windDirectionAlt = getWindDirectionIcon(windIcons(), windDirection);
            }

            forecasts.append(forecast);
            forecast = new WeatherData::ForecastInfo;

            dayNumber++;
            // Only get the next 7 days (including today)
            if (dayNumber == 7)
                break;
        }

        delete forecast;

        // Warnings data
        QVariantList warningData = weatherMap[QStringLiteral("warnings")].toList();

        QVector<WeatherData::WarningInfo *> &warningList = weatherData.warnings;

        // Flush out the old forecasts when updating.
        warningList.clear();

        WeatherData::WarningInfo *warning = new WeatherData::WarningInfo;

        for (const QVariant &warningElement : warningData) {
            QMap warningMap = warningElement.toMap();

            QString warningDesc = warningMap[QStringLiteral("description")].toString();

            // This loop adds line breaks because the weather widget doesn't seem to do that which completly ruins the layout
            // Should be removed if the weather widget is ever fixed
            for (int i = 1; i <= (warningDesc.size() / 50); i++) {
                warningDesc.insert(i * 50, QChar::LineFeed);
            }

            warning->description = warningDesc;
            warning->priority = warningMap[QStringLiteral("level")].toInt();
            warning->type = warningMap[QStringLiteral("event")].toString();
            warning->timestamp = QDateTime::fromMSecsSinceEpoch(warningMap[QStringLiteral("start")].toLongLong());

            warningList.append(warning);
            warning = new WeatherData::WarningInfo;
        }

        delete warning;

        weatherData.isForecastsDataPending = false;

        updateWeather(source);
    }
}

void DWDIon::parseMeasureData(const QString source, QJsonDocument doc)
{
    WeatherData &weatherData = m_weatherData[source];
    QVariantMap weatherMap = doc.object().toVariantMap();

    if (!weatherMap.isEmpty()) {
        bool windIconValid = false;
        bool tempValid = false;
        bool humidityValid = false;
        bool pressureValid = false;
        bool windSpeedValid = false;
        bool gustSpeedValid = false;
        bool dewpointValid = false;

        QDateTime time = QDateTime::fromMSecsSinceEpoch(weatherMap[QStringLiteral("time")].toLongLong());
        QString condIconNumber = weatherMap[QStringLiteral("icon")].toString();
        int windDirection = weatherMap[QStringLiteral("winddirection")].toInt(&windIconValid);
        float temp = parseNumber(weatherMap[QStringLiteral("temperature")].toInt(&tempValid));
        float humidity = parseNumber(weatherMap[QStringLiteral("humidity")].toInt(&humidityValid));
        float pressure = parseNumber(weatherMap[QStringLiteral("pressure")].toInt(&pressureValid));
        float windSpeed = parseNumber(weatherMap[QStringLiteral("meanwind")].toInt(&windSpeedValid));
        float gustSpeed = parseNumber(weatherMap[QStringLiteral("maxwind")].toInt(&gustSpeedValid));
        float dewpoint = parseNumber(weatherMap[QStringLiteral("dewpoint")].toInt(&dewpointValid));

        if (condIconNumber != QLatin1String(""))
            weatherData.conditionIcon = getWeatherIcon(dayIcons(), condIconNumber);
        if (windIconValid)
            weatherData.windDirection = getWindDirectionIcon(windIcons(), roundWindDirections(windDirection));
        if (tempValid)
            weatherData.temperature = temp;
        if (humidityValid)
            weatherData.humidity = humidity;
        if (pressureValid)
            weatherData.pressure = pressure;
        if (windSpeedValid)
            weatherData.windSpeed = windSpeed;
        if (gustSpeedValid)
            weatherData.gustSpeed = gustSpeed;
        if (dewpointValid)
            weatherData.dewpoint = dewpoint;
        weatherData.observationDateTime = time;
    }

    weatherData.isMeasureDataPending = false;

    updateWeather(source);
}

void DWDIon::validate(const QString &searchText)
{
    const QString source(QStringLiteral("dwd|validate|") + searchText);

    if (m_locations.isEmpty()) {
        const QString invalidPlace = searchText;
        setData(source, QStringLiteral("validate"), QVariant(QStringLiteral("dwd|invalid|multiple|") + invalidPlace));
        return;
    }

    QString placeList;
    for (const QString &place : qAsConst(m_locations)) {
        placeList.append(QStringLiteral("|place|") + place + QStringLiteral("|extra|") + m_place[place]);
    }
    if (m_locations.count() > 1) {
        setData(source, QStringLiteral("validate"), QVariant(QStringLiteral("dwd|valid|multiple") + placeList));
    } else {
        placeList[7] = placeList[7].toUpper();
        setData(source, QStringLiteral("validate"), QVariant(QStringLiteral("dwd|valid|single") + placeList));
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
    QString weatherSource = QStringLiteral("dwd|weather|%1|%2").arg(source, placeCode);

    Plasma::DataEngine::Data data;

    data.insert(QStringLiteral("Place"), source);
    data.insert(QStringLiteral("Station"), source);

    data.insert(QStringLiteral("Temperature Unit"), KUnitConversion::Celsius);
    data.insert(QStringLiteral("Wind Speed Unit"), KUnitConversion::KilometerPerHour);
    data.insert(QStringLiteral("Humidity Unit"), KUnitConversion::Percent);
    data.insert(QStringLiteral("Pressure Unit"), KUnitConversion::Hectopascal);

    if (!weatherData.observationDateTime.isNull())
        data.insert(QStringLiteral("Observation Timestamp"), weatherData.observationDateTime);
    else
        data.insert(QStringLiteral("Observation Timestamp"), QDateTime::currentDateTime());

    if (!weatherData.conditionIcon.isEmpty())
        data.insert(QStringLiteral("Condition Icon"), weatherData.conditionIcon);

    if (!qIsNaN(weatherData.temperature))
        data.insert(QStringLiteral("Temperature"), weatherData.temperature);

    if (!qIsNaN(weatherData.humidity))
        data.insert(QStringLiteral("Humidity"), weatherData.humidity);

    if (!qIsNaN(weatherData.pressure))
        data.insert(QStringLiteral("Pressure"), weatherData.pressure);

    if (!qIsNaN(weatherData.dewpoint))
        data.insert(QStringLiteral("Dewpoint"), weatherData.dewpoint);

    if (!qIsNaN(weatherData.windSpeed))
        data.insert(QStringLiteral("Wind Speed"), weatherData.windSpeed);
    else
        data.insert(QStringLiteral("Wind Speed"), weatherData.windSpeedAlt);

    if (!qIsNaN(weatherData.gustSpeed))
        data.insert(QStringLiteral("Wind Gust Speed"), weatherData.gustSpeed);
    else
        data.insert(QStringLiteral("Wind Gust Speed"), weatherData.gustSpeedAlt);

    if (!weatherData.windDirection.isEmpty()) {
        data.insert(QStringLiteral("Wind Direction"), weatherData.windDirection);
    } else {
        data.insert(QStringLiteral("Wind Direction"), weatherData.windDirectionAlt);
    }

    int dayNumber = 0;
    for (const WeatherData::ForecastInfo *dayForecast : weatherData.forecasts) {
        if (dayNumber > 0) {
            QString period = dayForecast->period.toString(QStringLiteral("dddd"));

            period.replace(QStringLiteral("Saturday"), i18nc("Short for Saturday", "Sat"));
            period.replace(QStringLiteral("Sunday"), i18nc("Short for Sunday", "Sun"));
            period.replace(QStringLiteral("Monday"), i18nc("Short for Monday", "Mon"));
            period.replace(QStringLiteral("Tuesday"), i18nc("Short for Tuesday", "Tue"));
            period.replace(QStringLiteral("Wednesday"), i18nc("Short for Wednesday", "Wed"));
            period.replace(QStringLiteral("Thursday"), i18nc("Short for Thursday", "Thu"));
            period.replace(QStringLiteral("Friday"), i18nc("Short for Friday", "Fri"));

            data.insert(QStringLiteral("Short Forecast Day %1").arg(dayNumber),
                        QStringLiteral("%1|%2|%3|%4|%5|%6")
                            .arg(period, dayForecast->iconName, QLatin1String(""))
                            .arg(dayForecast->tempHigh)
                            .arg(dayForecast->tempLow)
                            .arg(QLatin1String("")));
            dayNumber++;
        } else {
            data.insert(QStringLiteral("Short Forecast Day %1").arg(dayNumber),
                        QStringLiteral("%1|%2|%3|%4|%5|%6")
                            .arg(i18nc("Short for Today", "Today"), dayForecast->iconName, QLatin1String(""))
                            .arg(dayForecast->tempHigh)
                            .arg(dayForecast->tempLow)
                            .arg(QLatin1String("")));
            dayNumber++;
        }
    }

    int k = 0;

    for (const WeatherData::WarningInfo *warning : weatherData.warnings) {
        const QString number = QString::number(k);

        data.insert(QStringLiteral("Warning Priority ") + number, warning->priority);
        data.insert(QStringLiteral("Warning Description ") + number, warning->description);
        data.insert(QStringLiteral("Warning Timestamp ") + number, warning->timestamp.toString(QStringLiteral("dd.MM.yyyy")));

        ++k;
    }

    data.insert(QStringLiteral("Total Weather Days"), weatherData.forecasts.size());
    data.insert(QStringLiteral("Total Warnings Issued"), weatherData.warnings.size());
    data.insert(QStringLiteral("Credit"), i18nc("credit line, don't change name!", "Source: Deutscher Wetterdienst"));
    data.insert(QStringLiteral("Credit Url"), QStringLiteral("https://www.dwd.de/"));

    setData(weatherSource, data);
}

/*
 * Helper methods
 */

// e.g. DWD API int 17 equals 1.7
float DWDIon::parseNumber(int number)
{
    return ((float)number) / 10;
}

QString DWDIon::roundWindDirections(int windDirection)
{
    QString roundedWindDirection = QString::number(qRound(((float)windDirection) / 100) * 10);
    return roundedWindDirection;
}

QString DWDIon::extractString(QByteArray array, int start, int length)
{
    QString string;

    for (int i = start; i < start + length; i++) {
        string.append(QLatin1Char(array[i]));
    }

    return string;
}

QString DWDIon::camelCaseString(const QString text)
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
            if (c == QChar::Space || c == QLatin1Char('-')) {
                nextBig = true;
            }
            result.append(c);
        }
    }

    return result;
}

K_PLUGIN_CLASS_WITH_JSON(DWDIon, "ion-dwd.json")

#include "ion_dwd.moc"
