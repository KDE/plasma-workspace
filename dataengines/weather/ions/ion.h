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

#ifndef ION_H
#define ION_H

#include <Plasma/DataEngine>

#include "ion_export.h"

/**
 * @author Shawn Starr
 * This is the base class to be used to implement new ions for the WeatherEngine.
 * The idea is that you can have multiple ions which provide weather information from different services to the engine from which an applet will request the
 * data from.
 *
 * Basically an ion is a Plasma::DataEngine, which is queried by the WeatherEngine instead of some applet.
 *
 *
 * Find following the currently used entries of the data for a "SOMEION|weather|SOMEPLACE" source.
 * Any free text strings should be translated by the dataengine if possible,
 * as any dataengine user has less knowledge about the meaning of the strings.
 *
 * Data about the weather station:
 * "Station": string, id of the location of the weather station with the service, required, TODO: ensure it's id
 * "Place": string, display name of the location of the weather station, required, TODO: what details here, country?
 * "Country": string, display name of country of the weather station, optional
 * "Latitude": float, latitude of the weather station in decimal degrees, optional
 * "Longitude": float, longitude of the weather station in decimal degrees, optional
 *
 * Data about last observation:
 * "Observation Period": string, free text string for time of observation, optional
 * "Observation Timestamp": datetime (with timezone), time of observation, optional
 * "Current Conditions": string, free text string for current weather observation, optional
 * "Condition Icon": string, xdg icon name for current weather observation, optional
 * "Temperature": float, using general temperature unit, optional
 * "Windchill": float, felt temperature due to wind, using general temperature unit, optional
 * "Heat Index": float, using general temperature unit, optional
 * "Humidex": int, humidity index (not to be mixed up with heat index), optional
 * "Wind Speed": float, average wind speed, optional TODO: fix "Calm" strings injected on dataengine side, it's a display thing?
 * "Wind Speed Unit": int, kunitconversion enum number for the unit with all wind speed values, required if wind speeds are given
 * "Wind Gust": float, max wind gust speed, optional
 * "Wind Direction": string, wind direction in cardinal directions (up to secondary-intercardinal + VR), optional
 * "Visibility": float, visibility in distance, optional
 * "Visibility Unit": int, kunitconversion enum number for the unit with all visibility values, required if visibilities are given
 * "Pressure": float, air pressure, optional
 * "Pressure Tendency": string, "rising", "falling", "steady", optional TODO: turn into enum/string id set, currently passed as string
 * "Pressure Unit": int, kunitconversion enum number for the unit with all pressure values, required if pressures are given
 * "UV Index": int, value in UV index UN standard, optional
 * "UV Rating": string, grouping in which UV index is: "Low"0</"Moderate"3</"High"6</"Very high"8</"Extreme"11<, optional TODO: can be calculated from UV Index,
 * so needed? "Humidity": float, humidity of the air, optional "Humidity Unit": int, kunitconversion enum number for the unit with all humidity values, required
 * if humidities are given TODO: any other unit than percent expected? "Dewpoint": string, temperature where water condensates given other conditions, optional
 *
 * Data about current day:
 * "Normal High": float, average highest temperature measured at location, optional
 * "Normal Low": float, average lowest temperature measured at location, optional
 * "Record High Temperature": float, highest temperature ever measured at location, optional
 * "Record Low Temperature": float, lowest temperature ever measured at location, optional
 * "Record Rainfall": float, highest height of rain (precipitation?) ever measured at location, optional
 * "Record Rainfall Unit": int, kunitconversion enum number for the unit with all rainfall values, required if rainfalls are given
 * "Record Snowfall": float, highest height of snow ever measured at location, optional
 * "Record Snowfall Unit": int, kunitconversion enum number for the unit with Record Snowfall, required if snowfall is given
 *
 * Data about last day:
 * "Yesterday High": float, highest temperature at location, optional
 * "Yesterday Low": float, lowest temperature at location, optional
 * "Yesterday Precip Total": float, total precipitation over day, optional TODO: "Trace" injected, should be dealt with at display side?
 * "Yesterday Precip Unit": int, kunitconversion enum number for the unit with recip, required if precipitation is given
 *
 * Data about next days:
 * "Total Weather Days": int
 * "Short Forecast Day %1": string, 6 fields separated by |, %1 number in list, required
 *     fields are (optional means empty string possible):
 *     period: time of, required, TODO: have standardized to allow better processing
 *     condition icon: xdg icon name for current weather observation, optional,
 *     condition: free text string for weather condition, optional
 *     temperature high: number of highest temperature (using general unit), optional
 *     temperature low: number of lowest temperature (using general unit), optional
 *     probability: chance of conditions to happen, optional
 *     TODO: this should be rather a QVariantList if possible
 *
 * Data about warning/watches:
 * "Total Warnings Issued": int, number of warnings, there should be matching "Watch * "+number data, TODO: what is a warning exactly?
 * "Total Watches Issued": int, number of watches, there should be matching "Watch * "+number data, TODO: what is a watch exactly?
 * "Warning Description " + number, string, free text string, optional
 * "Warning Info " + number, string, free text string, optional
 * "Warning Priority " + number, string, free text string, optional TODO: get standardized enum
 * "Warning Timestamp " + number, string, free text string, optional TODO: get standardized datetime
 * "Watch Description " + number, string, free text string, optional
 * "Watch Info " + number, string, free text string, optional
 * "Watch Priority " + number, string, free text string, optional TODO: get standardized enum
 * "Watch Timestamp " + number, string, free text string, optional TODO: get standardized datetime
 *
 * Data about the data:
 * "Temperature Unit": int, kunitconversion enum number for the unit with all temperature values, required if temperatures are given
 *
 * "Credit": string, credit line for the data, required
 * "Credit Url": string, url related to the credit for the data (can be also webpage with more forecast), optional
 */
