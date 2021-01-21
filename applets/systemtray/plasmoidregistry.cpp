/***************************************************************************
 *   Copyright (C) 2015 Marco Martin <mart@kde.org>                        *
 *   Copyright (C) 2020 Konrad Materka <materka@gmail.com>                 *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "plasmoidregistry.h"
#include "debug.h"

#include "dbusserviceobserver.h"
#include "systemtraysettings.h"

#include <KPluginMetaData>
#include <Plasma/PluginLoader>

#include <QDBusConnection>

PlasmoidRegistry::PlasmoidRegistry(QPointer<SystemTraySettings> settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_dbusObserver(new DBusServiceObserver(settings, this))
{
    connect(m_dbusObserver, &DBusServiceObserver::serviceStarted, this, &PlasmoidRegistry::plasmoidEnabled);
    connect(m_dbusObserver, &DBusServiceObserver::serviceStopped, this, &PlasmoidRegistry::plasmoidStopped);
}

void PlasmoidRegistry::init()
{
    QDBusConnection::sessionBus().connect(QString(),
                                          QStringLiteral("/KPackage/Plasma/Applet"),
                                          QStringLiteral("org.kde.plasma.kpackage"),
                                          QStringLiteral("packageInstalled"),
                                          this,
                                          SLOT(packageInstalled(QString)));
    QDBusConnection::sessionBus().connect(QString(),
                                          QStringLiteral("/KPackage/Plasma/Applet"),
                                          QStringLiteral("org.kde.plasma.kpackage"),
                                          QStringLiteral("packageUpdated"),
                                          this,
                                          SLOT(packageInstalled(QString)));
    QDBusConnection::sessionBus().connect(QString(),
                                          QStringLiteral("/KPackage/Plasma/Applet"),
                                          QStringLiteral("org.kde.plasma.kpackage"),
                                          QStringLiteral("packageUninstalled"),
                                          this,
                                          SLOT(packageUninstalled(QString)));

    connect(m_settings, &SystemTraySettings::enabledPluginsChanged, this, &PlasmoidRegistry::onEnabledPluginsChanged);

    for (const auto &info : Plasma::PluginLoader::self()->listAppletMetaData(QString())) {
        registerPlugin(info);
    }

    m_dbusObserver->initDBusActivatables();

    sanitizeSettings();
}

QMap<QString, KPluginMetaData> PlasmoidRegistry::systemTrayApplets()
{
    return m_systrayApplets;
}

bool PlasmoidRegistry::isSystemTrayApplet(const QString &pluginId)
{
    return m_systrayApplets.contains(pluginId);
}

void PlasmoidRegistry::onEnabledPluginsChanged(const QStringList &enabledPlugins, const QStringList &disabledPlugins)
{
    for (const QString &pluginId : enabledPlugins) {
        if (m_systrayApplets.contains(pluginId) && !m_dbusObserver->isDBusActivable(pluginId)) {
            emit plasmoidEnabled(pluginId);
        }
    }
    for (const QString &pluginId : disabledPlugins) {
        if (m_systrayApplets.contains(pluginId)) {
            emit plasmoidDisabled(pluginId);
        }
    }
}

void PlasmoidRegistry::packageInstalled(const QString &pluginId)
{
    qCDebug(SYSTEM_TRAY) << "New package installed" << pluginId;

    if (m_systrayApplets.contains(pluginId)) {
        if (m_settings->isEnabledPlugin(pluginId) && !m_dbusObserver->isDBusActivable(pluginId)) {
            // restart plasmoid
            emit plasmoidStopped(pluginId);
            emit plasmoidEnabled(pluginId);
        }
        return;
    }

    for (const auto &info : Plasma::PluginLoader::self()->listAppletMetaData(QString())) {
        if (info.pluginId() == pluginId) {
            registerPlugin(info);
        }
    }
}

void PlasmoidRegistry::packageUninstalled(const QString &pluginId)
{
    qCDebug(SYSTEM_TRAY) << "Package uninstalled" << pluginId;
    if (m_systrayApplets.contains(pluginId)) {
        unregisterPlugin(pluginId);
    }
}

void PlasmoidRegistry::registerPlugin(const KPluginMetaData &pluginMetaData)
{
    if (!pluginMetaData.isValid() || pluginMetaData.value(QStringLiteral("X-Plasma-NotificationArea")) != "true") {
        return;
    }

    const QString &pluginId = pluginMetaData.pluginId();

    m_systrayApplets[pluginId] = pluginMetaData;
    m_dbusObserver->registerPlugin(pluginMetaData);

    emit pluginRegistered(pluginMetaData);

    // add plasmoid if is both not enabled explicitly and not already known
    if (pluginMetaData.isEnabledByDefault()) {
        const QString &candidate = pluginMetaData.pluginId();
        if (!m_settings->isKnownPlugin(candidate)) {
            m_settings->addKnownPlugin(candidate);
            if (!m_settings->isEnabledPlugin(candidate)) {
                m_settings->addEnabledPlugin(candidate);
            }
        }
    }

    if (m_settings->isEnabledPlugin(pluginId) && !m_dbusObserver->isDBusActivable(pluginId)) {
        emit plasmoidEnabled(pluginId);
    }
}

void PlasmoidRegistry::unregisterPlugin(const QString &pluginId)
{
    emit pluginUnregistered(pluginId);

    m_dbusObserver->unregisterPlugin(pluginId);
    m_systrayApplets.remove(pluginId);

    m_settings->cleanupPlugin(pluginId);
}

void PlasmoidRegistry::sanitizeSettings()
{
    // remove all no longer available in the system (e.g. uninstalled)
    const QStringList knownPlugins = m_settings->knownPlugins();
    for (const QString &pluginId : knownPlugins) {
        if (!m_systrayApplets.contains(pluginId)) {
            m_settings->removeKnownPlugin(pluginId);
        }
    }

    const QStringList enabledPlugins = m_settings->enabledPlugins();
    for (const QString &pluginId : enabledPlugins) {
        if (!m_systrayApplets.contains(pluginId)) {
            m_settings->removeEnabledPlugin(pluginId);
        }
    }
}
