/***************************************************************************
 *   Copyright (C) 2007-2011 by Shawn Starr <shawn.starr@rogers.com>       *
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

#include <KIO/Job>
#include <KUnitConversion/Converter>
#include <KLocalizedString>


// ctor, dtor
EnvCanadaIon::EnvCanadaIon(QObject *parent, const QVariantList &args)
        : IonInterface(parent, args)
{
    // Get the real city XML URL so we can parse this
    getXMLSetup();
    // not used while daytime not considered, see below
    // m_timeEngine = dataEngine("time");
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
    QMap<QString, ConditionIcons> conditionList;

    // Explicit periods
    conditionList.insert(QStringLiteral("mainly sunny"), FewCloudsDay);
    conditionList.insert(QStringLiteral("mainly clear"), FewCloudsNight);
    conditionList.insert(QStringLiteral("sunny"), ClearDay);
    conditionList.insert(QStringLiteral("clear"), ClearNight);

    // Available conditions
    conditionList.insert(QStringLiteral("blowing snow"), Snow);
    conditionList.insert(QStringLiteral("cloudy"), Overcast);
    conditionList.insert(QStringLiteral("distant precipitation"), LightRain);
    conditionList.insert(QStringLiteral("drifting snow"), Flurries);
    conditionList.insert(QStringLiteral("drizzle"), LightRain);
    conditionList.insert(QStringLiteral("dust"), NotAvailable);
    conditionList.insert(QStringLiteral("dust devils"), NotAvailable);
    conditionList.insert(QStringLiteral("fog"), Mist);
    conditionList.insert(QStringLiteral("fog bank near station"), Mist);
    conditionList.insert(QStringLiteral("fog depositing ice"), Mist);
    conditionList.insert(QStringLiteral("fog patches"), Mist);
    conditionList.insert(QStringLiteral("freezing drizzle"), FreezingDrizzle);
    conditionList.insert(QStringLiteral("freezing rain"), FreezingRain);
    conditionList.insert(QStringLiteral("funnel cloud"), NotAvailable);
    conditionList.insert(QStringLiteral("hail"), Hail);
    conditionList.insert(QStringLiteral("haze"), Haze);
    conditionList.insert(QStringLiteral("heavy blowing snow"), Snow);
    conditionList.insert(QStringLiteral("heavy drifting snow"), Snow);
    conditionList.insert(QStringLiteral("heavy drizzle"), LightRain);
    conditionList.insert(QStringLiteral("heavy hail"), Hail);
    conditionList.insert(QStringLiteral("heavy mixed rain and drizzle"), LightRain);
    conditionList.insert(QStringLiteral("heavy mixed rain and snow shower"), RainSnow);
    conditionList.insert(QStringLiteral("heavy rain"), Rain);
    conditionList.insert(QStringLiteral("heavy rain and snow"), RainSnow);
    conditionList.insert(QStringLiteral("heavy rainshower"), Rain);
    conditionList.insert(QStringLiteral("heavy snow"), Snow);
    conditionList.insert(QStringLiteral("heavy snow pellets"), Snow);
    conditionList.insert(QStringLiteral("heavy snowshower"), Snow);
    conditionList.insert(QStringLiteral("heavy thunderstorm with hail"), Thunderstorm);
    conditionList.insert(QStringLiteral("heavy thunderstorm with rain"), Thunderstorm);
    conditionList.insert(QStringLiteral("ice crystals"), Flurries);
    conditionList.insert(QStringLiteral("ice pellets"), Hail);
    conditionList.insert(QStringLiteral("increasing cloud"), Overcast);
    conditionList.insert(QStringLiteral("light drizzle"), LightRain);
    conditionList.insert(QStringLiteral("light freezing drizzle"), FreezingRain);
    conditionList.insert(QStringLiteral("light freezing rain"), FreezingRain);
    conditionList.insert(QStringLiteral("light rain"), LightRain);
    conditionList.insert(QStringLiteral("light rainshower"), LightRain);
    conditionList.insert(QStringLiteral("light snow"), LightSnow);
    conditionList.insert(QStringLiteral("light snow pellets"), LightSnow);
    conditionList.insert(QStringLiteral("light snowshower"), Flurries);
    conditionList.insert(QStringLiteral("lightning visible"), Thunderstorm);
    conditionList.insert(QStringLiteral("mist"), Mist);
    conditionList.insert(QStringLiteral("mixed rain and drizzle"), LightRain);
    conditionList.insert(QStringLiteral("mixed rain and snow shower"), RainSnow);
    conditionList.insert(QStringLiteral("not reported"), NotAvailable);
    conditionList.insert(QStringLiteral("rain"), Rain);
    conditionList.insert(QStringLiteral("rain and snow"), RainSnow);
    conditionList.insert(QStringLiteral("rainshower"), LightRain);
    conditionList.insert(QStringLiteral("recent drizzle"), LightRain);
    conditionList.insert(QStringLiteral("recent dust or sand storm"), NotAvailable);
    conditionList.insert(QStringLiteral("recent fog"), Mist);
    conditionList.insert(QStringLiteral("recent freezing precipitation"), FreezingDrizzle);
    conditionList.insert(QStringLiteral("recent hail"), Hail);
    conditionList.insert(QStringLiteral("recent rain"), Rain);
    conditionList.insert(QStringLiteral("recent rain and snow"), RainSnow);
    conditionList.insert(QStringLiteral("recent rainshower"), Rain);
    conditionList.insert(QStringLiteral("recent snow"), Snow);
    conditionList.insert(QStringLiteral("recent snowshower"), Flurries);
    conditionList.insert(QStringLiteral("recent thunderstorm"), Thunderstorm);
    conditionList.insert(QStringLiteral("recent thunderstorm with hail"), Thunderstorm);
    conditionList.insert(QStringLiteral("recent thunderstorm with heavy hail"), Thunderstorm);
    conditionList.insert(QStringLiteral("recent thunderstorm with heavy rain"), Thunderstorm);
    conditionList.insert(QStringLiteral("recent thunderstorm with rain"), Thunderstorm);
    conditionList.insert(QStringLiteral("sand or dust storm"), NotAvailable);
    conditionList.insert(QStringLiteral("severe sand or dust storm"), NotAvailable);
    conditionList.insert(QStringLiteral("shallow fog"), Mist);
    conditionList.insert(QStringLiteral("smoke"), NotAvailable);
    conditionList.insert(QStringLiteral("snow"), Snow);
    conditionList.insert(QStringLiteral("snow crystals"), Flurries);
    conditionList.insert(QStringLiteral("snow grains"), Flurries);
    conditionList.insert(QStringLiteral("squalls"), Snow);
    conditionList.insert(QStringLiteral("thunderstorm with hail"), Thunderstorm);
    conditionList.insert(QStringLiteral("thunderstorm with rain"), Thunderstorm);
    conditionList.insert(QStringLiteral("thunderstorm with sand or dust storm"), Thunderstorm);
    conditionList.insert(QStringLiteral("thunderstorm without precipitation"), Thunderstorm);
    conditionList.insert(QStringLiteral("tornado"), NotAvailable);

    return conditionList;
}


QMap<QString, IonInterface::ConditionIcons> EnvCanadaIon::setupForecastIconMappings() const
{
    QMap<QString, ConditionIcons> forecastList;

    // Abbreviated forecast descriptions
    forecastList.insert(QStringLiteral("a few flurries"), Flurries);
    forecastList.insert(QStringLiteral("a few flurries mixed with ice pellets"), RainSnow);
    forecastList.insert(QStringLiteral("a few flurries or rain showers"), RainSnow);
    forecastList.insert(QStringLiteral("a few flurries or thundershowers"), RainSnow);
    forecastList.insert(QStringLiteral("a few rain showers or flurries"), RainSnow);
    forecastList.insert(QStringLiteral("a few rain showers or wet flurries"), RainSnow);
    forecastList.insert(QStringLiteral("a few showers"), LightRain);
    forecastList.insert(QStringLiteral("a few showers or drizzle"), LightRain);
    forecastList.insert(QStringLiteral("a few showers or thundershowers"), Thunderstorm);
    forecastList.insert(QStringLiteral("a few showers or thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("a few thundershowers"), Thunderstorm);
    forecastList.insert(QStringLiteral("a few thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("a few wet flurries"), RainSnow);
    forecastList.insert(QStringLiteral("a few wet flurries or rain showers"), RainSnow);
    forecastList.insert(QStringLiteral("a mix of sun and cloud"), PartlyCloudyDay);
    forecastList.insert(QStringLiteral("cloudy with sunny periods"), PartlyCloudyDay);
    forecastList.insert(QStringLiteral("partly cloudy"), PartlyCloudyDay);
    forecastList.insert(QStringLiteral("mainly sunny"), FewCloudsDay);
    forecastList.insert(QStringLiteral("sunny"), ClearDay);
    forecastList.insert(QStringLiteral("blizzard"), Snow);
    forecastList.insert(QStringLiteral("clear"), ClearNight);
    forecastList.insert(QStringLiteral("cloudy"), Overcast);
    forecastList.insert(QStringLiteral("drizzle"), LightRain);
    forecastList.insert(QStringLiteral("drizzle mixed with freezing drizzle"), FreezingDrizzle);
    forecastList.insert(QStringLiteral("drizzle mixed with rain"), LightRain);
    forecastList.insert(QStringLiteral("drizzle or freezing drizzle"), LightRain);
    forecastList.insert(QStringLiteral("drizzle or rain"), LightRain);
    forecastList.insert(QStringLiteral("flurries"), Flurries);
    forecastList.insert(QStringLiteral("flurries at times heavy"), Flurries);
    forecastList.insert(QStringLiteral("flurries at times heavy or rain snowers"), RainSnow);
    forecastList.insert(QStringLiteral("flurries mixed with ice pellets"), FreezingRain);
    forecastList.insert(QStringLiteral("flurries or ice pellets"), FreezingRain);
    forecastList.insert(QStringLiteral("flurries or rain showers"), RainSnow);
    forecastList.insert(QStringLiteral("flurries or thundershowers"), Flurries);
    forecastList.insert(QStringLiteral("fog"), Mist);
    forecastList.insert(QStringLiteral("fog developing"), Mist);
    forecastList.insert(QStringLiteral("fog dissipating"), Mist);
    forecastList.insert(QStringLiteral("fog patches"), Mist);
    forecastList.insert(QStringLiteral("freezing drizzle"), FreezingDrizzle);
    forecastList.insert(QStringLiteral("freezing rain"), FreezingRain);
    forecastList.insert(QStringLiteral("freezing rain mixed with rain"), FreezingRain);
    forecastList.insert(QStringLiteral("freezing rain mixed with snow"), FreezingRain);
    forecastList.insert(QStringLiteral("freezing rain or ice pellets"), FreezingRain);
    forecastList.insert(QStringLiteral("freezing rain or rain"), FreezingRain);
    forecastList.insert(QStringLiteral("freezing rain or snow"), FreezingRain);
    forecastList.insert(QStringLiteral("ice fog"), Mist);
    forecastList.insert(QStringLiteral("ice fog developing"), Mist);
    forecastList.insert(QStringLiteral("ice fog dissipating"), Mist);
    forecastList.insert(QStringLiteral("ice pellet"), Hail);
    forecastList.insert(QStringLiteral("ice pellet mixed with freezing rain"), Hail);
    forecastList.insert(QStringLiteral("ice pellet mixed with snow"), Hail);
    forecastList.insert(QStringLiteral("ice pellet or snow"), RainSnow);
    forecastList.insert(QStringLiteral("light snow"), LightSnow);
    forecastList.insert(QStringLiteral("light snow and blizzard"), LightSnow);
    forecastList.insert(QStringLiteral("light snow and blizzard and blowing snow"), Snow);
    forecastList.insert(QStringLiteral("light snow and blowing snow"), LightSnow);
    forecastList.insert(QStringLiteral("light snow mixed with freezing drizzle"), FreezingDrizzle);
    forecastList.insert(QStringLiteral("light snow mixed with freezing rain"), FreezingRain);
    forecastList.insert(QStringLiteral("light snow or ice pellets"), LightSnow);
    forecastList.insert(QStringLiteral("light snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("light wet snow"), RainSnow);
    forecastList.insert(QStringLiteral("light wet snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("local snow squalls"), Snow);
    forecastList.insert(QStringLiteral("near blizzard"), Snow);
    forecastList.insert(QStringLiteral("overcast"), Overcast);
    forecastList.insert(QStringLiteral("increasing cloudiness"), Overcast);
    forecastList.insert(QStringLiteral("increasing clouds"), Overcast);
    forecastList.insert(QStringLiteral("periods of drizzle"), LightRain);
    forecastList.insert(QStringLiteral("periods of drizzle mixed with freezing drizzle"), FreezingDrizzle);
    forecastList.insert(QStringLiteral("periods of drizzle mixed with rain"), LightRain);
    forecastList.insert(QStringLiteral("periods of drizzle or freezing drizzle"), FreezingDrizzle);
    forecastList.insert(QStringLiteral("periods of drizzle or rain"), LightRain);
    forecastList.insert(QStringLiteral("periods of freezing drizzle"), FreezingDrizzle);
    forecastList.insert(QStringLiteral("periods of freezing drizzle or drizzle"), FreezingDrizzle);
    forecastList.insert(QStringLiteral("periods of freezing drizzle or rain"), FreezingDrizzle);
    forecastList.insert(QStringLiteral("periods of freezing rain"), FreezingRain);
    forecastList.insert(QStringLiteral("periods of freezing rain mixed with ice pellets"), FreezingRain);
    forecastList.insert(QStringLiteral("periods of freezing rain mixed with rain"), FreezingRain);
    forecastList.insert(QStringLiteral("periods of freezing rain mixed with snow"), FreezingRain);
    forecastList.insert(QStringLiteral("periods of freezing rain mixed with freezing drizzle"), FreezingRain);
    forecastList.insert(QStringLiteral("periods of freezing rain or ice pellets"), FreezingRain);
    forecastList.insert(QStringLiteral("periods of freezing rain or rain"), FreezingRain);
    forecastList.insert(QStringLiteral("periods of freezing rain or snow"), FreezingRain);
    forecastList.insert(QStringLiteral("periods of ice pellet"), Hail);
    forecastList.insert(QStringLiteral("periods of ice pellet mixed with freezing rain"), Hail);
    forecastList.insert(QStringLiteral("periods of ice pellet mixed with snow"), Hail);
    forecastList.insert(QStringLiteral("periods of ice pellet or freezing rain"), Hail);
    forecastList.insert(QStringLiteral("periods of ice pellet or snow"), Hail);
    forecastList.insert(QStringLiteral("periods of light snow"), LightSnow);
    forecastList.insert(QStringLiteral("periods of light snow and blizzard"), Snow);
    forecastList.insert(QStringLiteral("periods of light snow and blizzard and blowing snow"), Snow);
    forecastList.insert(QStringLiteral("periods of light snow and blowing snow"), LightSnow);
    forecastList.insert(QStringLiteral("periods of light snow mixed with freezing drizzle"), RainSnow);
    forecastList.insert(QStringLiteral("periods of light snow mixed with freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of light snow mixed with ice pelletS"), LightSnow);
    forecastList.insert(QStringLiteral("periods of light snow mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of light snow or freezing drizzle"), RainSnow);
    forecastList.insert(QStringLiteral("periods of light snow or freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of light snow or ice pellets"), LightSnow);
    forecastList.insert(QStringLiteral("periods of light snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of light wet snow"), LightSnow);
    forecastList.insert(QStringLiteral("periods of light wet snow mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of light wet snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of rain"), Rain);
    forecastList.insert(QStringLiteral("periods of rain mixed with freezing rain"), Rain);
    forecastList.insert(QStringLiteral("periods of rain mixed with snow"), RainSnow);
    forecastList.insert(QStringLiteral("periods of rain or drizzle"), Rain);
    forecastList.insert(QStringLiteral("periods of rain or freezing rain"), Rain);
    forecastList.insert(QStringLiteral("periods of rain or thundershowers"), Showers);
    forecastList.insert(QStringLiteral("periods of rain or thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("periods of rain or snow"), RainSnow);
    forecastList.insert(QStringLiteral("periods of snow"), Snow);
    forecastList.insert(QStringLiteral("periods of snow and blizzard"), Snow);
    forecastList.insert(QStringLiteral("periods of snow and blizzard and blowing snow"), Snow);
    forecastList.insert(QStringLiteral("periods of snow and blowing snow"), Snow);
    forecastList.insert(QStringLiteral("periods of snow mixed with freezing drizzle"), RainSnow);
    forecastList.insert(QStringLiteral("periods of snow mixed with freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of snow mixed with ice pellets"), Snow);
    forecastList.insert(QStringLiteral("periods of snow mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of snow or freezing drizzle"), RainSnow);
    forecastList.insert(QStringLiteral("periods of snow or freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of snow or ice pellets"), Snow);
    forecastList.insert(QStringLiteral("periods of snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of rain or snow"), RainSnow);
    forecastList.insert(QStringLiteral("periods of wet snow"), Snow);
    forecastList.insert(QStringLiteral("periods of wet snow mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("periods of wet snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("rain"), Rain);
    forecastList.insert(QStringLiteral("rain at times heavy"), Rain);
    forecastList.insert(QStringLiteral("rain at times heavy mixed with freezing rain"), FreezingRain);
    forecastList.insert(QStringLiteral("rain at times heavy mixed with snow"), RainSnow);
    forecastList.insert(QStringLiteral("rain at times heavy or drizzle"), Rain);
    forecastList.insert(QStringLiteral("rain at times heavy or freezing rain"), Rain);
    forecastList.insert(QStringLiteral("rain at times heavy or snow"), RainSnow);
    forecastList.insert(QStringLiteral("rain at times heavy or thundershowers"), Showers);
    forecastList.insert(QStringLiteral("rain at times heavy or thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("rain mixed with freezing rain"), FreezingRain);
    forecastList.insert(QStringLiteral("rain mixed with snow"), RainSnow);
    forecastList.insert(QStringLiteral("rain or drizzle"), Rain);
    forecastList.insert(QStringLiteral("rain or freezing rain"), Rain);
    forecastList.insert(QStringLiteral("rain or snow"), RainSnow);
    forecastList.insert(QStringLiteral("rain or thundershowers"), Showers);
    forecastList.insert(QStringLiteral("rain or thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("rain showers or flurries"), RainSnow);
    forecastList.insert(QStringLiteral("rain showers or wet flurries"), RainSnow);
    forecastList.insert(QStringLiteral("showers"), Showers);
    forecastList.insert(QStringLiteral("showers at times heavy"), Showers);
    forecastList.insert(QStringLiteral("showers at times heavy or thundershowers"), Showers);
    forecastList.insert(QStringLiteral("showers at times heavy or thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("showers or drizzle"), Showers);
    forecastList.insert(QStringLiteral("showers or thundershowers"), Thunderstorm);
    forecastList.insert(QStringLiteral("showers or thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("smoke"), NotAvailable);
    forecastList.insert(QStringLiteral("snow"), Snow);
    forecastList.insert(QStringLiteral("snow and blizzard"), Snow);
    forecastList.insert(QStringLiteral("snow and blizzard and blowing snow"), Snow);
    forecastList.insert(QStringLiteral("snow and blowing snow"), Snow);
    forecastList.insert(QStringLiteral("snow at times heavy"), Snow);
    forecastList.insert(QStringLiteral("snow at times heavy and blizzard"), Snow);
    forecastList.insert(QStringLiteral("snow at times heavy and blowing snow"), Snow);
    forecastList.insert(QStringLiteral("snow at times heavy mixed with freezing drizzle"), RainSnow);
    forecastList.insert(QStringLiteral("snow at times heavy mixed with freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("snow at times heavy mixed with ice pellets"), Snow);
    forecastList.insert(QStringLiteral("snow at times heavy mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("snow at times heavy or freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("snow at times heavy or ice pellets"), Snow);
    forecastList.insert(QStringLiteral("snow at times heavy or rain"), RainSnow);
    forecastList.insert(QStringLiteral("snow mixed with freezing drizzle"), RainSnow);
    forecastList.insert(QStringLiteral("snow mixed with freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("snow mixed with ice pellets"), Snow);
    forecastList.insert(QStringLiteral("snow mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("snow or freezing drizzle"), RainSnow);
    forecastList.insert(QStringLiteral("snow or freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("snow or ice pellets"), Snow);
    forecastList.insert(QStringLiteral("snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("snow squalls"), Snow);
    forecastList.insert(QStringLiteral("sunny"), ClearDay);
    forecastList.insert(QStringLiteral("sunny with cloudy periods"), PartlyCloudyDay);
    forecastList.insert(QStringLiteral("thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("thunderstorms and possible hail"), Thunderstorm);
    forecastList.insert(QStringLiteral("wet flurries"), Flurries);
    forecastList.insert(QStringLiteral("wet flurries at times heavy"), Flurries);
    forecastList.insert(QStringLiteral("wet flurries at times heavy or rain snowers"), RainSnow);
    forecastList.insert(QStringLiteral("wet flurries or rain showers"), RainSnow);
    forecastList.insert(QStringLiteral("wet snow"), Snow);
    forecastList.insert(QStringLiteral("wet snow at times heavy"), Snow);
    forecastList.insert(QStringLiteral("wet snow at times heavy mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("wet snow mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("wet snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("windy"), NotAvailable);

    forecastList.insert(QStringLiteral("chance of drizzle mixed with freezing drizzle"), LightRain);
    forecastList.insert(QStringLiteral("chance of flurries mixed with ice pellets"), Flurries);
    forecastList.insert(QStringLiteral("chance of flurries or ice pellets"), Flurries);
    forecastList.insert(QStringLiteral("chance of flurries or rain showers"), RainSnow);
    forecastList.insert(QStringLiteral("chance of flurries or thundershowers"), RainSnow);
    forecastList.insert(QStringLiteral("chance of freezing drizzle"), FreezingDrizzle);
    forecastList.insert(QStringLiteral("chance of freezing rain"), FreezingRain);
    forecastList.insert(QStringLiteral("chance of freezing rain mixed with snow"), RainSnow);
    forecastList.insert(QStringLiteral("chance of freezing rain or rain"), FreezingRain);
    forecastList.insert(QStringLiteral("chance of freezing rain or snow"), RainSnow);
    forecastList.insert(QStringLiteral("chance of light snow and blowing snow"), LightSnow);
    forecastList.insert(QStringLiteral("chance of light snow mixed with freezing drizzle"), LightSnow);
    forecastList.insert(QStringLiteral("chance of light snow mixed with ice pellets"), LightSnow);
    forecastList.insert(QStringLiteral("chance of light snow mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("chance of light snow or freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("chance of light snow or ice pellets"), LightSnow);
    forecastList.insert(QStringLiteral("chance of light snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("chance of light wet snow"), Snow);
    forecastList.insert(QStringLiteral("chance of rain"), Rain);
    forecastList.insert(QStringLiteral("chance of rain at times heavy"), Rain);
    forecastList.insert(QStringLiteral("chance of rain mixed with snow"), RainSnow);
    forecastList.insert(QStringLiteral("chance of rain or drizzle"), Rain);
    forecastList.insert(QStringLiteral("chance of rain or freezing rain"), Rain);
    forecastList.insert(QStringLiteral("chance of rain or snow"), RainSnow);
    forecastList.insert(QStringLiteral("chance of rain showers or flurries"), RainSnow);
    forecastList.insert(QStringLiteral("chance of rain showers or wet flurries"), RainSnow);
    forecastList.insert(QStringLiteral("chance of severe thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("chance of showers at times heavy"), Rain);
    forecastList.insert(QStringLiteral("chance of showers at times heavy or thundershowers"), Thunderstorm);
    forecastList.insert(QStringLiteral("chance of showers at times heavy or thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("chance of showers or thundershowers"), Thunderstorm);
    forecastList.insert(QStringLiteral("chance of showers or thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("chance of snow"), Snow);
    forecastList.insert(QStringLiteral("chance of snow and blizzard"), Snow);
    forecastList.insert(QStringLiteral("chance of snow mixed with freezing drizzle"), Snow);
    forecastList.insert(QStringLiteral("chance of snow mixed with freezing rain"), RainSnow);
    forecastList.insert(QStringLiteral("chance of snow mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("chance of snow or rain"), RainSnow);
    forecastList.insert(QStringLiteral("chance of snow squalls"), Snow);
    forecastList.insert(QStringLiteral("chance of thundershowers"), Showers);
    forecastList.insert(QStringLiteral("chance of thunderstorms"), Thunderstorm);
    forecastList.insert(QStringLiteral("chance of thunderstorms and possible hail"), Thunderstorm);
    forecastList.insert(QStringLiteral("chance of wet flurries"), Flurries);
    forecastList.insert(QStringLiteral("chance of wet flurries at times heavy"), Flurries);
    forecastList.insert(QStringLiteral("chance of wet flurries or rain showers"), RainSnow);
    forecastList.insert(QStringLiteral("chance of wet snow"), Snow);
    forecastList.insert(QStringLiteral("chance of wet snow mixed with rain"), RainSnow);
    forecastList.insert(QStringLiteral("chance of wet snow or rain"), RainSnow);

    return forecastList;
}

QMap<QString, IonInterface::ConditionIcons> const& EnvCanadaIon::conditionIcons() const
{
    static QMap<QString, ConditionIcons> const condval = setupConditionIconMappings();
    return condval;
}

QMap<QString, IonInterface::ConditionIcons> const& EnvCanadaIon::forecastIcons() const
{
    static QMap<QString, ConditionIcons> const foreval = setupForecastIconMappings();
    return foreval;
}

QStringList EnvCanadaIon::validate(const QString& source) const
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
bool EnvCanadaIon::updateIonSource(const QString& source)
{
    //qDebug() << "updateIonSource()" << source;

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

        const QString reply =
            (result.size() == 1) ? QStringLiteral("envcan|valid|single|") + result[0] :
            (result.size() > 1) ?  QStringLiteral("envcan|valid|multiple|") + result.join(QLatin1Char('|')) :
            /*else*/               QStringLiteral("envcan|invalid|single|") + sourceAction[2];
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
    //qDebug() << "getXMLSetup()";

    // If network is down, we need to spin and wait

    const QUrl url(QStringLiteral("http://dd.weatheroffice.ec.gc.ca/citypage_weather/xml/siteList.xml"));

    KIO::TransferJob* getJob = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);

    m_xmlSetup.clear();
    connect(getJob, &KIO::TransferJob::data,
            this, &EnvCanadaIon::setup_slotDataArrived);
    connect(getJob, &KJob::result,
            this, &EnvCanadaIon::setup_slotJobFinished);
}

