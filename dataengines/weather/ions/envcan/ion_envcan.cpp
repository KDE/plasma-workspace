/***************************************************************************
 *   Copyright (C) 2007-2011,2019 by Shawn Starr <shawn.starr@rogers.com>  *
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

/* Ion for Environment Canada XML data */

#include "ion_envcan.h"

#include "ion_envcandebug.h"

#include <KIO/Job>
#include <KLocalizedString>
#include <KUnitConversion/Converter>

#include <QRegularExpression>
#include <QTimeZone>

WeatherData::WeatherData()
    : stationLatitude(qQNaN())
    , stationLongitude(qQNaN())
    , temperature(qQNaN())
    , dewpoint(qQNaN())
    , windchill(qQNaN())
    , pressure(qQNaN())
    , visibility(qQNaN())
    , humidity(qQNaN())
    , windSpeed(qQNaN())
    , windGust(qQNaN())
    , normalHigh(qQNaN())
    , normalLow(qQNaN())
    , prevHigh(qQNaN())
    , prevLow(qQNaN())
    , recordHigh(qQNaN())
    , recordLow(qQNaN())
    , recordRain(qQNaN())
    , recordSnow(qQNaN())
{
}

WeatherData::ForecastInfo::ForecastInfo()
    : tempHigh(qQNaN())
    , tempLow(qQNaN())
    , popPrecent(qQNaN())
{
}

// ctor, dtor
EnvCanadaIon::EnvCanadaIon(QObject *parent, const QVariantList &args)
    : IonInterface(parent, args)
{
    // Get the real city XML URL so we can parse this
    getXMLSetup();
}

void EnvCanadaIon::deleteForecasts()
{
    QMutableHashIterator<QString, WeatherData> it(m_weatherData);
    while (it.hasNext()) {
        it.next();
        WeatherData &item = it.value();
        qDeleteAll(item.warnings);
        item.warnings.clear();

        qDeleteAll(item.watches);
        item.watches.clear();

        qDeleteAll(item.forecasts);
        item.forecasts.clear();
    }
}

void EnvCanadaIon::reset()
{
    deleteForecasts();
    emitWhenSetup = true;
    m_sourcesToReset = sources();
    getXMLSetup();
}

EnvCanadaIon::~EnvCanadaIon()
{
    // Destroy each watch/warning stored in a QVector
    deleteForecasts();
}

QMap<QString, IonInterface::ConditionIcons> EnvCanadaIon::setupConditionIconMappings() const
{
    return QMap<QString, ConditionIcons>{

        // Explicit periods
        {QStringLiteral("mainly sunny"), FewCloudsDay},
        {QStringLiteral("mainly clear"), FewCloudsNight},
        {QStringLiteral("sunny"), ClearDay},
        {QStringLiteral("clear"), ClearNight},

        // Available conditions
        {QStringLiteral("blowing snow"), Snow},
        {QStringLiteral("cloudy"), Overcast},
        {QStringLiteral("distant precipitation"), LightRain},
        {QStringLiteral("drifting snow"), Flurries},
        {QStringLiteral("drizzle"), LightRain},
        {QStringLiteral("dust"), NotAvailable},
        {QStringLiteral("dust devils"), NotAvailable},
        {QStringLiteral("fog"), Mist},
        {QStringLiteral("fog bank near station"), Mist},
        {QStringLiteral("fog depositing ice"), Mist},
        {QStringLiteral("fog patches"), Mist},
        {QStringLiteral("freezing drizzle"), FreezingDrizzle},
        {QStringLiteral("freezing rain"), FreezingRain},
        {QStringLiteral("funnel cloud"), NotAvailable},
        {QStringLiteral("hail"), Hail},
        {QStringLiteral("haze"), Haze},
        {QStringLiteral("heavy blowing snow"), Snow},
        {QStringLiteral("heavy drifting snow"), Snow},
        {QStringLiteral("heavy drizzle"), LightRain},
        {QStringLiteral("heavy hail"), Hail},
        {QStringLiteral("heavy mixed rain and drizzle"), LightRain},
        {QStringLiteral("heavy mixed rain and snow shower"), RainSnow},
        {QStringLiteral("heavy rain"), Rain},
        {QStringLiteral("heavy rain and snow"), RainSnow},
        {QStringLiteral("heavy rainshower"), Rain},
        {QStringLiteral("heavy snow"), Snow},
        {QStringLiteral("heavy snow pellets"), Snow},
        {QStringLiteral("heavy snowshower"), Snow},
        {QStringLiteral("heavy thunderstorm with hail"), Thunderstorm},
        {QStringLiteral("heavy thunderstorm with rain"), Thunderstorm},
        {QStringLiteral("ice crystals"), Flurries},
        {QStringLiteral("ice pellets"), Hail},
        {QStringLiteral("increasing cloud"), Overcast},
        {QStringLiteral("light drizzle"), LightRain},
        {QStringLiteral("light freezing drizzle"), FreezingRain},
        {QStringLiteral("light freezing rain"), FreezingRain},
        {QStringLiteral("light rain"), LightRain},
        {QStringLiteral("light rainshower"), LightRain},
        {QStringLiteral("light snow"), LightSnow},
        {QStringLiteral("light snow pellets"), LightSnow},
        {QStringLiteral("light snowshower"), Flurries},
        {QStringLiteral("lightning visible"), Thunderstorm},
        {QStringLiteral("mist"), Mist},
        {QStringLiteral("mixed rain and drizzle"), LightRain},
        {QStringLiteral("mixed rain and snow shower"), RainSnow},
        {QStringLiteral("not reported"), NotAvailable},
        {QStringLiteral("rain"), Rain},
        {QStringLiteral("rain and snow"), RainSnow},
        {QStringLiteral("rainshower"), LightRain},
        {QStringLiteral("recent drizzle"), LightRain},
        {QStringLiteral("recent dust or sand storm"), NotAvailable},
        {QStringLiteral("recent fog"), Mist},
        {QStringLiteral("recent freezing precipitation"), FreezingDrizzle},
        {QStringLiteral("recent hail"), Hail},
        {QStringLiteral("recent rain"), Rain},
        {QStringLiteral("recent rain and snow"), RainSnow},
        {QStringLiteral("recent rainshower"), Rain},
        {QStringLiteral("recent snow"), Snow},
        {QStringLiteral("recent snowshower"), Flurries},
        {QStringLiteral("recent thunderstorm"), Thunderstorm},
        {QStringLiteral("recent thunderstorm with hail"), Thunderstorm},
        {QStringLiteral("recent thunderstorm with heavy hail"), Thunderstorm},
        {QStringLiteral("recent thunderstorm with heavy rain"), Thunderstorm},
        {QStringLiteral("recent thunderstorm with rain"), Thunderstorm},
        {QStringLiteral("sand or dust storm"), NotAvailable},
        {QStringLiteral("severe sand or dust storm"), NotAvailable},
        {QStringLiteral("shallow fog"), Mist},
        {QStringLiteral("smoke"), NotAvailable},
        {QStringLiteral("snow"), Snow},
        {QStringLiteral("snow crystals"), Flurries},
        {QStringLiteral("snow grains"), Flurries},
        {QStringLiteral("squalls"), Snow},
        {QStringLiteral("thunderstorm"), Thunderstorm},
        {QStringLiteral("thunderstorm with hail"), Thunderstorm},
        {QStringLiteral("thunderstorm with rain"), Thunderstorm},
        {QStringLiteral("thunderstorm with light rainshowers"), Thunderstorm},
        {QStringLiteral("thunderstorm with heavy rainshowers"), Thunderstorm},
        {QStringLiteral("thunderstorm with sand or dust storm"), Thunderstorm},
        {QStringLiteral("thunderstorm without precipitation"), Thunderstorm},
        {QStringLiteral("tornado"), NotAvailable},
    };
}

