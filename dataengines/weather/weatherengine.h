/***************************************************************************
 *   Copyright (C) 2007-2009 by Shawn Starr <shawn.starr@rogers.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
#include < *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

#ifndef WEATHERENGINE_H
#define WEATHERENGINE_H

#include <QTimer>
#include <QNetworkAccessManager>

#include <Plasma/DataEngine>
#include <Plasma/DataEngineConsumer>

#include "ions/ion.h"


/**
 * @author Shawn Starr
 * This class is DataEngine. It handles loading, unloading, updating any data the ions wish to send. It is a gateway for datasources (ions) to
 * communicate with the WeatherEngine.
 * 
 * To search for a city:
 * ion|validate|name such as noaa|validate|washington it will return a | separated list of valid places
 *
 * To fetch the weather:
 * ion|weather|place name  where the place name is a name returned by the former validate
 * noaa|weather|Claxton Evans County Airport, GA
 * 
 * Some ions may have a longer syntax, for instance wetter.com requires two extra params
 * for instance:
 * 
 * wettercom|validate|turin may return a list with items on the form
 *    Turin, Piemont, IT|extra|IT0PI0397;Turin
 * 
 * Thus the query for weather will be on the form:
 * 
 * wettercom|weather|Turin, Piemont, IT|IT0PI0397;Turin
 * 
 * with the extra strings appended after extra
 */

class WeatherEngine : public Plasma::DataEngine, public Plasma::DataEngineConsumer
{
    Q_OBJECT

public:
    /** Constructor
     * @param parent The parent object.
     * @param args Argument list, unused.
     */
    WeatherEngine(QObject *parent, const QVariantList &args);

    // Destructor
    ~WeatherEngine();

    // initialization
    void init();

    /**
     * Load a plugin
     * @arg pluginName Name of the plugin
     * @return IonInterface returns an instance of the loaded plugin
     */
    DataEngine* loadIon(const QString& pluginName);

protected:
    /**
     * Reimplemented from Plasma::DataEngine. We use it to communicate to the Ion plugins to set the data sources.
     * @param source The datasource name.
     */
    bool sourceRequestEvent(const QString &source);

protected Q_SLOTS:
    /**
     * Reimplemented from Plasma::DataEngine.
     * @param source The datasource to be updated.
     * @param data The new data updated.
     */
    void dataUpdated(const QString& source, Plasma::DataEngine::Data data);

    void forceUpdate(IonInterface *ion, const QString &source);

    /**
     * Notify WeatherEngine a new ion has data sources.
     * @arg source datasource name.
     */
    void newIonSource(const QString& source);
    /**
     * Notify WeatherEngine a datasource is being removed.
     * @arg source datasource name.
     */
    void removeIonSource(const QString& source);
    /**
     * Reimplemented from Plasma::DataEngine.
     * @param source The datasource to update.
     */
    bool updateSourceEvent(const QString& source);

    /**
     * Whenever networking changes, take action
     */
    void networkStatusChanged(QNetworkAccessManager::NetworkAccessibility);
    void startReconnect();

    /**
     * updates the list of ions whenever KSycoca changes (as well as on init
     */
    void updateIonList(const QStringList &changedResources = QStringList());

private:
    /**
     * Get instance of a loaded ion.
     * @returns a IonInterface instance of a loaded plugin.
     */
    IonInterface* ionForSource(const QString& name);

    /**
     * Get plugin name from datasource.
     * @returns The plugin name given a datasource.
     */
    QString ionNameForSource(const QString& source) const;

    QStringList m_ions;
    bool m_networkAvailable;
    QTimer m_reconnectTimer;
    QNetworkAccessManager *m_networkAccessManager;
};

#endif