// Gets specific city XML data
void EnvCanadaIon::getXMLData(const QString& source)
{
    foreach (const QString& fetching, m_jobList) {
        if (fetching == source) {
            // already getting this source and awaiting the data
            return;
        }
    }

    //qDebug() << source;

    // Demunge source name for key only.
    QString dataKey = source;
    dataKey.remove(QStringLiteral("envcan|weather|"));

    const QUrl url(QLatin1String("http://dd.weatheroffice.ec.gc.ca/citypage_weather/xml/") + m_places[dataKey].territoryName + QLatin1Char('/') + m_places[dataKey].cityCode + QStringLiteral("_e.xml"));
    //url="file:///home/spstarr/Desktop/s0000649_e.xml";
    //qDebug() << "Will Try URL: " << url;

    if (m_places[dataKey].territoryName.isEmpty() && m_places[dataKey].cityCode.isEmpty()) {
        setData(source, QStringLiteral("validate"), QStringLiteral("envcan|malformed"));
        return;
    }

    KIO::TransferJob* getJob  = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);

    m_jobXml.insert(getJob, new QXmlStreamReader);
    m_jobList.insert(getJob, source);

    connect(getJob, &KIO::TransferJob::data,
            this, &EnvCanadaIon::slotDataArrived);
    connect(getJob, &KJob::result,
            this, &EnvCanadaIon::slotJobFinished);
}