QMap<QString, IonInterface::ConditionIcons> EnvCanadaIon::setupForecastIconMappings() const
{
    return QMap<QString, ConditionIcons>{

        // Abbreviated forecast descriptions
        {QStringLiteral("a few flurries"), Flurries},
        {QStringLiteral("a few flurries mixed with ice pellets"), RainSnow},
        {QStringLiteral("a few flurries or rain showers"), RainSnow},
        {QStringLiteral("a few flurries or thundershowers"), RainSnow},
        {QStringLiteral("a few rain showers or flurries"), RainSnow},
        {QStringLiteral("a few rain showers or wet flurries"), RainSnow},
        {QStringLiteral("a few showers"), LightRain},
        {QStringLiteral("a few showers or drizzle"), LightRain},
        {QStringLiteral("a few showers or thundershowers"), Thunderstorm},
        {QStringLiteral("a few showers or thunderstorms"), Thunderstorm},
        {QStringLiteral("a few thundershowers"), Thunderstorm},
        {QStringLiteral("a few thunderstorms"), Thunderstorm},
        {QStringLiteral("a few wet flurries"), RainSnow},
        {QStringLiteral("a few wet flurries or rain showers"), RainSnow},
        {QStringLiteral("a mix of sun and cloud"), PartlyCloudyDay},
        {QStringLiteral("cloudy with sunny periods"), PartlyCloudyDay},
        {QStringLiteral("partly cloudy"), PartlyCloudyDay},
        {QStringLiteral("mainly cloudy"), PartlyCloudyDay},
        {QStringLiteral("mainly sunny"), FewCloudsDay},
        {QStringLiteral("sunny"), ClearDay},
        {QStringLiteral("blizzard"), Snow},
        {QStringLiteral("clear"), ClearNight},
        {QStringLiteral("cloudy"), Overcast},
        {QStringLiteral("drizzle"), LightRain},
        {QStringLiteral("drizzle mixed with freezing drizzle"), FreezingDrizzle},
        {QStringLiteral("drizzle mixed with rain"), LightRain},
        {QStringLiteral("drizzle or freezing drizzle"), LightRain},
        {QStringLiteral("drizzle or rain"), LightRain},
        {QStringLiteral("flurries"), Flurries},
        {QStringLiteral("flurries at times heavy"), Flurries},
        {QStringLiteral("flurries at times heavy or rain snowers"), RainSnow},
        {QStringLiteral("flurries mixed with ice pellets"), FreezingRain},
        {QStringLiteral("flurries or ice pellets"), FreezingRain},
        {QStringLiteral("flurries or rain showers"), RainSnow},
        {QStringLiteral("flurries or thundershowers"), Flurries},
        {QStringLiteral("fog"), Mist},
        {QStringLiteral("fog developing"), Mist},
        {QStringLiteral("fog dissipating"), Mist},
        {QStringLiteral("fog patches"), Mist},
        {QStringLiteral("freezing drizzle"), FreezingDrizzle},
        {QStringLiteral("freezing rain"), FreezingRain},
        {QStringLiteral("freezing rain mixed with rain"), FreezingRain},
        {QStringLiteral("freezing rain mixed with snow"), FreezingRain},
        {QStringLiteral("freezing rain or ice pellets"), FreezingRain},
        {QStringLiteral("freezing rain or rain"), FreezingRain},
        {QStringLiteral("freezing rain or snow"), FreezingRain},
        {QStringLiteral("ice fog"), Mist},
        {QStringLiteral("ice fog developing"), Mist},
        {QStringLiteral("ice fog dissipating"), Mist},
        {QStringLiteral("ice pellets"), Hail},
        {QStringLiteral("ice pellets mixed with freezing rain"), Hail},
        {QStringLiteral("ice pellets mixed with snow"), Hail},
        {QStringLiteral("ice pellets or snow"), RainSnow},
        {QStringLiteral("light snow"), LightSnow},
        {QStringLiteral("light snow and blizzard"), LightSnow},
        {QStringLiteral("light snow and blizzard and blowing snow"), Snow},
        {QStringLiteral("light snow and blowing snow"), LightSnow},
        {QStringLiteral("light snow mixed with freezing drizzle"), FreezingDrizzle},
        {QStringLiteral("light snow mixed with freezing rain"), FreezingRain},
        {QStringLiteral("light snow or ice pellets"), LightSnow},
        {QStringLiteral("light snow or rain"), RainSnow},
        {QStringLiteral("light wet snow"), RainSnow},
        {QStringLiteral("light wet snow or rain"), RainSnow},
        {QStringLiteral("local snow squalls"), Snow},
        {QStringLiteral("near blizzard"), Snow},
        {QStringLiteral("overcast"), Overcast},
        {QStringLiteral("increasing cloudiness"), Overcast},
        {QStringLiteral("increasing clouds"), Overcast},
        {QStringLiteral("periods of drizzle"), LightRain},
        {QStringLiteral("periods of drizzle mixed with freezing drizzle"), FreezingDrizzle},
        {QStringLiteral("periods of drizzle mixed with rain"), LightRain},
        {QStringLiteral("periods of drizzle or freezing drizzle"), FreezingDrizzle},
        {QStringLiteral("periods of drizzle or rain"), LightRain},
        {QStringLiteral("periods of freezing drizzle"), FreezingDrizzle},
        {QStringLiteral("periods of freezing drizzle or drizzle"), FreezingDrizzle},
        {QStringLiteral("periods of freezing drizzle or rain"), FreezingDrizzle},
        {QStringLiteral("periods of freezing rain"), FreezingRain},
        {QStringLiteral("periods of freezing rain mixed with ice pellets"), FreezingRain},
        {QStringLiteral("periods of freezing rain mixed with rain"), FreezingRain},
        {QStringLiteral("periods of freezing rain mixed with snow"), FreezingRain},
        {QStringLiteral("periods of freezing rain mixed with freezing drizzle"), FreezingRain},
        {QStringLiteral("periods of freezing rain or ice pellets"), FreezingRain},
        {QStringLiteral("periods of freezing rain or rain"), FreezingRain},
        {QStringLiteral("periods of freezing rain or snow"), FreezingRain},
        {QStringLiteral("periods of ice pellets"), Hail},
        {QStringLiteral("periods of ice pellets mixed with freezing rain"), Hail},
        {QStringLiteral("periods of ice pellets mixed with snow"), Hail},
        {QStringLiteral("periods of ice pellets or freezing rain"), Hail},
        {QStringLiteral("periods of ice pellets or snow"), Hail},
        {QStringLiteral("periods of light snow"), LightSnow},
        {QStringLiteral("periods of light snow and blizzard"), Snow},
        {QStringLiteral("periods of light snow and blizzard and blowing snow"), Snow},
        {QStringLiteral("periods of light snow and blowing snow"), LightSnow},
        {QStringLiteral("periods of light snow mixed with freezing drizzle"), RainSnow},
        {QStringLiteral("periods of light snow mixed with freezing rain"), RainSnow},
        {QStringLiteral("periods of light snow mixed with ice pellets"), LightSnow},
        {QStringLiteral("periods of light snow mixed with rain"), RainSnow},
        {QStringLiteral("periods of light snow or freezing drizzle"), RainSnow},
        {QStringLiteral("periods of light snow or freezing rain"), RainSnow},
        {QStringLiteral("periods of light snow or ice pellets"), LightSnow},
        {QStringLiteral("periods of light snow or rain"), RainSnow},
        {QStringLiteral("periods of light wet snow"), LightSnow},
        {QStringLiteral("periods of light wet snow mixed with rain"), RainSnow},
        {QStringLiteral("periods of light wet snow or rain"), RainSnow},
        {QStringLiteral("periods of rain"), Rain},
        {QStringLiteral("periods of rain mixed with freezing rain"), Rain},
        {QStringLiteral("periods of rain mixed with snow"), RainSnow},
        {QStringLiteral("periods of rain or drizzle"), Rain},
        {QStringLiteral("periods of rain or freezing rain"), Rain},
        {QStringLiteral("periods of rain or thundershowers"), Showers},
        {QStringLiteral("periods of rain or thunderstorms"), Thunderstorm},
        {QStringLiteral("periods of rain or snow"), RainSnow},
        {QStringLiteral("periods of snow"), Snow},
        {QStringLiteral("periods of snow and blizzard"), Snow},
        {QStringLiteral("periods of snow and blizzard and blowing snow"), Snow},
        {QStringLiteral("periods of snow and blowing snow"), Snow},
        {QStringLiteral("periods of snow mixed with freezing drizzle"), RainSnow},
        {QStringLiteral("periods of snow mixed with freezing rain"), RainSnow},
        {QStringLiteral("periods of snow mixed with ice pellets"), Snow},
        {QStringLiteral("periods of snow mixed with rain"), RainSnow},
        {QStringLiteral("periods of snow or freezing drizzle"), RainSnow},
        {QStringLiteral("periods of snow or freezing rain"), RainSnow},
        {QStringLiteral("periods of snow or ice pellets"), Snow},
        {QStringLiteral("periods of snow or rain"), RainSnow},
        {QStringLiteral("periods of rain or snow"), RainSnow},
        {QStringLiteral("periods of wet snow"), Snow},
        {QStringLiteral("periods of wet snow mixed with rain"), RainSnow},
        {QStringLiteral("periods of wet snow or rain"), RainSnow},
        {QStringLiteral("rain"), Rain},
        {QStringLiteral("rain at times heavy"), Rain},
        {QStringLiteral("rain at times heavy mixed with freezing rain"), FreezingRain},
        {QStringLiteral("rain at times heavy mixed with snow"), RainSnow},
        {QStringLiteral("rain at times heavy or drizzle"), Rain},
        {QStringLiteral("rain at times heavy or freezing rain"), Rain},
        {QStringLiteral("rain at times heavy or snow"), RainSnow},
        {QStringLiteral("rain at times heavy or thundershowers"), Showers},
        {QStringLiteral("rain at times heavy or thunderstorms"), Thunderstorm},
        {QStringLiteral("rain mixed with freezing rain"), FreezingRain},
        {QStringLiteral("rain mixed with snow"), RainSnow},
        {QStringLiteral("rain or drizzle"), Rain},
        {QStringLiteral("rain or freezing rain"), Rain},
        {QStringLiteral("rain or snow"), RainSnow},
        {QStringLiteral("rain or thundershowers"), Showers},
        {QStringLiteral("rain or thunderstorms"), Thunderstorm},
        {QStringLiteral("rain showers or flurries"), RainSnow},
        {QStringLiteral("rain showers or wet flurries"), RainSnow},
        {QStringLiteral("showers"), Showers},
        {QStringLiteral("showers at times heavy"), Showers},
        {QStringLiteral("showers at times heavy or thundershowers"), Showers},
        {QStringLiteral("showers at times heavy or thunderstorms"), Thunderstorm},
        {QStringLiteral("showers or drizzle"), Showers},
        {QStringLiteral("showers or thundershowers"), Thunderstorm},
        {QStringLiteral("showers or thunderstorms"), Thunderstorm},
        {QStringLiteral("smoke"), NotAvailable},
        {QStringLiteral("snow"), Snow},
        {QStringLiteral("snow and blizzard"), Snow},
        {QStringLiteral("snow and blizzard and blowing snow"), Snow},
        {QStringLiteral("snow and blowing snow"), Snow},
        {QStringLiteral("snow at times heavy"), Snow},
        {QStringLiteral("snow at times heavy and blizzard"), Snow},
        {QStringLiteral("snow at times heavy and blowing snow"), Snow},
        {QStringLiteral("snow at times heavy mixed with freezing drizzle"), RainSnow},
        {QStringLiteral("snow at times heavy mixed with freezing rain"), RainSnow},
        {QStringLiteral("snow at times heavy mixed with ice pellets"), Snow},
        {QStringLiteral("snow at times heavy mixed with rain"), RainSnow},
        {QStringLiteral("snow at times heavy or freezing rain"), RainSnow},
        {QStringLiteral("snow at times heavy or ice pellets"), Snow},
        {QStringLiteral("snow at times heavy or rain"), RainSnow},
        {QStringLiteral("snow mixed with freezing drizzle"), RainSnow},
        {QStringLiteral("snow mixed with freezing rain"), RainSnow},
        {QStringLiteral("snow mixed with ice pellets"), Snow},
        {QStringLiteral("snow mixed with rain"), RainSnow},
        {QStringLiteral("snow or freezing drizzle"), RainSnow},
        {QStringLiteral("snow or freezing rain"), RainSnow},
        {QStringLiteral("snow or ice pellets"), Snow},
        {QStringLiteral("snow or rain"), RainSnow},
        {QStringLiteral("snow squalls"), Snow},
        {QStringLiteral("sunny"), ClearDay},
        {QStringLiteral("sunny with cloudy periods"), PartlyCloudyDay},
        {QStringLiteral("thunderstorms"), Thunderstorm},
        {QStringLiteral("thunderstorms and possible hail"), Thunderstorm},
        {QStringLiteral("wet flurries"), Flurries},
        {QStringLiteral("wet flurries at times heavy"), Flurries},
        {QStringLiteral("wet flurries at times heavy or rain snowers"), RainSnow},
        {QStringLiteral("wet flurries or rain showers"), RainSnow},
        {QStringLiteral("wet snow"), Snow},
        {QStringLiteral("wet snow at times heavy"), Snow},
        {QStringLiteral("wet snow at times heavy mixed with rain"), RainSnow},
        {QStringLiteral("wet snow mixed with rain"), RainSnow},
        {QStringLiteral("wet snow or rain"), RainSnow},
        {QStringLiteral("windy"), NotAvailable},

        {QStringLiteral("chance of drizzle mixed with freezing drizzle"), LightRain},
        {QStringLiteral("chance of flurries mixed with ice pellets"), Flurries},
        {QStringLiteral("chance of flurries or ice pellets"), Flurries},
        {QStringLiteral("chance of flurries or rain showers"), RainSnow},
        {QStringLiteral("chance of flurries or thundershowers"), RainSnow},
        {QStringLiteral("chance of freezing drizzle"), FreezingDrizzle},
        {QStringLiteral("chance of freezing rain"), FreezingRain},
        {QStringLiteral("chance of freezing rain mixed with snow"), RainSnow},
        {QStringLiteral("chance of freezing rain or rain"), FreezingRain},
        {QStringLiteral("chance of freezing rain or snow"), RainSnow},
        {QStringLiteral("chance of light snow and blowing snow"), LightSnow},
        {QStringLiteral("chance of light snow mixed with freezing drizzle"), LightSnow},
        {QStringLiteral("chance of light snow mixed with ice pellets"), LightSnow},
        {QStringLiteral("chance of light snow mixed with rain"), RainSnow},
        {QStringLiteral("chance of light snow or freezing rain"), RainSnow},
        {QStringLiteral("chance of light snow or ice pellets"), LightSnow},
        {QStringLiteral("chance of light snow or rain"), RainSnow},
        {QStringLiteral("chance of light wet snow"), Snow},
        {QStringLiteral("chance of rain"), Rain},
        {QStringLiteral("chance of rain at times heavy"), Rain},
        {QStringLiteral("chance of rain mixed with snow"), RainSnow},
        {QStringLiteral("chance of rain or drizzle"), Rain},
        {QStringLiteral("chance of rain or freezing rain"), Rain},
        {QStringLiteral("chance of rain or snow"), RainSnow},
        {QStringLiteral("chance of rain showers or flurries"), RainSnow},
        {QStringLiteral("chance of rain showers or wet flurries"), RainSnow},
        {QStringLiteral("chance of severe thunderstorms"), Thunderstorm},
        {QStringLiteral("chance of showers at times heavy"), Rain},
        {QStringLiteral("chance of showers at times heavy or thundershowers"), Thunderstorm},
        {QStringLiteral("chance of showers at times heavy or thunderstorms"), Thunderstorm},
        {QStringLiteral("chance of showers or thundershowers"), Thunderstorm},
        {QStringLiteral("chance of showers or thunderstorms"), Thunderstorm},
        {QStringLiteral("chance of snow"), Snow},
        {QStringLiteral("chance of snow and blizzard"), Snow},
        {QStringLiteral("chance of snow mixed with freezing drizzle"), Snow},
        {QStringLiteral("chance of snow mixed with freezing rain"), RainSnow},
        {QStringLiteral("chance of snow mixed with rain"), RainSnow},
        {QStringLiteral("chance of snow or rain"), RainSnow},
        {QStringLiteral("chance of snow squalls"), Snow},
        {QStringLiteral("chance of thundershowers"), Showers},
        {QStringLiteral("chance of thunderstorms"), Thunderstorm},
        {QStringLiteral("chance of thunderstorms and possible hail"), Thunderstorm},
        {QStringLiteral("chance of wet flurries"), Flurries},
        {QStringLiteral("chance of wet flurries at times heavy"), Flurries},
        {QStringLiteral("chance of wet flurries or rain showers"), RainSnow},
        {QStringLiteral("chance of wet snow"), Snow},
        {QStringLiteral("chance of wet snow mixed with rain"), RainSnow},
        {QStringLiteral("chance of wet snow or rain"), RainSnow},
    };
}

