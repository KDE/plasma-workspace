/*****************************************************************************
 * Copyright (C) 2007-2009 by Shawn Starr <shawn.starr@rogers.com>           *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License as published by the Free Software Foundation; either              *
 * version 2 of the License, or (at your option) any later version.          *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#include "ion.h"

#include "iondebug.h"

#include <KLocalizedString>

class Q_DECL_HIDDEN IonInterface::Private
{
public:
    Private(IonInterface *i)
        : ion(i)
        , initialized(false)
    {
    }

    IonInterface *ion;
    bool initialized;
};

IonInterface::IonInterface(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
    , d(new Private(this))
{
}

IonInterface::~IonInterface()
{
    delete d;
}

/**
 * If the ion is not initialized just set the initial data source up even if it's empty, we'll retry once the initialization is done
 */
bool IonInterface::sourceRequestEvent(const QString &source)
{
    qCDebug(IONENGINE) << "sourceRequested(): " << source;

    // init anyway the data as it's going to be used
    // sooner or later (doesn't depend upon initialization
    // this will avoid problems if updateIonSource() fails for any reason
    // but later it's able to retrieve the data
    setData(source, Plasma::DataEngine::Data());

    // if initialized, then we can try to grab the data
    if (d->initialized) {
        return updateIonSource(source);
    }

    return true;
}

/**
 * Update the ion's datasource. Triggered when a Plasma::DataEngine::connectSource() timeout occurs.
 */
bool IonInterface::updateSourceEvent(const QString &source)
{
    qCDebug(IONENGINE) << "updateSource(" << source << ")";
    if (d->initialized) {
        qCDebug(IONENGINE) << "Calling updateIonSource(" << source << ")";
        return updateIonSource(source);
    }

    return false;
}

/**
 * Set the ion to make sure it is ready to get real data.
 */
void IonInterface::setInitialized(bool initialized)
{
    d->initialized = initialized;

    if (d->initialized) {
        updateAllSources();
    }
}

/**
 * Return wind direction svg element to display in applet when given a wind direction.
 */
QString IonInterface::getWindDirectionIcon(const QMap<QString, WindDirections> &windDirList, const QString &windDirection) const
{
    switch (windDirList[windDirection.toLower()]) {
    case N:
        return QStringLiteral("N");
    case NNE:
        return QStringLiteral("NNE");
    case NE:
        return QStringLiteral("NE");
    case ENE:
        return QStringLiteral("ENE");
    case E:
        return QStringLiteral("E");
    case SSE:
        return QStringLiteral("SSE");
    case SE:
        return QStringLiteral("SE");
    case ESE:
        return QStringLiteral("ESE");
    case S:
        return QStringLiteral("S");
    case NNW:
        return QStringLiteral("NNW");
    case NW:
        return QStringLiteral("NW");
    case WNW:
        return QStringLiteral("WNW");
    case W:
        return QStringLiteral("W");
    case SSW:
        return QStringLiteral("SSW");
    case SW:
        return QStringLiteral("SW");
    case WSW:
        return QStringLiteral("WSW");
    case VR:
        return QStringLiteral("VR"); // For now, we'll make a variable wind icon later on
    }

    // No icon available, use 'X'
    return QString();
}

/**
 * Return weather icon to display in an applet when given a condition.
 */
QString IonInterface::getWeatherIcon(ConditionIcons condition) const
{
    switch (condition) {
    case ClearDay:
        return QStringLiteral("weather-clear");
    case ClearWindyDay:
        return QStringLiteral("weather-clear-wind");
    case FewCloudsDay:
        return QStringLiteral("weather-few-clouds");
    case FewCloudsWindyDay:
        return QStringLiteral("weather-few-clouds-wind");
    case PartlyCloudyDay:
        return QStringLiteral("weather-clouds");
    case PartlyCloudyWindyDay:
        return QStringLiteral("weather-clouds-wind");
    case Overcast:
        return QStringLiteral("weather-overcast");
    case OvercastWindy:
        return QStringLiteral("weather-overcast-wind");
    case Rain:
        return QStringLiteral("weather-showers");
    case LightRain:
        return QStringLiteral("weather-showers-scattered");
    case Showers:
        return QStringLiteral("weather-showers-scattered");
    case ChanceShowersDay:
        return QStringLiteral("weather-showers-scattered-day");
    case ChanceShowersNight:
        return QStringLiteral("weather-showers-scattered-night");
    case ChanceSnowDay:
        return QStringLiteral("weather-snow-scattered-day");
    case ChanceSnowNight:
        return QStringLiteral("weather-snow-scattered-night");
    case Thunderstorm:
        return QStringLiteral("weather-storm");
    case Hail:
        return QStringLiteral("weather-hail");
    case Snow:
        return QStringLiteral("weather-snow");
    case LightSnow:
        return QStringLiteral("weather-snow-scattered");
    case Flurries:
        return QStringLiteral("weather-snow-scattered");
    case RainSnow:
        return QStringLiteral("weather-snow-rain");
    case FewCloudsNight:
        return QStringLiteral("weather-few-clouds-night");
    case FewCloudsWindyNight:
        return QStringLiteral("weather-few-clouds-wind-night");
    case PartlyCloudyNight:
        return QStringLiteral("weather-clouds-night");
    case PartlyCloudyWindyNight:
        return QStringLiteral("weather-clouds-wind-night");
    case ClearNight:
        return QStringLiteral("weather-clear-night");
    case ClearWindyNight:
        return QStringLiteral("weather-clear-wind-night");
    case Mist:
        return QStringLiteral("weather-fog");
    case Haze:
        return QStringLiteral("weather-fog");
    case FreezingRain:
        return QStringLiteral("weather-freezing-rain");
    case FreezingDrizzle:
        return QStringLiteral("weather-freezing-rain");
    case ChanceThunderstormDay:
        return QStringLiteral("weather-storm-day");
    case ChanceThunderstormNight:
        return QStringLiteral("weather-storm-night");
    case NotAvailable:
        return QStringLiteral("weather-none-available");
    }
    return QStringLiteral("weather-none-available");
}

/**
 * Return weather icon to display in an applet when given a condition.
 */
QString IonInterface::getWeatherIcon(const QMap<QString, ConditionIcons> &conditionList, const QString &condition) const
{
    return getWeatherIcon(conditionList[condition.toLower()]);
}
