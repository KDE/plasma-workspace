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

#include <QtCore/QObject>
#include <Plasma/DataEngine>

#include "ion_export.h"

/**
* @author Shawn Starr
* This is the base class to be used to implement new ions for the WeatherEngine.
* The idea is that you can have multiple ions which provide weather information from different services to the engine from which an applet will request the data from.
*
* Basically an ion is a Plasma::DataEngine, which is queried by the WeatherEngine instead of some applet.
*/
class ION_EXPORT IonInterface : public Plasma::DataEngine
{
    Q_OBJECT

public:

    enum ConditionIcons { ClearDay = 1, FewCloudsDay, PartlyCloudyDay, Overcast,
                          Rain, LightRain, Showers, ChanceShowersDay, Thunderstorm, Hail,
                          Snow, LightSnow, Flurries, FewCloudsNight, ChanceShowersNight,
                          PartlyCloudyNight, ClearNight, Mist, Haze, FreezingRain,
                          RainSnow, FreezingDrizzle, ChanceThunderstormDay, ChanceThunderstormNight,
                          ChanceSnowDay, ChanceSnowNight, NotAvailable
                        };

    enum WindDirections { N, NNE, NE, ENE, E, SSE, SE, ESE, S, NNW, NW, WNW, W, SSW, SW, WSW, VR };

    /**
     * Constructor for the ion
     * @param parent The parent object.
     * @Param args The argument list.
     */
    explicit IonInterface(QObject *parent = 0, const QVariantList &args = QVariantList());
    /**
     * Destructor for the ion
     */
    virtual ~IonInterface();

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
    QString getWeatherIcon(const QMap<QString, ConditionIcons> &conditionList, const QString& condition) const;

    /**
     * Returns wind icon element to display in applet.
     * @param windDirList a QList map pair of wind directions mapped to a enumeration of directions.
     * @param windDirection the current wind direction.
     * @return svg element for wind direction
     */
    QString getWindDirectionIcon(const QMap<QString, WindDirections> &windDirList, const QString& windDirection) const;

public Q_SLOTS:

    /**
     * Reimplemented from Plasma::DataEngine
     * @param source the name of the datasource to be updated
     */
    bool updateSourceEvent(const QString& source);

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
    bool sourceRequestEvent(const QString &source);

    /**
     * Reimplement to fetch the data from the ion.
     * @arg source the name of the datasource.
     * @return true if update was successful, false if failed
     */
    virtual bool updateIonSource(const QString &source) = 0;

    friend class WeatherEngine;

private:
    class Private;
    Private* const d;
};

#endif