void EnvCanadaIon::setup_slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)

    if (data.isEmpty()) {
        //qDebug() << "done!";
        return;
    }

    // Send to xml.
    //qDebug() << data;
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
    //qDebug() << source << m_sourcesToReset.contains(source);
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
    //qDebug() << success << m_sourcesToReset;
    setInitialized(success);
}

// Parse the city list and store into a QMap
bool EnvCanadaIon::readXMLSetup()
{
    bool success = false;
    QString territory;
    QString code;
    QString cityName;

    //qDebug() << "readXMLSetup()";

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

void EnvCanadaIon::parseWeatherSite(WeatherData& data, QXmlStreamReader& xml)
{
    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("license")) {
                xml.readElementText();
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

// Parse Weather data main loop, from here we have to decend into each tag pair
bool EnvCanadaIon::readXMLData(const QString& source, QXmlStreamReader& xml)
{
    WeatherData data;
    data.recordHigh = 0.0;
    data.recordLow = 0.0;

    //qDebug() << "readXMLData()";

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

    m_weatherData[source] = data;
    updateWeather(source);
    return !xml.error();
}

void EnvCanadaIon::parseDateTime(WeatherData& data, QXmlStreamReader& xml, WeatherData::WeatherEvent *event)
{

    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("dateTime"));

    // What kind of date info is this?
    const QString dateType = xml.attributes().value(QStringLiteral("name")).toString();
    const QString dateZone = xml.attributes().value(QStringLiteral("zone")).toString();

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
                    m_dateFormat = QDateTime::fromString(selectTimeStamp, QStringLiteral("yyyyMMddHHmmss"));
                    data.obsTimestamp = m_dateFormat.toString(QStringLiteral("dd.MM.yyyy @ hh:mm"));
                    data.iconPeriodHour = m_dateFormat.toString(QStringLiteral("hh")).toInt();
                    data.iconPeriodMinute = m_dateFormat.toString(QStringLiteral("mm")).toInt();
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

void EnvCanadaIon::parseLocations(WeatherData& data, QXmlStreamReader& xml)
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

void EnvCanadaIon::parseWindInfo(WeatherData& data, QXmlStreamReader& xml)
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
                data.windSpeed = xml.readElementText();
            } else if (elementName == QLatin1String("gust")) {
                data.windGust = xml.readElementText();
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

void EnvCanadaIon::parseConditions(WeatherData& data, QXmlStreamReader& xml)
{

    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("currentConditions"));
    data.temperature = i18n("N/A");
    data.dewpoint = i18n("N/A");
    data.condition = i18n("N/A");
    data.comforttemp = i18n("N/A");
    data.stationID = i18n("N/A");
    data.stationLat = i18n("N/A");
    data.stationLon = i18n("N/A");
    data.pressure = 0.0;
    data.pressureTendency = i18n("N/A");
    data.visibility = 0;
    data.humidity = i18n("N/A");

    while (!xml.atEnd()) {
        xml.readNext();

        const QStringRef elementName = xml.name();

        if (xml.isEndElement() && elementName == QLatin1String("currentConditions"))
            break;

        if (xml.isStartElement()) {
            if (elementName == QLatin1String("station")) {
                data.stationID = xml.attributes().value(QStringLiteral("code")).toString();
                data.stationLat = xml.attributes().value(QStringLiteral("lat")).toString();
                data.stationLon = xml.attributes().value(QStringLiteral("lon")).toString();
            } else if (elementName == QLatin1String("dateTime")) {
                parseDateTime(data, xml);
            } else if (elementName == QLatin1String("condition")) {
                data.condition = xml.readElementText();
            } else if (elementName == QLatin1String("temperature")) {
                data.temperature = xml.readElementText();
            } else if (elementName == QLatin1String("dewpoint")) {
                data.dewpoint = xml.readElementText();
            } else if (elementName == QLatin1String("humidex") || elementName == QLatin1String("windChill")) {
                data.comforttemp = xml.readElementText();
            } else if (elementName == QLatin1String("pressure")) {
                data.pressureTendency = xml.attributes().value(QStringLiteral("tendency")).toString();
                if (data.pressureTendency.isEmpty()) {
                    data.pressureTendency = QStringLiteral("steady");
                }
                data.pressure = xml.readElementText().toFloat();
            } else if (elementName == QLatin1String("visibility")) {
                data.visibility = xml.readElementText().toFloat();
            } else if (elementName == QLatin1String("relativeHumidity")) {
                data.humidity = xml.readElementText();
            } else if (elementName == QLatin1String("wind")) {
                parseWindInfo(data, xml);
            }
            //} else {
            //    parseUnknownElement(xml);
            //}
        }
    }
    if (data.temperature.isEmpty())  {
        data.temperature = i18n("N/A");
    }
}

void EnvCanadaIon::parseWarnings(WeatherData &data, QXmlStreamReader& xml)
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

                if (!warning->timestamp.isEmpty() && !warning->url.isEmpty())  {
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


void EnvCanadaIon::parseWeatherForecast(WeatherData& data, QXmlStreamReader& xml)
{
    WeatherData::ForecastInfo* forecast = new WeatherData::ForecastInfo;
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

void EnvCanadaIon::parseRegionalNormals(WeatherData& data, QXmlStreamReader& xml)
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
                data.normalHigh = xml.readElementText();
            } else if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("low")) {
                data.normalLow = xml.readElementText();
            }
        }
    }
}