QMap<QString, IonInterface::ConditionIcons> const &EnvCanadaIon::conditionIcons() const
{
    static QMap<QString, ConditionIcons> const condval = setupConditionIconMappings();
    return condval;
}

QMap<QString, IonInterface::ConditionIcons> const &EnvCanadaIon::forecastIcons() const
{
    static QMap<QString, ConditionIcons> const foreval = setupForecastIconMappings();
    return foreval;
}

QStringList EnvCanadaIon::validate(const QString &source) const
{
    QStringList placeList;

    QString sourceNormalized = source.toUpper();
    QHash<QString, EnvCanadaIon::XMLMapInfo>::const_iterator it = m_places.constBegin();
    while (it != m_places.constEnd()) {
        if (it.key().toUpper().contains(sourceNormalized)) {
            placeList.append(QStringLiteral("place|") + it.key());
        }
        ++it;
    }

    placeList.sort();
    return placeList;
}

// Get a specific Ion's data
bool EnvCanadaIon::updateIonSource(const QString &source)
{
    // qCDebug(IONENGINE_ENVCAN) << "updateIonSource()" << source;

    // We expect the applet to send the source in the following tokenization:
    // ionname|validate|place_name - Triggers validation of place
    // ionname|weather|place_name - Triggers receiving weather of place

    const QStringList sourceAction = source.split(QLatin1Char('|'));

    // Guard: if the size of array is not 2 then we have bad data, return an error
    if (sourceAction.size() < 2) {
        setData(source, QStringLiteral("validate"), QStringLiteral("envcan|malformed"));
        return true;
    }

    if (sourceAction[1] == QLatin1String("validate") && sourceAction.size() > 2) {
        const QStringList result = validate(sourceAction[2]);

        const QString reply = (result.size() == 1) ? QStringLiteral("envcan|valid|single|") + result[0]
                                                   : (result.size() > 1) ? QStringLiteral("envcan|valid|multiple|") + result.join(QLatin1Char('|')) :
                                                                         /*else*/ QStringLiteral("envcan|invalid|single|") + sourceAction[2];
        setData(source, QStringLiteral("validate"), reply);

        return true;
    }
    if (sourceAction[1] == QLatin1String("weather") && sourceAction.size() > 2) {
        getXMLData(source);
        return true;
    }
    setData(source, QStringLiteral("validate"), QStringLiteral("envcan|malformed"));
    return true;
}