class ION_EXPORT IonInterface : public Plasma::DataEngine
{
    Q_OBJECT

public:
    enum ConditionIcons {
        ClearDay = 1,
        ClearWindyDay,
        FewCloudsDay,
        FewCloudsWindyDay,
        PartlyCloudyDay,
        PartlyCloudyWindyDay,
        Overcast,
        OvercastWindy,
        Rain,
        LightRain,
        Showers,
        ChanceShowersDay,
        Thunderstorm,
        Hail,
        Snow,
        LightSnow,
        Flurries,
        FewCloudsNight,
        FewCloudsWindyNight,
        ChanceShowersNight,
        PartlyCloudyNight,
        PartlyCloudyWindyNight,
        ClearNight,
        ClearWindyNight,
        Mist,
        Haze,
        FreezingRain,
        RainSnow,
        FreezingDrizzle,
        ChanceThunderstormDay,
        ChanceThunderstormNight,
        ChanceSnowDay,
        ChanceSnowNight,
        NotAvailable,
    };

    enum WindDirections {
        N,
        NNE,
        NE,
        ENE,
        E,
        SSE,
        SE,
        ESE,
        S,
        NNW,
        NW,
        WNW,
        W,
        SSW,
        SW,
        WSW,
        VR,
    };

    /**
     * Constructor for the ion
     * @param parent The parent object.
     * @Param args The argument list.
     */
    explicit IonInterface(QObject *parent = nullptr, const QVariantList &args = QVariantList());
    /**
     * Destructor for the ion
     */
    ~IonInterface() override;

    /**
     * Returns weather icon filename to display in applet.
     * @param condition the current condition being reported.
     * @return icon name
     */
    QString getWeatherIcon(ConditionIcons condition) const;

    /**
     * Returns weather icon filename to display in applet.
     * @param conditionList a QList map pair of icons mapped to a enumeration of conditions.
     * @param condition the current condition being reported.
     * @return icon name
     */
    QString getWeatherIcon(const QMap<QString, ConditionIcons> &conditionList, const QString &condition) const;

    /**
     * Returns wind icon element to display in applet.
     * @param windDirList a QList map pair of wind directions mapped to a enumeration of directions.
     * @param windDirection the current wind direction.
     * @return svg element for wind direction
     */
    QString getWindDirectionIcon(const QMap<QString, WindDirections> &windDirList, const QString &windDirection) const;

public Q_SLOTS:

    /**
     * Reimplemented from Plasma::DataEngine
     * @param source the name of the datasource to be updated
     */
    bool updateSourceEvent(const QString &source) override;

    /**
     * Reimplement for ion to reload data if network status comes back up
     */
    virtual void reset() = 0;

Q_SIGNALS:
    void forceUpdate(IonInterface *ion, const QString &source);

protected:
    /**
     * Call this method to flush waiting source requests that may be pending
     * initialization
     *
     * @arg initialized whether or not the ion is currently ready to fetch data
     */
    void setInitialized(bool initialized);

    /**
     * Reimplemented from Plasma::DataEngine
     * @param source The datasource being requested
     */
    bool sourceRequestEvent(const QString &source) override;

    /**
     * Reimplement to fetch the data from the ion.
     * @arg source the name of the datasource.
     * @return true if update was successful, false if failed
     */
    virtual bool updateIonSource(const QString &source) = 0;

    friend class WeatherEngine;

private:
    class Private;
    Private *const d;
};

#endif