void EnvCanadaIon::parseForecast(WeatherData& data, QXmlStreamReader& xml, WeatherData::ForecastInfo *forecast)
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

void EnvCanadaIon::parseShortForecast(WeatherData::ForecastInfo *forecast, QXmlStreamReader& xml)
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
                forecast->popPrecent = xml.readElementText();
            }
            if (elementName == QLatin1String("textSummary")) {
                shortText = xml.readElementText();
                QMap<QString, ConditionIcons> forecastList = forecastIcons();
                if ((forecast->forecastPeriod == QLatin1String("tonight")) ||
                    (forecast->forecastPeriod.contains(QLatin1String("night")))) {
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

void EnvCanadaIon::parseUVIndex(WeatherData& data, QXmlStreamReader& xml)
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

void EnvCanadaIon::parseForecastTemperatures(WeatherData::ForecastInfo *forecast, QXmlStreamReader& xml)
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
                forecast->forecastTempLow = xml.readElementText();
            } else if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("high")) {
                forecast->forecastTempHigh = xml.readElementText();
            } else if (elementName == QLatin1String("textSummary")) {
                xml.readElementText();
            }
        }
    }
}

void EnvCanadaIon::parsePrecipitationForecast(WeatherData::ForecastInfo *forecast, QXmlStreamReader& xml)
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