// Parses city list and gets the correct city based on ID number
void EnvCanadaIon::getXMLSetup()
{
    // qCDebug(IONENGINE_ENVCAN) << "getXMLSetup()";

    // If network is down, we need to spin and wait

    const QUrl url(QStringLiteral("http://dd.weather.gc.ca/citypage_weather/xml/siteList.xml"));

    KIO::TransferJob *getJob = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);

    m_xmlSetup.clear();
    connect(getJob, &KIO::TransferJob::data, this, &EnvCanadaIon::setup_slotDataArrived);
    connect(getJob, &KJob::result, this, &EnvCanadaIon::setup_slotJobFinished);
}

// Gets specific city XML data
void EnvCanadaIon::getXMLData(const QString &source)
{
    for (const QString &fetching : qAsConst(m_jobList)) {
        if (fetching == source) {
            // already getting this source and awaiting the data
            return;
        }
    }

    // qCDebug(IONENGINE_ENVCAN) << source;

    // Demunge source name for key only.
    QString dataKey = source;
    dataKey.remove(QStringLiteral("envcan|weather|"));
    const XMLMapInfo &place = m_places[dataKey];

    const QUrl url(QLatin1String("http://dd.weather.gc.ca/citypage_weather/xml/") + place.territoryName + QLatin1Char('/') + place.cityCode
                   + QStringLiteral("_e.xml"));
    // url="file:///home/spstarr/Desktop/s0000649_e.xml";
    // qCDebug(IONENGINE_ENVCAN) << "Will Try URL: " << url;

    if (place.territoryName.isEmpty() && place.cityCode.isEmpty()) {
        setData(source, QStringLiteral("validate"), QStringLiteral("envcan|malformed"));
        return;
    }

    KIO::TransferJob *getJob = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);

    m_jobXml.insert(getJob, new QXmlStreamReader);
    m_jobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data, this, &EnvCanadaIon::slotDataArrived);
    connect(getJob, &KJob::result, this, &EnvCanadaIon::slotJobFinished);
}

void EnvCanadaIon::setup_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)

    if (data.isEmpty()) {
        // qCDebug(IONENGINE_ENVCAN) << "done!";
        return;
    }

    // Send to xml.
    // qCDebug(IONENGINE_ENVCAN) << data;
    m_xmlSetup.addData(data);
}

void EnvCanadaIon::slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    if (data.isEmpty() || !m_jobXml.contains(job)) {
        return;
    }

    // Send to xml.
    m_jobXml[job]->addData(data);
}

