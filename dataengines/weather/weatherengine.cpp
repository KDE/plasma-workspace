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

#include <QCoreApplication>
#include <QTimer>

#include <QDebug>
#include <KLocale>
#include <KSycoca>

#include <Plasma/DataEngineManager>
#include <Plasma/DataContainer>

#include "ions/ion.h"

// Constructor
WeatherEngine::WeatherEngine(QObject *parent, const QVariantList& args)
        :  Plasma::DataEngine(parent, args),
           m_networkAvailable(false)
{
    Q_UNUSED(args)

    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, SIGNAL(timeout()), this, SLOT(startReconnect()));

    // Globally notify all plugins to remove their sources (and unload plugin)
    connect(this, SIGNAL(sourceRemoved(QString)), this, SLOT(removeIonSource(QString)));
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(unloadIons()));
}

// Destructor
WeatherEngine::~WeatherEngine()
{
    // Cleanup all private data.
    unloadIons();
}

/**
 * Loads an ion plugin given a plugin name found via KService.
 */
Plasma::DataEngine *WeatherEngine::loadIon(const QString& plugName)
{
    KPluginInfo foundPlugin;

    foreach(const KPluginInfo &info, Plasma::DataEngineManager::listEngineInfo("weatherengine")) {
        if (info.pluginName() == plugName) {
            foundPlugin = info;
            break;
        }
    }

    if (!foundPlugin.isValid()) {
        return NULL;
    }

    // Load the Ion plugin, store it into a QMap to handle multiple ions.
    Plasma::DataEngine *ion = Plasma::DataEngineManager::self()->loadEngine(foundPlugin.pluginName());
    ion->setObjectName(plugName);
    connect(ion, SIGNAL(sourceAdded(QString)), this, SLOT(newIonSource(QString)));
    connect(ion, SIGNAL(forceUpdate(IonInterface*,QString)), this, SLOT(forceUpdate(IonInterface*,QString)));

    m_ions << plugName;

    return ion;
}

/* FIXME: Q_PROPERTY functions to update the list of available plugins */

/**
 * Unload an Ion plugin given a Ion plugin name.
 */
void WeatherEngine::unloadIon(const QString &name)
{
    Plasma::DataEngineManager::self()->unloadEngine(name);
    m_ions.removeOne(name);
}

void WeatherEngine::init()
{
    // Get the list of available plugins but don't load them
    Solid::Networking::Status status = Solid::Networking::status();
    m_networkAvailable = (status == Solid::Networking::Connected ||
                             status == Solid::Networking::Unknown);
    connect(Solid::Networking::notifier(), SIGNAL(statusChanged(Solid::Networking::Status)),
            this, SLOT(networkStatusChanged(Solid::Networking::Status)));
    connect(KSycoca::self(), SIGNAL(databaseChanged(QStringList)), this, SLOT(updateIonList()));

    updateIonList();
    qDebug() << "init()";
}

void WeatherEngine::updateIonList(const QStringList &changedResources)
{
    if (changedResources.isEmpty() || changedResources.contains("services")) {
        removeAllData("ions");
        foreach (const KPluginInfo &info, Plasma::DataEngineManager::listEngineInfo("weatherengine")) {
            setData("ions", info.pluginName(),
                    QString("%1|%2").arg(info.property("Name").toString()).arg(info.pluginName()));
        }
    }
}

/**
 * SLOT: Get data from a new source
 */
void WeatherEngine::newIonSource(const QString& source)
{
    IonInterface *ion = qobject_cast<IonInterface*>(sender());

    if (!ion) {
        return;
    }

    qDebug() << "newIonSource()";
    ion->connectSource(source, this);
}

/**
 * SLOT: Remove the datasource from the ion and unload plugin if needed
 */
void WeatherEngine::removeIonSource(const QString& source)
{
    IonInterface *ion = ionForSource(source);
    if (ion) {
        ion->removeSource(source);
        // If plugin has no more sources let's unload the plugin
        if (ion->isEmpty()) {
            unloadIon(ionNameForSource(source));
        }
    }
    qDebug() << "removeIonSource()";
}

/**
 * SLOT: Push out new data to applet
 */
void WeatherEngine::dataUpdated(const QString& source, Plasma::DataEngine::Data data)
{
    qDebug() << "dataUpdated()";
    setData(source, data);
}

void WeatherEngine::unloadIons()
{
    foreach (const QString &ion, m_ions) {
        Plasma::DataEngineManager::self()->unloadEngine(ion);
    }

    m_ions.clear();
}

/**
 * SLOT: Set up each Ion for the first time and get any data
 */
bool WeatherEngine::sourceRequestEvent(const QString &source)
{
    Plasma::DataEngine *ion = ionForSource(source);

    if (!ion) {
        ion = loadIon(ionNameForSource(source));
        if (!ion) {
            return false;
        }
    }

    // we should connect to the ion anyway, even if the network
    // is down. when it comes up again, then it will be refreshed
    ion->connectSource(source, this);

    qDebug() << "sourceRequestEvent(): Network is: " << m_networkAvailable;
    if (!m_networkAvailable) {
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
    IonInterface *ion = ionForSource(source);
    if (!ion) {
        return false;
    }

    qDebug() << "updateSourceEvent(): Network is: " << m_networkAvailable;
    if (!m_networkAvailable) {
        return false;
    }

    return ion->updateSourceEvent(source);
}

void WeatherEngine::networkStatusChanged(Solid::Networking::Status status)
{
    qDebug();
    m_networkAvailable = status == Solid::Networking::Connected || status == Solid::Networking::Unknown;
    if (m_networkAvailable) {
        // allow the network to settle down and actually come up
        m_reconnectTimer.start(5000);
    }
}

void WeatherEngine::startReconnect()
{
    foreach (const QString &i, m_ions) {
        IonInterface * ion = qobject_cast<IonInterface *>(Plasma::DataEngineManager::self()->engine(i));
        qDebug() << "resetting" << ion;
        if (ion) {
            ion->reset();
        }
    }
}

void WeatherEngine::forceUpdate(IonInterface *i, const QString &source)
{
    const QString actualSource(i->pluginName() + '|' + source);
    Plasma::DataContainer *container = containerForSource(source);
    if (container) {
        qDebug() << "immediate update of" << source;
        container->forceImmediateUpdate();
    } else {
        qDebug() << "innexplicable failure of" << source;
    }
}

IonInterface* WeatherEngine::ionForSource(const QString& name) const
{
    int offset = name.indexOf('|');

    if (offset < 1) {
        return NULL;
    }

    QString ionName = name.left(offset);
    return qobject_cast<IonInterface *>(Plasma::DataEngineManager::self()->engine(ionName));
}

QString WeatherEngine::ionNameForSource(const QString& source) const
{
    int offset = source.indexOf('|');
    if (offset < 1) {
        return QString();
    }

    return QString(source.left(offset));
}

#include "weatherengine.moc"