void EnvCanadaIon::parsePrecipTotals(WeatherData::ForecastInfo *forecast, QXmlStreamReader& xml)
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

void EnvCanadaIon::parseWindForecast(WeatherData::ForecastInfo *forecast, QXmlStreamReader& xml)
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

void EnvCanadaIon::parseYesterdayWeather(WeatherData& data, QXmlStreamReader& xml)
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
                data.prevHigh = xml.readElementText();
            } else if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("low")) {
                data.prevLow = xml.readElementText();
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

void EnvCanadaIon::parseWeatherRecords(WeatherData& data, QXmlStreamReader& xml)
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
                data.recordHigh = xml.readElementText().toFloat();
            } else if (elementName == QLatin1String("temperature") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("extremeMin")) {
                data.recordLow = xml.readElementText().toFloat();
            } else if (elementName == QLatin1String("precipitation") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("extremeRainfall")) {
                data.recordRain = xml.readElementText().toFloat();
            } else if (elementName == QLatin1String("precipitation") && xml.attributes().value(QStringLiteral("class")) == QLatin1String("extremeSnowfall")) {
                data.recordSnow = xml.readElementText().toFloat();
            }
        }
    }
}

void EnvCanadaIon::parseAstronomicals(WeatherData& data, QXmlStreamReader& xml)
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
void EnvCanadaIon::parseUnknownElement(QXmlStreamReader& xml) const
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