void EnvCanadaIon::slotJobFinished(KJob *job)
{
    // Dual use method, if we're fetching location data to parse we need to do this first
    const QString source = m_jobList.value(job);
    // qCDebug(IONENGINE_ENVCAN) << source << m_sourcesToReset.contains(source);
    setData(source, Data());
    QXmlStreamReader *reader = m_jobXml.value(job);
    if (reader) {
        readXMLData(m_jobList[job], *reader);
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

void EnvCanadaIon::setup_slotJobFinished(KJob *job)
{
    Q_UNUSED(job)
    const bool success = readXMLSetup();
    m_xmlSetup.clear();
    // qCDebug(IONENGINE_ENVCAN) << success << m_sourcesToReset;
    setInitialized(success);
}

// Parse the city list and store into a QMap
bool EnvCanadaIon::readXMLSetup()
{
    bool success = false;
    QString territory;
    QString code;
    QString cityName;

    // qCDebug(IONENGINE_ENVCAN) << "readXMLSetup()";

    while (!m_xmlSetup.atEnd()) {
        m_xmlSetup.readNext();

        const QStringRef elementName = m_xmlSetup.name();

        if (m_xmlSetup.isStartElement()) {
            // XML ID code to match filename
            if (elementName == QLatin1String("site")) {
                code = m_xmlSetup.attributes().value(QStringLiteral("code")).toString();
            }

            if (elementName == QLatin1String("nameEn")) {
                cityName = m_xmlSetup.readElementText(); // Name of cities
            }

            if (elementName == QLatin1String("provinceCode")) {
                territory = m_xmlSetup.readElementText(); // Provinces/Territory list
            }
        }

        if (m_xmlSetup.isEndElement() && elementName == QLatin1String("site")) {
            EnvCanadaIon::XMLMapInfo info;
            QString tmp = cityName + QStringLiteral(", ") + territory; // Build the key name.

            // Set the mappings
            info.cityCode = code;
            info.territoryName = territory;
            info.cityName = cityName;

            // Set the string list, we will use for the applet to display the available cities.
            m_places[tmp] = info;
            success = true;
        }
    }

    return (success && !m_xmlSetup.error());
}

void EnvCanadaIon::parseWeatherSite(WeatherData &data, QXmlStreamReader &xml)
{
    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("license")) {
                data.creditUrl = xml.readElementText();
            } else if (elementName == QLatin1String("location")) {
                parseLocations(data, xml);
            } else if (elementName == QLatin1String("warnings")) {
                // Cleanup warning list on update
                data.warnings.clear();
                data.watches.clear();
                parseWarnings(data, xml);
            } else if (elementName == QLatin1String("currentConditions")) {
                parseConditions(data, xml);
            } else if (elementName == QLatin1String("forecastGroup")) {
                // Clean up forecast list on update
                data.forecasts.clear();
                parseWeatherForecast(data, xml);
            } else if (elementName == QLatin1String("yesterdayConditions")) {
                parseYesterdayWeather(data, xml);
            } else if (elementName == QLatin1String("riseSet")) {
                parseAstronomicals(data, xml);
            } else if (elementName == QLatin1String("almanac")) {
                parseWeatherRecords(data, xml);
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

// Parse Weather data main loop, from here we have to descend into each tag pair
bool EnvCanadaIon::readXMLData(const QString &source, QXmlStreamReader &xml)
{
    WeatherData data;

    // qCDebug(IONENGINE_ENVCAN) << "readXMLData()";

    QString dataKey = source;
    dataKey.remove(QStringLiteral("envcan|weather|"));
    data.shortTerritoryName = m_places[dataKey].territoryName;
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("siteData")) {
                parseWeatherSite(data, xml);
            } else {
                parseUnknownElement(xml);
            }
        }
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
        timeEngine->connectSource(data.solarDataTimeEngineSourceName, this);
    } else {
        updateWeather(source);
    }

    return !xml.error();
}

void EnvCanadaIon::parseFloat(float &value, QXmlStreamReader &xml)
{
    bool ok = false;
    const float result = xml.readElementText().toFloat(&ok);
    if (ok) {
        value = result;
    }
}

void EnvCanadaIon::parseDateTime(WeatherData &data, QXmlStreamReader &xml, WeatherData::WeatherEvent *event)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("dateTime"));

    // What kind of date info is this?
    const QString dateType = xml.attributes().value(QStringLiteral("name")).toString();
    const QString dateZone = xml.attributes().value(QStringLiteral("zone")).toString();
    const QString dateUtcOffset = xml.attributes().value(QStringLiteral("UTCOffset")).toString();

    QString selectTimeStamp;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        const QStringRef elementName = xml.name();

        if (xml.isStartElement()) {
            if (dateType == QLatin1String("xmlCreation")) {
                return;
            }
            if (dateZone == QLatin1String("UTC")) {
                return;
            }
            if (elementName == QLatin1String("year")) {
                xml.readElementText();
            } else if (elementName == QLatin1String("month")) {
                xml.readElementText();
            } else if (elementName == QLatin1String("day")) {
                xml.readElementText();
            } else if (elementName == QLatin1String("hour"))
                xml.readElementText();
            else if (elementName == QLatin1String("minute"))
                xml.readElementText();
            else if (elementName == QLatin1String("timeStamp"))
                selectTimeStamp = xml.readElementText();
            else if (elementName == QLatin1String("textSummary")) {
                if (dateType == QLatin1String("eventIssue")) {
                    if (event) {
                        event->timestamp = xml.readElementText();
                    }
                } else if (dateType == QLatin1String("observation")) {
                    xml.readElementText();
                    QDateTime observationDateTime = QDateTime::fromString(selectTimeStamp, QStringLiteral("yyyyMMddHHmmss"));
                    QTimeZone timeZone = QTimeZone(dateZone.toUtf8());
                    // if timezone id not recognized, fallback to utcoffset
                    if (!timeZone.isValid()) {
                        timeZone = QTimeZone(dateUtcOffset.toInt() * 3600);
                    }
                    if (observationDateTime.isValid() && timeZone.isValid()) {
                        data.observationDateTime = observationDateTime;
                        data.observationDateTime.setTimeZone(timeZone);
                    }
                    data.obsTimestamp = observationDateTime.toString(QStringLiteral("dd.MM.yyyy @ hh:mm"));
                } else if (dateType == QLatin1String("forecastIssue")) {
                    data.forecastTimestamp = xml.readElementText();
                } else if (dateType == QLatin1String("sunrise")) {
                    data.sunriseTimestamp = xml.readElementText();
                } else if (dateType == QLatin1String("sunset")) {
                    data.sunsetTimestamp = xml.readElementText();
                } else if (dateType == QLatin1String("moonrise")) {
                    data.moonriseTimestamp = xml.readElementText();
                } else if (dateType == QLatin1String("moonset")) {
                    data.moonsetTimestamp = xml.readElementText();
                }
            }
        }
    }
}

void EnvCanadaIon::parseLocations(WeatherData &data, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("location"));

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        const QStringRef elementName = xml.name();

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("country")) {
                data.countryName = xml.readElementText();
            } else if (elementName == QLatin1String("province") || elementName == QLatin1String("territory")) {
                data.longTerritoryName = xml.readElementText();
            } else if (elementName == QLatin1String("name")) {
                data.cityName = xml.readElementText();
            } else if (elementName == QLatin1String("region")) {
                data.regionName = xml.readElementText();
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

void EnvCanadaIon::parseWindInfo(WeatherData &data, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("wind"));

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        const QStringRef elementName = xml.name();

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("speed")) {
                parseFloat(data.windSpeed, xml);
            } else if (elementName == QLatin1String("gust")) {
                parseFloat(data.windGust, xml);
            } else if (elementName == QLatin1String("direction")) {
                data.windDirection = xml.readElementText();
            } else if (elementName == QLatin1String("bearing")) {
                data.windDegrees = xml.attributes().value(QStringLiteral("degrees")).toString();
            } else {
                parseUnknownElement(xml);
            }
        }
    }
}

void EnvCanadaIon::parseConditions(WeatherData &data, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("currentConditions"));
    data.temperature = qQNaN();
    data.dewpoint = qQNaN();
    data.condition = i18n("N/A");
    data.humidex.clear();
    data.stationID = i18n("N/A");
    data.stationLatitude = qQNaN();
    data.stationLongitude = qQNaN();
    data.pressure = qQNaN();
    data.visibility = qQNaN();
    data.humidity = qQNaN();

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("currentConditions"))
            break;

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("station")) {
                data.stationID = xml.attributes().value(QStringLiteral("code")).toString();
                QRegularExpression dumpDirection(QStringLiteral("[^0-9.]"));
                data.stationLatitude = xml.attributes().value(QStringLiteral("lat")).toString().remove(dumpDirection).toDouble();
                data.stationLongitude = xml.attributes().value(QStringLiteral("lon")).toString().remove(dumpDirection).toDouble();
            } else if (elementName == QLatin1String("dateTime")) {
                parseDateTime(data, xml);
            } else if (elementName == QLatin1String("condition")) {
                data.condition = xml.readElementText().trimmed();
            } else if (elementName == QLatin1String("temperature")) {
                // prevent N/A text to result in 0.0 value
                parseFloat(data.temperature, xml);
            } else if (elementName == QLatin1String("dewpoint")) {
                // prevent N/A text to result in 0.0 value
                parseFloat(data.dewpoint, xml);
            } else if (elementName == QLatin1String("humidex")) {
                data.humidex = xml.readElementText();
            } else if (elementName == QLatin1String("windChill")) {
                // prevent N/A text to result in 0.0 value
                parseFloat(data.windchill, xml);
            } else if (elementName == QLatin1String("pressure")) {
                data.pressureTendency = xml.attributes().value(QStringLiteral("tendency")).toString();
                if (data.pressureTendency.isEmpty()) {
                    data.pressureTendency = QStringLiteral("steady");
                }
                parseFloat(data.pressure, xml);
            } else if (elementName == QLatin1String("visibility")) {
                parseFloat(data.visibility, xml);
            } else if (elementName == QLatin1String("relativeHumidity")) {
                parseFloat(data.humidity, xml);
            } else if (elementName == QLatin1String("wind")) {
                parseWindInfo(data, xml);
            }
            //} else {
            //    parseUnknownElement(xml);
            //}
        }
    }
}

