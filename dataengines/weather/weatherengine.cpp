/***************************************************************************
 *   Copyright (C) 2007-2009 by Shawn Starr <shawn.starr@rogers.com>       *
 *   Copyright (C) 2009 by Aaron Seigo <aseigo@kde.org>                    *
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

#include "weatherengine.h"

#include <QTimer>

#include <KSycoca>

#include <Plasma/DataContainer>
#include <Plasma/PluginLoader>

#include "ions/ion.h"
#include "weatherenginedebug.h"

// Constructor
WeatherEngine::WeatherEngine(QObject *parent, const QVariantList& args)
        :  Plasma::DataEngine(parent, args)
{
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &WeatherEngine::startReconnect);

    // Globally notify all plugins to remove their sources (and unload plugin)
    connect(this, &Plasma::DataEngine::sourceRemoved, this, &WeatherEngine::removeIonSource);

    connect(&m_networkConfigurationManager, &QNetworkConfigurationManager::onlineStateChanged,
            this, &WeatherEngine::onOnlineStateChanged);

    // Get the list of available plugins but don't load them
    connect(KSycoca::self(), static_cast<void (KSycoca::*)(const QStringList&)>(&KSycoca::databaseChanged),
            this, &WeatherEngine::updateIonList);

    updateIonList();
}

// Destructor
WeatherEngine::~WeatherEngine()
{
}

/* FIXME: Q_PROPERTY functions to update the list of available plugins */

void WeatherEngine::updateIonList(const QStringList &changedResources)
{
    if (changedResources.isEmpty() || changedResources.contains(QLatin1String("services"))) {
        removeAllData(QStringLiteral("ions"));
        const auto infos = Plasma::PluginLoader::self()->listEngineInfo(QStringLiteral("weatherengine"));
        for (const KPluginInfo& info : infos) {
            const QString data = info.name() + QLatin1Char('|') + info.pluginName();
            setData(QStringLiteral("ions"), info.pluginName(), data);
        }
    }
}


/**
 * SLOT: Remove the datasource from the ion and unload plugin if needed
 */
void WeatherEngine::removeIonSource(const QString& source)
{
    QString ionName;
    IonInterface *ion = ionForSource(source, &ionName);
    if (ion) {
        ion->removeSource(source);

        // track used ions
        QHash<QString, int>::Iterator it = m_ionUsage.find(ionName);

        if (it == m_ionUsage.end()) {
            qCWarning(WEATHER) << "Removing ion source without being added before:" << source;
        } else {
            // no longer used?
            if (it.value() <= 1) {
                // forget about it
                m_ionUsage.erase(it);
                disconnect(ion, &IonInterface::forceUpdate, this, &WeatherEngine::forceUpdate);
                qCDebug(WEATHER) << "Ion no longer used as source:" << ionName;
            } else {
                --(it.value());
            }
        }
    } else {
        qCWarning(WEATHER) << "Could not find ion to remove source for:" << source;
    }
}

/**
 * SLOT: Push out new data to applet
 */
void WeatherEngine::dataUpdated(const QString& source, const Plasma::DataEngine::Data& data)
{
    qCDebug(WEATHER) << "dataUpdated() for:" << source;
    setData(source, data);
}

/**
 * SLOT: Set up each Ion for the first time and get any data
 */
bool WeatherEngine::sourceRequestEvent(const QString &source)
{
    QString ionName;
    IonInterface* ion = ionForSource(source, &ionName);

    if (!ion) {
        qCWarning(WEATHER) << "Could not find ion to request source for:" << source;
        return false;
    }

    // track used ions
    QHash<QString, int>::Iterator it = m_ionUsage.find(ionName);
    if (it == m_ionUsage.end()) {
        m_ionUsage.insert(ionName, 1);
        connect(ion, &IonInterface::forceUpdate, this, &WeatherEngine::forceUpdate);
        qCDebug(WEATHER) << "Ion now used as source:" << ionName;
    } else {
        ++(*it);
    }

    // we should connect to the ion anyway, even if the network
    // is down. when it comes up again, then it will be refreshed
    ion->connectSource(source, this);

    qCDebug(WEATHER) << "sourceRequestEvent(): Network is: " << m_networkConfigurationManager.isOnline();
    if (!m_networkConfigurationManager.isOnline()) {
        setData(source, Data());
        return true;
    }

    if (!containerForSource(source)) {
        // it is an async reply, we need to set up the data anyways
        setData(source, Data());
    }

    return true;
}

/**
 * SLOT: update the Applet with new data from all ions loaded.
 */
bool WeatherEngine::updateSourceEvent(const QString& source)
{
    qCDebug(WEATHER) << "updateSourceEvent(): Network is: " << m_networkConfigurationManager.isOnline();

    if (!m_networkConfigurationManager.isOnline()) {
        return false;
    }

    IonInterface *ion = ionForSource(source);
    if (!ion) {
        qCWarning(WEATHER) << "Could not find ion to update source for:" << source;
        return false;
    }

    return ion->updateSourceEvent(source);
}

void WeatherEngine::onOnlineStateChanged(bool isOnline)
{
    if (isOnline) {
        qCDebug(WEATHER) << "starting m_reconnectTimer";
        // allow the network to settle down and actually come up
        m_reconnectTimer.start(1000);
    } else {
        m_reconnectTimer.stop();
    }
}

void WeatherEngine::startReconnect()
{
    for(QHash<QString, int>::ConstIterator it = m_ionUsage.constBegin(); it != m_ionUsage.constEnd(); ++it) {
        const QString& ionName = it.key();
        IonInterface * ion = qobject_cast<IonInterface *>(dataEngine(ionName));

        if (ion) {
            qCDebug(WEATHER) << "Resetting ion" << ion;
            ion->reset();
        } else {
            qCWarning(WEATHER) << "Could not find ion to reset:" << ionName;
        }
    }
}

void WeatherEngine::forceUpdate(IonInterface *ion, const QString &source)
{
    Q_UNUSED(ion);
    Plasma::DataContainer *container = containerForSource(source);
    if (container) {
        qCDebug(WEATHER) << "immediate update of" << source;
        container->forceImmediateUpdate();
    } else {
        qCWarning(WEATHER) << "inexplicable failure of" << source;
    }
}

IonInterface* WeatherEngine::ionForSource(const QString& source, QString* ionName)
{
    const int offset = source.indexOf(QLatin1Char('|'));

    if (offset < 1) {
        return nullptr;
    }

    const QString name = source.left(offset);

    IonInterface* result = qobject_cast<IonInterface *>(dataEngine(name));

    if (result && ionName) {
        *ionName = name;
    }

    return result;
}


K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(weather, WeatherEngine, "plasma-dataengine-weather.json")

#include "weatherengine.moc"