void EnvCanadaIon::updateWeather(const QString& source)
{
    //qDebug() << "updateWeather()";

    QMap<QString, QString> dataFields;
    Plasma::DataEngine::Data data;

    data.insert(QStringLiteral("Country"), country(source));
    data.insert(QStringLiteral("Place"), QVariant(city(source) + QStringLiteral(", ") + territory(source)));
    data.insert(QStringLiteral("Region"), region(source));
    data.insert(QStringLiteral("Station"), station(source));

    data.insert(QStringLiteral("Latitude"), latitude(source));
    data.insert(QStringLiteral("Longitude"), longitude(source));

    // Real weather - Current conditions
    data.insert(QStringLiteral("Observation Period"), observationTime(source));
    data.insert(QStringLiteral("Current Conditions"), i18nc("weather condition", condition(source).toUtf8().data()));
    //qDebug() << "i18n condition string: " << qPrintable(condition(source));

    // Tell applet which icon to use for conditions and provide mapping for condition type to the icons to display
    QMap<QString, ConditionIcons> conditionList;
    conditionList = conditionIcons();

//TODO: Port to Plasma5
#if 0
    const double lati = latitude(source).remove(QRegExp("[^0-9.]")).toDouble();
    const double longi = longitude(source).remove(QRegExp("[^0-9.]")).toDouble();
    const Plasma::DataEngine::Data timeData = m_timeEngine->query(
            QString("Local|Solar|Latitude=%1|Longitude=%2")
                .arg(lati).arg(-1 * longi));

    if (timeData["Corrected Elevation"].toDouble() < 0.0) {
        conditionList.insert(QStringLiteral("decreasing cloud"), FewCloudsNight);
        conditionList.insert(QStringLiteral("mostly cloudy"), PartlyCloudyNight);
        conditionList.insert(QStringLiteral("partly cloudy"), PartlyCloudyNight);
        conditionList.insert(QStringLiteral("fair"), FewCloudsNight);
        //qDebug() << "Before sunrise/After sunset - using night icons\n";
    } else {
#endif
        conditionList.insert(QStringLiteral("decreasing cloud"), FewCloudsDay);
        conditionList.insert(QStringLiteral("mostly cloudy"), PartlyCloudyDay);
        conditionList.insert(QStringLiteral("partly cloudy"), PartlyCloudyDay);
        conditionList.insert(QStringLiteral("fair"), FewCloudsDay);
        //qDebug() << "Using daytime icons\n";
#if 0
    }
#endif

    data.insert(QStringLiteral("Condition Icon"), getWeatherIcon(conditionList, condition(source)));

    dataFields = temperature(source);
    data.insert(QStringLiteral("Temperature"), dataFields[QStringLiteral("temperature")]);

    // Do we have a comfort temperature? if so display it
    const QString &comfortTemperature = dataFields[QStringLiteral("comfortTemperature")];
    if (comfortTemperature != i18n("N/A") && !comfortTemperature.isEmpty()) {
        // TODO: switch might depend on temperature unit
        if (comfortTemperature.toFloat() <= 0) {
            data.insert(QStringLiteral("Windchill"), comfortTemperature);
            data.insert(QStringLiteral("Humidex"), i18n("N/A"));
        } else {
            data.insert(QStringLiteral("Windchill"), i18n("N/A"));
            data.insert(QStringLiteral("Humidex"), comfortTemperature);
        }
    } else {
        data.insert(QStringLiteral("Windchill"), i18n("N/A"));
        data.insert(QStringLiteral("Humidex"), i18n("N/A"));
    }

    // Used for all temperatures
    data.insert(QStringLiteral("Temperature Unit"), dataFields[QStringLiteral("temperatureUnit")]);

    data.insert(QStringLiteral("Dewpoint"), dewpoint(source));

    dataFields = pressure(source);
    data.insert(QStringLiteral("Pressure"), dataFields[QStringLiteral("pressure")]);
    data.insert(QStringLiteral("Pressure Unit"), dataFields[QStringLiteral("pressureUnit")]);
    data.insert(QStringLiteral("Pressure Tendency"), dataFields[QStringLiteral("pressureTendency")]);

    dataFields = visibility(source);
    data.insert(QStringLiteral("Visibility"), dataFields[QStringLiteral("visibility")]);
    data.insert(QStringLiteral("Visibility Unit"), dataFields[QStringLiteral("visibilityUnit")]);

    dataFields = humidity(source);
    data.insert(QStringLiteral("Humidity"), dataFields[QStringLiteral("humidity")]);
    data.insert(QStringLiteral("Humidity Unit"), dataFields[QStringLiteral("humidityUnit")]);

    dataFields = wind(source);
    data.insert(QStringLiteral("Wind Speed"), dataFields[QStringLiteral("windSpeed")]);
    data.insert(QStringLiteral("Wind Speed Unit"), dataFields[QStringLiteral("windUnit")]);

    data.insert(QStringLiteral("Wind Gust"), dataFields[QStringLiteral("windGust")]);
    data.insert(QStringLiteral("Wind Direction"), dataFields[QStringLiteral("windDirection")]);
    data.insert(QStringLiteral("Wind Degrees"), dataFields[QStringLiteral("windDegrees")]);
    data.insert(QStringLiteral("Wind Gust Unit"), dataFields[QStringLiteral("windGustUnit")]);

    dataFields = regionalTemperatures(source);
    data.insert(QStringLiteral("Normal High"), dataFields[QStringLiteral("normalHigh")]);
    data.insert(QStringLiteral("Normal Low"), dataFields[QStringLiteral("normalLow")]);

    // Check if UV index is available for the location
    dataFields = uvIndex(source);
    data.insert(QStringLiteral("UV Index"), dataFields[QStringLiteral("uvIndex")]);
    data.insert(QStringLiteral("UV Rating"), dataFields[QStringLiteral("uvRating")]);

    dataFields = watches(source);

    // Set number of forecasts per day/night supported
    data.insert(QStringLiteral("Total Watches Issued"), m_weatherData[source].watches.size());

    // Check if we have warnings or watches
    for (int i = 0; i < m_weatherData[source].watches.size(); ++i) {
        const QString number = QString::number(i);
        const QStringList fieldList = dataFields[QStringLiteral("watch ") + number].split(QLatin1Char('|'));
        data.insert(QStringLiteral("Watch Priority ") + number, fieldList[0]);
        data.insert(QStringLiteral("Watch Description ") + number, fieldList[1]);
        data.insert(QStringLiteral("Watch Info ") + number, fieldList[2]);
        data.insert(QStringLiteral("Watch Timestamp ") + number, fieldList[3]);
    }

    dataFields = warnings(source);

    data.insert(QStringLiteral("Total Warnings Issued"), m_weatherData[source].warnings.size());

    for (int k = 0; k < m_weatherData[source].warnings.size(); ++k) {
        const QString number = QString::number(k);
        const QStringList fieldList = dataFields[QStringLiteral("warning ") + number].split(QLatin1Char('|'));
        data.insert(QStringLiteral("Warning Priority ") + number, fieldList[0]);
        data.insert(QStringLiteral("Warning Description ") + number, fieldList[1]);
        data.insert(QStringLiteral("Warning Info ") + number, fieldList[2]);
        data.insert(QStringLiteral("Warning Timestamp ") + number, fieldList[3]);
    }

    const QVector<QString> forecastList = forecasts(source);

    // Set number of forecasts per day/night supported
    data.insert(QStringLiteral("Total Weather Days"), m_weatherData[source].forecasts.size());

    int i = 0;
    foreach(const QString& forecastItem, forecastList) {
        data.insert(QStringLiteral("Short Forecast Day %1").arg(i), forecastItem);

        /*
                data.insert(QString("Long Forecast Day %1").arg(i), QString("%1|%2|%3|%4|%5|%6|%7|%8") \
                        .arg(fieldList[0]).arg(fieldList[2]).arg(fieldList[3]).arg(fieldList[4]).arg(fieldList[6]) \
                        .arg(fieldList[7]).arg(fieldList[8]).arg(fieldList[9]));
        */
        i++;
    }

    dataFields = yesterdayWeather(source);
    data.insert(QStringLiteral("Yesterday High"), dataFields[QStringLiteral("prevHigh")]);
    data.insert(QStringLiteral("Yesterday Low"), dataFields[QStringLiteral("prevLow")]);

    data.insert(QStringLiteral("Yesterday Precip Total"), dataFields[QStringLiteral("prevPrecip")]);
    data.insert(QStringLiteral("Yesterday Precip Unit"), dataFields[QStringLiteral("prevPrecipUnit")]);

    dataFields = sunriseSet(source);
    data.insert(QStringLiteral("Sunrise At"), dataFields[QStringLiteral("sunrise")]);
    data.insert(QStringLiteral("Sunset At"), dataFields[QStringLiteral("sunset")]);

    dataFields = moonriseSet(source);
    data.insert(QStringLiteral("Moonrise At"), dataFields[QStringLiteral("moonrise")]);
    data.insert(QStringLiteral("Moonset At"), dataFields[QStringLiteral("moonset")]);

    dataFields = weatherRecords(source);
    data.insert(QStringLiteral("Record High Temperature"), dataFields[QStringLiteral("recordHigh")]);
    data.insert(QStringLiteral("Record Low Temperature"), dataFields[QStringLiteral("recordLow")]);

    data.insert(QStringLiteral("Record Rainfall"), dataFields[QStringLiteral("recordRain")]);
    data.insert(QStringLiteral("Record Rainfall Unit"), dataFields[QStringLiteral("recordRainUnit")]);
    data.insert(QStringLiteral("Record Snowfall"), dataFields[QStringLiteral("recordSnow")]);
    data.insert(QStringLiteral("Record Snowfall Unit"), dataFields[QStringLiteral("recordSnowUnit")]);

    data.insert(QStringLiteral("Credit"), i18n("Meteorological data is provided by Environment Canada"));
    setData(source, data);
}

