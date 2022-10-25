/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "locationupdater.h"

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
    auto mode = group.readEntry(QStringLiteral("Mode"), 0);
    if (m_adaptor->running() && mode == 0) {
        if (!m_locator) {
            m_locator = new ColorCorrect::Geolocator(this);
            connect(m_locator, &ColorCorrect::Geolocator::locationChanged, this, &LocationUpdater::sendLocation);
        }
    } else {
        delete m_locator;
        m_locator = nullptr;
        // if automatic location isn't enabled, there's no need to keep running
        // Night Color KCM will enable us again if user changes to automatic
        disableSelf();
    }
}

void LocationUpdater::sendLocation(double latitude, double longitude)
{
    m_adaptor->sendAutoLocationUpdate(latitude, longitude);
}

void LocationUpdater::disableSelf()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusMessage unloadMsg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kded5"),
                                                            QStringLiteral("/kded"),
                                                            QStringLiteral("org.kde.kded5"),
                                                            QStringLiteral("unloadModule"));
    unloadMsg.setArguments({QVariant(QStringLiteral("colorcorrectlocationupdater"))});
    dbus.call(unloadMsg, QDBus::NoBlock);
}

#include "locationupdater.moc"