void EnvCanadaIon::parseWarnings(WeatherData &data, QXmlStreamReader &xml)
{
    WeatherData::WeatherEvent *watch = new WeatherData::WeatherEvent;
    WeatherData::WeatherEvent *warning = new WeatherData::WeatherEvent;

    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("warnings"));
    QString eventURL = xml.attributes().value(QStringLiteral("url")).toString();
    int flag = 0;

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("warnings")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("dateTime")) {
                if (flag == 1) {
                    parseDateTime(data, xml, watch);
                }
                if (flag == 2) {
                    parseDateTime(data, xml, warning);
                }

                if (!warning->timestamp.isEmpty() && !warning->url.isEmpty()) {
                    data.warnings.append(warning);
                    warning = new WeatherData::WeatherEvent;
                }
                if (!watch->timestamp.isEmpty() && !watch->url.isEmpty()) {
                    data.watches.append(watch);
                    watch = new WeatherData::WeatherEvent;
                }

            } else if (elementName == QLatin1String("event")) {
                // Append new event to list.
                QString eventType = xml.attributes().value(QStringLiteral("type")).toString();
                if (eventType == QLatin1String("watch")) {
                    watch->url = eventURL;
                    watch->type = eventType;
                    watch->priority = xml.attributes().value(QStringLiteral("priority")).toString();
                    watch->description = xml.attributes().value(QStringLiteral("description")).toString();
                    flag = 1;
                }

                if (eventType == QLatin1String("warning")) {
                    warning->url = eventURL;
                    warning->type = eventType;
                    warning->priority = xml.attributes().value(QStringLiteral("priority")).toString();
                    warning->description = xml.attributes().value(QStringLiteral("description")).toString();
                    flag = 2;
                }
            } else {
                if (xml.name() != QLatin1String("dateTime")) {
                    parseUnknownElement(xml);
                }
            }
        }
    }
    delete watch;
    delete warning;
}

void EnvCanadaIon::parseWeatherForecast(WeatherData &data, QXmlStreamReader &xml)
{
    WeatherData::ForecastInfo *forecast = new WeatherData::ForecastInfo;
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("forecastGroup"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("forecastGroup")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("dateTime")) {
                parseDateTime(data, xml);
            } else if (elementName == QLatin1String("regionalNormals")) {
                parseRegionalNormals(data, xml);
            } else if (elementName == QLatin1String("forecast")) {
                parseForecast(data, xml, forecast);
                forecast = new WeatherData::ForecastInfo;
            } else {
                parseUnknownElement(xml);
            }
        }
    }
    delete forecast;
}

void EnvCanadaIon::parseRegionalNormals(WeatherData &data, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("regionalNormals"));

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        const QStringRef elementName = xml.name();

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("textSummary")) {
                xml.readElementText();
            } else if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("high")) {
                // prevent N/A text to result in 0.0 value
                parseFloat(data.normalHigh, xml);
            } else if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("low")) {
                // prevent N/A text to result in 0.0 value
                parseFloat(data.normalLow, xml);
            }
        }
    }
}

void EnvCanadaIon::parseForecast(WeatherData &data, QXmlStreamReader &xml, WeatherData::ForecastInfo *forecast)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("forecast"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("forecast")) {
            data.forecasts.append(forecast);
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("period")) {
                forecast->forecastPeriod = xml.attributes().value(QStringLiteral("textForecastName")).toString();
            } else if (elementName == QLatin1String("textSummary")) {
                forecast->forecastSummary = xml.readElementText();
            } else if (elementName == QLatin1String("abbreviatedForecast")) {
                parseShortForecast(forecast, xml);
            } else if (elementName == QLatin1String("temperatures")) {
                parseForecastTemperatures(forecast, xml);
            } else if (elementName == QLatin1String("winds")) {
                parseWindForecast(forecast, xml);
            } else if (elementName == QLatin1String("precipitation")) {
                parsePrecipitationForecast(forecast, xml);
            } else if (elementName == QLatin1String("uv")) {
                data.UVRating = xml.attributes().value(QStringLiteral("category")).toString();
                parseUVIndex(data, xml);
                // else if (elementName == QLatin1String("frost")) { FIXME: Wait until winter to see what this looks like.
                //  parseFrost(xml, forecast);
            } else {
                if (elementName != QLatin1String("forecast")) {
                    parseUnknownElement(xml);
                }
            }
        }
    }
}

void EnvCanadaIon::parseShortForecast(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("abbreviatedForecast"));

    QString shortText;

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("abbreviatedForecast")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("pop")) {
                parseFloat(forecast->popPrecent, xml);
            }
            if (elementName == QLatin1String("textSummary")) {
                shortText = xml.readElementText();
                QMap<QString, ConditionIcons> forecastList = forecastIcons();
                if ((forecast->forecastPeriod == QLatin1String("tonight")) || (forecast->forecastPeriod.contains(QLatin1String("night")))) {
                    forecastList.insert(QStringLiteral("a few clouds"), FewCloudsNight);
                    forecastList.insert(QStringLiteral("cloudy periods"), PartlyCloudyNight);
                    forecastList.insert(QStringLiteral("chance of drizzle mixed with rain"), ChanceShowersNight);
                    forecastList.insert(QStringLiteral("chance of drizzle"), ChanceShowersNight);
                    forecastList.insert(QStringLiteral("chance of drizzle or rain"), ChanceShowersNight);
                    forecastList.insert(QStringLiteral("chance of flurries"), ChanceSnowNight);
                    forecastList.insert(QStringLiteral("chance of light snow"), ChanceSnowNight);
                    forecastList.insert(QStringLiteral("chance of flurries at times heavy"), ChanceSnowNight);
                    forecastList.insert(QStringLiteral("chance of showers or drizzle"), ChanceShowersNight);
                    forecastList.insert(QStringLiteral("chance of showers"), ChanceShowersNight);
                    forecastList.insert(QStringLiteral("clearing"), ClearNight);
                } else {
                    forecastList.insert(QStringLiteral("a few clouds"), FewCloudsDay);
                    forecastList.insert(QStringLiteral("cloudy periods"), PartlyCloudyDay);
                    forecastList.insert(QStringLiteral("chance of drizzle mixed with rain"), ChanceShowersDay);
                    forecastList.insert(QStringLiteral("chance of drizzle"), ChanceShowersDay);
                    forecastList.insert(QStringLiteral("chance of drizzle or rain"), ChanceShowersDay);
                    forecastList.insert(QStringLiteral("chance of flurries"), ChanceSnowDay);
                    forecastList.insert(QStringLiteral("chance of light snow"), ChanceSnowDay);
                    forecastList.insert(QStringLiteral("chance of flurries at times heavy"), ChanceSnowDay);
                    forecastList.insert(QStringLiteral("chance of showers or drizzle"), ChanceShowersDay);
                    forecastList.insert(QStringLiteral("chance of showers"), ChanceShowersDay);
                    forecastList.insert(QStringLiteral("clearing"), ClearDay);
                }
                forecast->shortForecast = shortText;
                forecast->iconName = getWeatherIcon(forecastList, shortText.toLower());
            }
        }
    }
}