QString const EnvCanadaIon::country(const QString& source) const
{
    // This will always return Canada
    return m_weatherData[source].countryName;
}

QString EnvCanadaIon::territory(const QString& source) const
{
    return m_weatherData[source].shortTerritoryName;
}

QString EnvCanadaIon::city(const QString& source) const
{
    return m_weatherData[source].cityName;
}

QString EnvCanadaIon::region(const QString& source) const
{
    return m_weatherData[source].regionName;
}

QString EnvCanadaIon::station(const QString& source) const
{
    const QString& stationID = m_weatherData[source].stationID;

    return stationID.isEmpty() ? i18n("N/A") : stationID.toUpper();
}

QString EnvCanadaIon::latitude(const QString& source) const
{
    return m_weatherData[source].stationLat;
}

QString EnvCanadaIon::longitude(const QString& source) const
{
    return m_weatherData[source].stationLon;
}

QString EnvCanadaIon::observationTime(const QString& source) const
{
    return m_weatherData[source].obsTimestamp;
}

int EnvCanadaIon::periodHour(const QString& source) const
{
    return m_weatherData[source].iconPeriodHour;
}

int EnvCanadaIon::periodMinute(const QString& source) const
{
    return m_weatherData[source].iconPeriodMinute;
}

QString EnvCanadaIon::condition(const QString& source)
{
    const QString& condition = m_weatherData[source].condition;

    return condition.isEmpty() ? i18n("N/A") : condition;
}

QString EnvCanadaIon::dewpoint(const QString& source) const
{
    const QString& dewpoint = m_weatherData[source].dewpoint;

    return dewpoint.isEmpty() ? i18n("N/A") : QString::number(m_weatherData[source].dewpoint.toFloat(), 'f', 1);
}

QMap<QString, QString> EnvCanadaIon::humidity(const QString& source) const
{
    QMap<QString, QString> humidityInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const QString& humidity = weatherData.humidity;
    humidityInfo.insert(QStringLiteral("humidity"), humidity.isEmpty() ? i18n("N/A") : humidity);
    humidityInfo.insert(QStringLiteral("humidityUnit"), QString::number(humidity.isEmpty() ? KUnitConversion::NoUnit : KUnitConversion::Percent));

    return humidityInfo;
}

QMap<QString, QString> EnvCanadaIon::visibility(const QString& source) const
{
    QMap<QString, QString> visibilityInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const float visibility = weatherData.visibility;
    visibilityInfo.insert(QStringLiteral("visibility"), (visibility == 0) ? i18n("N/A") : QString::number(m_weatherData[source].visibility, 'f', 1));
    visibilityInfo.insert(QStringLiteral("visibilityUnit"), QString::number((visibility == 0) ? KUnitConversion::NoUnit : KUnitConversion::Kilometer));

    return visibilityInfo;
}

QMap<QString, QString> EnvCanadaIon::temperature(const QString& source) const
{
    QMap<QString, QString> temperatureInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const QString& temperature = weatherData.temperature;
    temperatureInfo.insert(QStringLiteral("temperature"), temperature == i18n("N/A") ? temperature : QString::number(temperature.toFloat(), 'f', 1));

    const QString& comforttemp = weatherData.comforttemp;
    temperatureInfo.insert(QStringLiteral("comfortTemperature"), comforttemp);

    // This is used for not just current temperature but also 8 days. Cannot be NoUnit.
    temperatureInfo.insert(QStringLiteral("temperatureUnit"), QString::number(KUnitConversion::Celsius));

    return temperatureInfo;
}

QMap<QString, QString> EnvCanadaIon::watches(const QString& source) const
{
    QMap<QString, QString> watchData;

    const QVector<WeatherData::WeatherEvent*>& watches = m_weatherData[source].watches;
    for (int i = 0; i < watches.size(); ++i) {
        const WeatherData::WeatherEvent* watch = watches.at(i);
        watchData.insert(QStringLiteral("watch %1").arg(i),
                         QStringLiteral("%1|%2|%3|%4")
                         .arg(watch->priority, watch->description, watch->url, watch->timestamp));
    }

    return watchData;
}

QMap<QString, QString> EnvCanadaIon::warnings(const QString& source) const
{
    QMap<QString, QString> warningData;

    const QVector<WeatherData::WeatherEvent*>& warnings = m_weatherData[source].warnings;
    for (int i = 0; i < warnings.size(); ++i) {
        const WeatherData::WeatherEvent* warning = warnings.at(i);
        warningData.insert(QStringLiteral("warning %1").arg(i),
                           QStringLiteral("%1|%2|%3|%4")
                           .arg(warning->priority, warning->description, warning->url, warning->timestamp));
    }

    return warningData;
}

QVector<QString> EnvCanadaIon::forecasts(const QString& source)
{
    QVector<QString> forecastData;

    // Do some checks for empty data
    for (int i = 0; i < m_weatherData[source].forecasts.size(); ++i) {
        if (m_weatherData[source].forecasts[i]->forecastPeriod.isEmpty()) {
            m_weatherData[source].forecasts[i]->forecastPeriod = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->shortForecast.isEmpty()) {
            m_weatherData[source].forecasts[i]->shortForecast = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->iconName.isEmpty()) {
            m_weatherData[source].forecasts[i]->iconName = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->forecastSummary.isEmpty()) {
            m_weatherData[source].forecasts[i]->forecastSummary = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->forecastTempHigh.isEmpty()) {
            m_weatherData[source].forecasts[i]->forecastTempHigh = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->forecastTempLow.isEmpty()) {
            m_weatherData[source].forecasts[i]->forecastTempLow = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->popPrecent.isEmpty()) {
            m_weatherData[source].forecasts[i]->popPrecent = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->windForecast.isEmpty()) {
            m_weatherData[source].forecasts[i]->windForecast = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->precipForecast.isEmpty()) {
            m_weatherData[source].forecasts[i]->precipForecast = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->precipType.isEmpty()) {
            m_weatherData[source].forecasts[i]->precipType = i18n("N/A");
        }
        if (m_weatherData[source].forecasts[i]->precipTotalExpected.isEmpty()) {
            m_weatherData[source].forecasts[i]->precipTotalExpected = i18n("N/A");
        }
    }

    foreach(const WeatherData::ForecastInfo *forecastInfo, m_weatherData[source].forecasts) {
        QString forecastPeriod = forecastInfo->forecastPeriod;
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

        forecastData.append(QStringLiteral("%1|%2|%3|%4|%5|%6")
                            .arg(forecastPeriod, forecastInfo->iconName,
                                 i18nc("weather forecast", forecastInfo->shortForecast.toUtf8().data()),
                                 forecastInfo->forecastTempHigh, forecastInfo->forecastTempLow,
                                 forecastInfo->popPrecent));
        //qDebug() << "i18n summary string: " << qPrintable(i18n(forecastInfo->shortForecast.toUtf8()));
    }
    return forecastData;
}

QMap<QString, QString> EnvCanadaIon::pressure(const QString& source) const
{
    QMap<QString, QString> pressureInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const float pressure = weatherData.pressure;
    pressureInfo.insert(QStringLiteral("pressure"), (pressure == 0) ? i18n("N/A") : QString::number(pressure, 'f', 1));
    pressureInfo.insert(QStringLiteral("pressureUnit"), QString::number((pressure == 0) ? KUnitConversion::NoUnit : KUnitConversion::Kilopascal));
    pressureInfo.insert(QStringLiteral("pressureTendency"), (pressure == 0) ? QStringLiteral("N/A") : i18nc("pressure tendency", weatherData.pressureTendency.toUtf8().data()));

    return pressureInfo;
}

