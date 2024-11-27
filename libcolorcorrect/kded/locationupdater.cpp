/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "locationupdater.h"
#include "locationupdater_debug.h"

#include <QDBusConnection>
#include <QDBusMessage>

#include <KPluginFactory>

#include "../compositorcoloradaptor.h"

K_PLUGIN_CLASS_WITH_JSON(LocationUpdater, "colorcorrectlocationupdater.json")

LocationUpdater::LocationUpdater(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_adaptor(new ColorCorrect::CompositorAdaptor(this))
{
    m_configWatcher = KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("kwinrc")));
    connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this, &LocationUpdater::resetLocator);
    connect(m_adaptor, &ColorCorrect::CompositorAdaptor::runningChanged, this, &LocationUpdater::resetLocator);
    resetLocator();
}

void LocationUpdater::resetLocator()
{
    KConfigGroup group(m_configWatcher->config(), QStringLiteral("NightColor"));
    const bool enabled = group.readEntry(QStringLiteral("Active"), false);
    const QString mode = group.readEntry(QStringLiteral("Mode"), QStringLiteral("Automatic"));
    if (m_adaptor->running() && enabled && mode == QStringLiteral("Automatic")) {
        if (!m_positionSource) {
            m_positionSource = QGeoPositionInfoSource::createDefaultSource(this);
            if (!m_positionSource) {
                qCWarning(LOCATIONUPDATER) << "Failed to get a geolocation source";
                return;
            }

            const QGeoPositionInfo lastPosition = m_positionSource->lastKnownPosition();
            if (lastPosition.isValid()) {
                m_adaptor->sendAutoLocationUpdate(lastPosition.coordinate().latitude(), lastPosition.coordinate().longitude());
            }

            connect(m_positionSource, &QGeoPositionInfoSource::positionUpdated, this, [this](const QGeoPositionInfo &position) {
                m_adaptor->sendAutoLocationUpdate(position.coordinate().latitude(), position.coordinate().longitude());
            });
            m_positionSource->startUpdates();
        }
    } else {
        delete m_positionSource;
        m_positionSource = nullptr;
        // if automatic location isn't enabled, there's no need to keep running
        // Night Light KCM will enable us again if user changes to automatic
        disableSelf();
        qCInfo(LOCATIONUPDATER) << "Geolocator stopped";
    }
}

void LocationUpdater::disableSelf()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusMessage unloadMsg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kded6"),
                                                            QStringLiteral("/kded"),
                                                            QStringLiteral("org.kde.kded6"),
                                                            QStringLiteral("unloadModule"));
    unloadMsg.setArguments({QVariant(QStringLiteral("colorcorrectlocationupdater"))});
    dbus.call(unloadMsg, QDBus::NoBlock);
}

#include "locationupdater.moc"

#include "moc_locationupdater.cpp"