void EnvCanadaIon::parseUVIndex(WeatherData &data, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("uv"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("uv")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("index")) {
                data.UVIndex = xml.readElementText();
            }
            if (elementName == QLatin1String("textSummary")) {
                xml.readElementText();
            }
        }
    }
}

void EnvCanadaIon::parseForecastTemperatures(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("temperatures"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("temperatures")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("low")) {
                parseFloat(forecast->tempLow, xml);
            } else if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("high")) {
                parseFloat(forecast->tempHigh, xml);
            } else if (elementName == QLatin1String("textSummary")) {
                xml.readElementText();
            }
        }
    }
}

void EnvCanadaIon::parsePrecipitationForecast(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("precipitation"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("precipitation")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("textSummary")) {
                forecast->precipForecast = xml.readElementText();
            } else if (elementName == QLatin1String("precipType")) {
                forecast->precipType = xml.readElementText();
            } else if (elementName == QLatin1String("accumulation")) {
                parsePrecipTotals(forecast, xml);
            }
        }
    }
}

void EnvCanadaIon::parsePrecipTotals(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("accumulation"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("accumulation")) {
            break;
        }

        if (elementName == QLatin1String("name")) {
            xml.readElementText();
        } else if (elementName == QLatin1String("amount")) {
            forecast->precipTotalExpected = xml.readElementText();
        }
    }
}

void EnvCanadaIon::parseWindForecast(WeatherData::ForecastInfo *forecast, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("winds"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("winds")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("textSummary")) {
                forecast->windForecast = xml.readElementText();
            } else {
                if (xml.name() != QLatin1String("winds")) {
                    parseUnknownElement(xml);
                }
            }
        }
    }
}

void EnvCanadaIon::parseYesterdayWeather(WeatherData &data, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("yesterdayConditions"));

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break;
        }

        const QStringRef elementName = xml.name();

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("high")) {
                parseFloat(data.prevHigh, xml);
            } else if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("low")) {
                parseFloat(data.prevLow, xml);
            } else if (elementName == QLatin1String("precip")) {
                data.prevPrecipType = xml.attributes().value(QStringLiteral("units")).toString();
                if (data.prevPrecipType.isEmpty()) {
                    data.prevPrecipType = QString::number(KUnitConversion::NoUnit);
                }
                data.prevPrecipTotal = xml.readElementText();
            }
        }
    }
}

void EnvCanadaIon::parseWeatherRecords(WeatherData &data, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("almanac"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("almanac")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("extremeMax")) {
                parseFloat(data.recordHigh, xml);
            } else if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("extremeMin")) {
                parseFloat(data.recordLow, xml);
            } else if (elementName == QLatin1String("precipitation") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("extremeRainfall")) {
                parseFloat(data.recordRain, xml);
            } else if (elementName == QLatin1String("precipitation") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("extremeSnowfall")) {
                parseFloat(data.recordSnow, xml);
            }
        }
    }
}

void EnvCanadaIon::parseAstronomicals(WeatherData &data, QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("riseSet"));

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("riseSet")) {
            break;
        }

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("disclaimer")) {
                xml.readElementText();
            } else if (elementName == QLatin1String("dateTime")) {
                parseDateTime(data, xml);
            }
        }
    }
}

// handle when no XML tag is found
void EnvCanadaIon::parseUnknownElement(QXmlStreamReader &xml) const
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