QMap<QString, QString> EnvCanadaIon::wind(const QString& source) const
{
    QMap<QString, QString> windInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const QString& windSpeed = weatherData.windSpeed;
    // May not have any winds
    if (windSpeed.isEmpty()) {
        windInfo.insert(QStringLiteral("windSpeed"), i18n("N/A"));
        windInfo.insert(QStringLiteral("windUnit"), QString::number(KUnitConversion::NoUnit));
    } else if (windSpeed.toInt() == 0) {
        windInfo.insert(QStringLiteral("windSpeed"), i18nc("wind speed", "Calm"));
        windInfo.insert(QStringLiteral("windUnit"), QString::number(KUnitConversion::NoUnit));
    } else {
        windInfo.insert(QStringLiteral("windSpeed"), QString::number(windSpeed.toInt()));
        windInfo.insert(QStringLiteral("windUnit"), QString::number(KUnitConversion::KilometerPerHour));
    }

    const QString& windGust = weatherData.windGust;
    // May not always have gusty winds
    windInfo.insert(QStringLiteral("windGust"), windGust.isEmpty() ? i18n("N/A") : QString::number(windGust.toInt()));
    windInfo.insert(QStringLiteral("windGustUnit"), QString::number(windGust.isEmpty() ? KUnitConversion::NoUnit : KUnitConversion::KilometerPerHour));

    const QString& windDirection = weatherData.windDirection;
    if (windDirection.isEmpty() && windSpeed.isEmpty()) {
        windInfo.insert(QStringLiteral("windDirection"), i18n("N/A"));
        windInfo.insert(QStringLiteral("windDegrees"), i18n("N/A"));
    } else if (windSpeed.toInt() == 0) {
        windInfo.insert(QStringLiteral("windDirection"), i18nc("wind direction - wind speed is too low to measure", "VR")); // Variable/calm
    } else {
        windInfo.insert(QStringLiteral("windDirection"), i18nc("wind direction", windDirection.toUtf8().data()));
        windInfo.insert(QStringLiteral("windDegrees"), weatherData.windDegrees);
    }
    return windInfo;
}

QMap<QString, QString> EnvCanadaIon::uvIndex(const QString& source) const
{
    QMap<QString, QString> uvInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const QString& UVRating = weatherData.UVRating;
    uvInfo.insert(QStringLiteral("uvRating"), UVRating.isEmpty() ? i18n("N/A") : UVRating);

    const QString& UVIndex = weatherData.UVIndex;
    uvInfo.insert(QStringLiteral("uvIndex"), UVIndex.isEmpty() ? i18n("N/A") : UVIndex);

    return uvInfo;
}

QMap<QString, QString> EnvCanadaIon::regionalTemperatures(const QString& source) const
{
    QMap<QString, QString> regionalTempInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const QString& normalHigh = weatherData.normalHigh;
    regionalTempInfo.insert(QStringLiteral("normalHigh"), normalHigh.isEmpty() ? i18n("N/A") : normalHigh);

    const QString& normalLow = weatherData.normalLow;
    regionalTempInfo.insert(QStringLiteral("normalLow"), normalLow.isEmpty() ? i18n("N/A") : normalLow);

    return regionalTempInfo;
}

QMap<QString, QString> EnvCanadaIon::yesterdayWeather(const QString& source) const
{
    QMap<QString, QString> yesterdayInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const QString& prevHigh = weatherData.prevHigh;
    yesterdayInfo.insert(QStringLiteral("prevHigh"), prevHigh.isEmpty() ? i18n("N/A") : prevHigh);

    const QString& prevLow = weatherData.prevLow;
    yesterdayInfo.insert(QStringLiteral("prevLow"), prevLow.isEmpty() ? i18n("N/A") : prevLow);

    const QString& prevPrecipTotal = weatherData.prevPrecipTotal;
    if (prevPrecipTotal == QLatin1String("Trace")) {
        yesterdayInfo.insert(QStringLiteral("prevPrecip"), i18nc("precipitation total, very little", "Trace"));
    } else if (prevPrecipTotal.isEmpty()) {
        yesterdayInfo.insert(QStringLiteral("prevPrecip"), i18n("N/A"));
        yesterdayInfo.insert(QStringLiteral("prevPrecipUnit"), QString::number(KUnitConversion::NoUnit));
    } else {
        yesterdayInfo.insert(QStringLiteral("prevPrecipTotal"), prevPrecipTotal);
        const QString& prevPrecipType = weatherData.prevPrecipType;
        const KUnitConversion::UnitId unit =
            (prevPrecipType == QLatin1String("mm")) ? KUnitConversion::Millimeter :
            (prevPrecipType == QLatin1String("cm")) ? KUnitConversion::Centimeter :
            /*else*/                                  KUnitConversion::NoUnit;
        yesterdayInfo.insert(QStringLiteral("prevPrecipUnit"), QString::number(unit));
    }

    return yesterdayInfo;
}

QMap<QString, QString> EnvCanadaIon::sunriseSet(const QString& source) const
{
    QMap<QString, QString> sunInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const QString& sunriseTimestamp = weatherData.sunriseTimestamp;
    sunInfo.insert(QStringLiteral("sunrise"), sunriseTimestamp.isEmpty() ? i18n("N/A") : sunriseTimestamp);

    const QString& sunsetTimestamp = weatherData.sunsetTimestamp;
    sunInfo.insert(QStringLiteral("sunset"), sunsetTimestamp.isEmpty() ? i18n("N/A") : sunsetTimestamp);

    return sunInfo;
}

QMap<QString, QString> EnvCanadaIon::moonriseSet(const QString& source) const
{
    QMap<QString, QString> moonInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const QString& moonriseTimestamp = weatherData.moonriseTimestamp;
    moonInfo.insert(QStringLiteral("moonrise"), moonriseTimestamp.isEmpty() ? i18n("N/A") : moonriseTimestamp);

    const QString& moonsetTimestamp = weatherData.moonsetTimestamp;
    moonInfo.insert(QStringLiteral("moonset"), moonsetTimestamp.isEmpty() ? i18n("N/A"): moonsetTimestamp);

    return moonInfo;
}

QMap<QString, QString> EnvCanadaIon::weatherRecords(const QString& source) const
{
    QMap<QString, QString> recordInfo;

    const WeatherData& weatherData = m_weatherData[source];

    const float recordHigh = weatherData.recordHigh;
    recordInfo.insert(QStringLiteral("recordHigh"), (recordHigh == 0) ? i18n("N/A") : QString::number(recordHigh, 'f', -1));

    const float recordLow = weatherData.recordLow;
    recordInfo.insert(QStringLiteral("recordLow"), (recordLow == 0) ? i18n("N/A") : QString::number(recordLow, 'f', -1));

    const float recordRain = weatherData.recordRain;
    recordInfo.insert(QStringLiteral("recordRain"), (recordRain == 0) ? i18n("N/A") : QString::number(recordRain, 'f', -1));
    recordInfo.insert(QStringLiteral("recordRainUnit"), QString::number((recordRain == 0) ?KUnitConversion::NoUnit : KUnitConversion::Millimeter));

    const float recordSnow = weatherData.recordSnow;
    recordInfo.insert(QStringLiteral("recordSnow"), (recordSnow == 0) ? i18n("N/A") : QString::number(recordSnow, 'f', -1));
    recordInfo.insert(QStringLiteral("recordSnowUnit"), QString::number((recordSnow == 0) ? KUnitConversion::NoUnit : KUnitConversion::Centimeter));

    return recordInfo;
}


K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(envcan, EnvCanadaIon, "ion-envcan.json")

#include "ion_envcan.moc"
