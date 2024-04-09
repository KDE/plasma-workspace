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
#include "../geolocator.h"

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
        if (!m_locator) {
            m_locator = new ColorCorrect::Geolocator(this);
            qCInfo(LOCATIONUPDATER) << "Geolocator started";
            connect(m_locator, &ColorCorrect::Geolocator::locationChanged, this, &LocationUpdater::sendLocation);
        }
    } else {
        delete m_locator;
        m_locator = nullptr;
        // if automatic location isn't enabled, there's no need to keep running
        // Night Light KCM will enable us again if user changes to automatic
        disableSelf();
        qCInfo(LOCATIONUPDATER) << "Geolocator stopped";
    }
}

void LocationUpdater::sendLocation(double latitude, double longitude)
{
    m_adaptor->sendAutoLocationUpdate(latitude, longitude);
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