void EnvCanadaIon::updateWeather(const QString &source)
{
    // qCDebug(IONENGINE_ENVCAN) << "updateWeather()";

    const WeatherData &weatherData = m_weatherData[source];

    Plasma::DataEngine::Data data;

    data.insert(QStringLiteral("Country"), weatherData.countryName);
    data.insert(QStringLiteral("Place"), QVariant(weatherData.cityName + QStringLiteral(", ") + weatherData.shortTerritoryName));
    data.insert(QStringLiteral("Region"), weatherData.regionName);

    data.insert(QStringLiteral("Station"), weatherData.stationID.isEmpty() ? i18n("N/A") : weatherData.stationID.toUpper());

    const bool stationCoordValid = (!qIsNaN(weatherData.stationLatitude) && !qIsNaN(weatherData.stationLongitude));

    if (stationCoordValid) {
        data.insert(QStringLiteral("Latitude"), weatherData.stationLatitude);
        data.insert(QStringLiteral("Longitude"), weatherData.stationLongitude);
    }

    // Real weather - Current conditions
    if (weatherData.observationDateTime.isValid()) {
        data.insert(QStringLiteral("Observation Timestamp"), weatherData.observationDateTime);
    }

    data.insert(QStringLiteral("Observation Period"), weatherData.obsTimestamp);

    if (!weatherData.condition.isEmpty()) {
        data.insert(QStringLiteral("Current Conditions"), i18nc("weather condition", weatherData.condition.toUtf8().data()));
    }
    // qCDebug(IONENGINE_ENVCAN) << "i18n condition string: " << qPrintable(condition(source));

    QMap<QString, ConditionIcons> conditionList = conditionIcons();

    if (weatherData.isNight) {
        conditionList.insert(QStringLiteral("decreasing cloud"), FewCloudsNight);
        conditionList.insert(QStringLiteral("mostly cloudy"), PartlyCloudyNight);
        conditionList.insert(QStringLiteral("partly cloudy"), PartlyCloudyNight);
        conditionList.insert(QStringLiteral("fair"), FewCloudsNight);
    } else {
        conditionList.insert(QStringLiteral("decreasing cloud"), FewCloudsDay);
        conditionList.insert(QStringLiteral("mostly cloudy"), PartlyCloudyDay);
        conditionList.insert(QStringLiteral("partly cloudy"), PartlyCloudyDay);
        conditionList.insert(QStringLiteral("fair"), FewCloudsDay);
    }

    data.insert(QStringLiteral("Condition Icon"), getWeatherIcon(conditionList, weatherData.condition));

    if (!qIsNaN(weatherData.temperature)) {
        data.insert(QStringLiteral("Temperature"), weatherData.temperature);
    }
    if (!qIsNaN(weatherData.windchill)) {
        data.insert(QStringLiteral("Windchill"), weatherData.windchill);
    }
    if (!weatherData.humidex.isEmpty()) {
        data.insert(QStringLiteral("Humidex"), weatherData.humidex);
    }

    // Used for all temperatures
    data.insert(QStringLiteral("Temperature Unit"), KUnitConversion::Celsius);

    if (!qIsNaN(weatherData.dewpoint)) {
        data.insert(QStringLiteral("Dewpoint"), weatherData.dewpoint);
    }

    if (!qIsNaN(weatherData.pressure)) {
        data.insert(QStringLiteral("Pressure"), weatherData.pressure);
        data.insert(QStringLiteral("Pressure Unit"), KUnitConversion::Kilopascal);
        data.insert(QStringLiteral("Pressure Tendency"), weatherData.pressureTendency);
    }

    if (!qIsNaN(weatherData.visibility)) {
        data.insert(QStringLiteral("Visibility"), weatherData.visibility);
        data.insert(QStringLiteral("Visibility Unit"), KUnitConversion::Kilometer);
    }

    if (!qIsNaN(weatherData.humidity)) {
        data.insert(QStringLiteral("Humidity"), weatherData.humidity);
        data.insert(QStringLiteral("Humidity Unit"), KUnitConversion::Percent);
    }

    if (!qIsNaN(weatherData.windSpeed)) {
        data.insert(QStringLiteral("Wind Speed"), weatherData.windSpeed);
    }
    if (!qIsNaN(weatherData.windGust)) {
        data.insert(QStringLiteral("Wind Gust"), weatherData.windGust);
    }

    if (!qIsNaN(weatherData.windSpeed) || !qIsNaN(weatherData.windGust)) {
        data.insert(QStringLiteral("Wind Speed Unit"), KUnitConversion::KilometerPerHour);
    }

    if (!qIsNaN(weatherData.windSpeed) && static_cast<int>(weatherData.windSpeed) == 0) {
        data.insert(QStringLiteral("Wind Direction"), QStringLiteral("VR")); // Variable/calm
    } else if (!weatherData.windDirection.isEmpty()) {
        data.insert(QStringLiteral("Wind Direction"), weatherData.windDirection);
    }

    if (!qIsNaN(weatherData.normalHigh)) {
        data.insert(QStringLiteral("Normal High"), weatherData.normalHigh);
    }
    if (!qIsNaN(weatherData.normalLow)) {
        data.insert(QStringLiteral("Normal Low"), weatherData.normalLow);
    }

    // Check if UV index is available for the location
    if (!weatherData.UVIndex.isEmpty()) {
        data.insert(QStringLiteral("UV Index"), weatherData.UVIndex);
    }
    if (!weatherData.UVRating.isEmpty()) {
        data.insert(QStringLiteral("UV Rating"), weatherData.UVRating);
    }

    const QVector<WeatherData::WeatherEvent *> &watches = weatherData.watches;

    // Set number of forecasts per day/night supported
    data.insert(QStringLiteral("Total Watches Issued"), watches.size());

    // Check if we have warnings or watches
    for (int i = 0; i < watches.size(); ++i) {
        const WeatherData::WeatherEvent *watch = watches.at(i);
        const QString number = QString::number(i);

        data.insert(QStringLiteral("Watch Priority ") + number, watch->priority);
        data.insert(QStringLiteral("Watch Description ") + number, watch->description);
        data.insert(QStringLiteral("Watch Info ") + number, watch->url);
        data.insert(QStringLiteral("Watch Timestamp ") + number, watch->timestamp);
    }

    const QVector<WeatherData::WeatherEvent *> &warnings = weatherData.warnings;

    data.insert(QStringLiteral("Total Warnings Issued"), warnings.size());

    for (int k = 0; k < warnings.size(); ++k) {
        const WeatherData::WeatherEvent *warning = warnings.at(k);
        const QString number = QString::number(k);

        data.insert(QStringLiteral("Warning Priority ") + number, warning->priority);
        data.insert(QStringLiteral("Warning Description ") + number, warning->description);
        data.insert(QStringLiteral("Warning Info ") + number, warning->url);
        data.insert(QStringLiteral("Warning Timestamp ") + number, warning->timestamp);
    }

    const QVector<WeatherData::ForecastInfo *> &forecasts = weatherData.forecasts;

    // Set number of forecasts per day/night supported
    data.insert(QStringLiteral("Total Weather Days"), forecasts.size());

    int i = 0;
    for (const WeatherData::ForecastInfo *forecastInfo : forecasts) {
        QString forecastPeriod = forecastInfo->forecastPeriod;
        if (forecastPeriod.isEmpty()) {
            forecastPeriod = i18n("N/A");
        } else {
            // We need to shortform the day/night strings.

            forecastPeriod.replace(QStringLiteral("Today"), i18n("day"));
            forecastPeriod.replace(QStringLiteral("Tonight"), i18nc("Short for tonight", "nite"));
            forecastPeriod.replace(QStringLiteral("night"), i18nc("Short for night, appended to the end of the weekday", "nt"));
            forecastPeriod.replace(QStringLiteral("Saturday"), i18nc("Short for Saturday", "Sat"));
            forecastPeriod.replace(QStringLiteral("Sunday"), i18nc("Short for Sunday", "Sun"));
            forecastPeriod.replace(QStringLiteral("Monday"), i18nc("Short for Monday", "Mon"));
            forecastPeriod.replace(QStringLiteral("Tuesday"), i18nc("Short for Tuesday", "Tue"));
            forecastPeriod.replace(QStringLiteral("Wednesday"), i18nc("Short for Wednesday", "Wed"));
            forecastPeriod.replace(QStringLiteral("Thursday"), i18nc("Short for Thursday", "Thu"));
            forecastPeriod.replace(QStringLiteral("Friday"), i18nc("Short for Friday", "Fri"));
        }
        const QString shortForecast =
            forecastInfo->shortForecast.isEmpty() ? i18n("N/A") : i18nc("weather forecast", forecastInfo->shortForecast.toUtf8().data());

        const QString tempHigh = qIsNaN(forecastInfo->tempHigh) ? QString() : QString::number(forecastInfo->tempHigh);
        const QString tempLow = qIsNaN(forecastInfo->tempLow) ? QString() : QString::number(forecastInfo->tempLow);
        const QString popPrecent = qIsNaN(forecastInfo->popPrecent) ? QString() : QString::number(forecastInfo->popPrecent);

        data.insert(QStringLiteral("Short Forecast Day %1").arg(i),
                    QStringLiteral("%1|%2|%3|%4|%5|%6").arg(forecastPeriod, forecastInfo->iconName, shortForecast, tempHigh, tempLow, popPrecent));
        ++i;
    }

    // yesterday
    if (!qIsNaN(weatherData.prevHigh)) {
        data.insert(QStringLiteral("Yesterday High"), weatherData.prevHigh);
    }
    if (!qIsNaN(weatherData.prevLow)) {
        data.insert(QStringLiteral("Yesterday Low"), weatherData.prevLow);
    }

    const QString &prevPrecipTotal = weatherData.prevPrecipTotal;
    if (prevPrecipTotal == QLatin1String("Trace")) {
        data.insert(QStringLiteral("Yesterday Precip Total"), i18nc("precipitation total, very little", "Trace"));
    } else if (!prevPrecipTotal.isEmpty()) {
        data.insert(QStringLiteral("Yesterday Precip Total"), prevPrecipTotal);
        const QString& prevPrecipType = weatherData.prevPrecipType;
        // clang-format off
        const KUnitConversion::UnitId unit =
            (prevPrecipType == QLatin1String("mm")) ? KUnitConversion::Millimeter :
            (prevPrecipType == QLatin1String("cm")) ? KUnitConversion::Centimeter :
            /*else*/                                  KUnitConversion::NoUnit;
        // clang-format on
        data.insert(QStringLiteral("Yesterday Precip Unit"), unit);
    }

    // records
    if (!qIsNaN(weatherData.recordHigh)) {
        data.insert(QStringLiteral("Record High Temperature"), weatherData.recordHigh);
    }
    if (!qIsNaN(weatherData.recordLow)) {
        data.insert(QStringLiteral("Record Low Temperature"), weatherData.recordLow);
    }
    if (!qIsNaN(weatherData.recordRain)) {
        data.insert(QStringLiteral("Record Rainfall"), weatherData.recordRain);
        data.insert(QStringLiteral("Record Rainfall Unit"), KUnitConversion::Millimeter);
    }
    if (!qIsNaN(weatherData.recordSnow)) {
        data.insert(QStringLiteral("Record Snowfall"), weatherData.recordSnow);
        data.insert(QStringLiteral("Record Snowfall Unit"), KUnitConversion::Centimeter);
    }

    data.insert(QStringLiteral("Credit"), i18nc("credit line, keep string short", "Data from Environment and Climate Change\302\240Canada"));
    data.insert(QStringLiteral("Credit Url"), weatherData.creditUrl);
    setData(source, data);
}

void EnvCanadaIon::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    const bool isNight = (data.value(QStringLiteral("Corrected Elevation")).toDouble() < 0.0);

    for (auto end = m_weatherData.end(), it = m_weatherData.begin(); it != end; ++it) {
        auto &weatherData = it.value();
        if (weatherData.solarDataTimeEngineSourceName == sourceName) {
            weatherData.isNight = isNight;
            updateWeather(it.key());
        }
    }
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(envcan, EnvCanadaIon, "ion-envcan.json")

#include "ion_envcan.moc"
