/*
    SPDX-FileCopyrightText: 2009 Petri Damsten <damu@iki.fi>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "geolocation.h"

#include <limits.h>

#include <KPluginMetaData>
#include <NetworkManagerQt/Manager>
#include <QDebug>
#include <QNetworkConfigurationManager>

static const char SOURCE[] = "location";

Geolocation::Geolocation(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
    setMinimumPollingInterval(500);
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::networkingEnabledChanged, this, &Geolocation::networkStatusChanged);
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::wirelessEnabledChanged, this, &Geolocation::networkStatusChanged);
    m_updateTimer.setInterval(100);
    m_updateTimer.setSingleShot(true);
    connect(&m_updateTimer, &QTimer::timeout, this, &Geolocation::actuallySetData);
    m_networkChangedTimer.setInterval(100);
    m_networkChangedTimer.setSingleShot(true);
    connect(&m_networkChangedTimer, &QTimer::timeout, this, [this] {
        updatePlugins(GeolocationProvider::NetworkConnected);
    });
    init();
}

void Geolocation::init()
{
    // TODO: should this be delayed even further, e.g. when the source is requested?
    const QVector<KPluginMetaData> offers = KPluginMetaData::findPlugins("plasma/geolocationprovider");
    for (const auto &metaData : offers) {
        auto result = KPluginFactory::instantiatePlugin<GeolocationProvider>(metaData, this);
        if (result) {
            GeolocationProvider *plugin = result.plugin;
            m_plugins << plugin;
            plugin->init(&m_data, &m_accuracy);
            connect(plugin, &GeolocationProvider::updated, this, &Geolocation::pluginUpdated);
            connect(plugin, &GeolocationProvider::availabilityChanged, this, &Geolocation::pluginAvailabilityChanged);
        } else {
            qDebug() << "Failed to load GeolocationProvider:" << metaData.fileName() << result.errorString;
        }
    }
}

Geolocation::~Geolocation()
{
    qDeleteAll(m_plugins);
}

QStringList Geolocation::sources() const
{
    return QStringList() << SOURCE;
}

bool Geolocation::updateSourceEvent(const QString &name)
{
    // qDebug() << name;
    if (name == SOURCE) {
        return updatePlugins(GeolocationProvider::SourceEvent);
    }

    return false;
}

bool Geolocation::updatePlugins(GeolocationProvider::UpdateTriggers triggers)
{
    bool changed = false;

    for (GeolocationProvider *plugin : qAsConst(m_plugins)) {
        changed = plugin->requestUpdate(triggers) || changed;
    }

    if (changed) {
        m_updateTimer.start();
    }

    return changed;
}

bool Geolocation::sourceRequestEvent(const QString &name)
{
    qDebug() << name;
    if (name == SOURCE) {
        updatePlugins(GeolocationProvider::ForcedUpdate);
        setData(SOURCE, m_data);
        return true;
    }

    return false;
}

void Geolocation::networkStatusChanged(bool isOnline)
{
    qDebug() << "network status changed";
    if (isOnline) {
        m_networkChangedTimer.start();
    }
}

void Geolocation::pluginAvailabilityChanged(GeolocationProvider *provider)
{
    m_data.clear();
    m_accuracy.clear();

    provider->requestUpdate(GeolocationProvider::ForcedUpdate);

    bool changed = false;
    for (GeolocationProvider *plugin : qAsConst(m_plugins)) {
        changed = plugin->populateSharedData() || changed;
    }

    if (changed) {
        m_updateTimer.start();
    }
}

void Geolocation::pluginUpdated()
{
    m_updateTimer.start();
}

void Geolocation::actuallySetData()
{
    setData(SOURCE, m_data);
}

K_PLUGIN_CLASS_WITH_JSON(Geolocation, "plasma-dataengine-geolocation.json")

#include "geolocation.moc"
