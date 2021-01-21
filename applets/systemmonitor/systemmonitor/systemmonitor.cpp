/***************************************************************************
 *   Copyright (C) 2019 Marco Martin <mart@kde.org>                        *
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

#include "systemmonitor.h"

#include <QDebug>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QWindow>
#include <QtQml>

#include <faces/SensorFaceController.h>
#include <sensors/SensorQuery.h>

#include <KConfigLoader>
#include <KDeclarative/QmlObjectSharedEngine>
#include <KLocalizedString>

SystemMonitor::SystemMonitor(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args)
{
    setHasConfigurationInterface(true);

    // Don't set the preset right now as we can't write on the config here because we don't have a Corona yet
    if (args.count() > 2 && args.mid(3).length() > 0) {
        const QString preset = args.mid(3).constFirst().toString();
        if (preset.length() > 0) {
            m_pendingStartupPreset = preset;
        }
    }
}

SystemMonitor::~SystemMonitor() = default;

void SystemMonitor::init()
{
    configChanged();

    // NOTE: taking the pluginId this way, we take it from the child applet (cpu monitor, memory, whatever) rather than the parent fallback applet
    // (systemmonitor)
    const QString pluginId = KPluginMetaData(kPackage().path() + QStringLiteral("metadata.desktop")).pluginId();

    // FIXME HACK: better way to get the engine At least AppletQuickItem should have an engine() getter
    KDeclarative::QmlObjectSharedEngine *qmlObject = new KDeclarative::QmlObjectSharedEngine();
    KConfigGroup cg = config();
    m_sensorFaceController = new KSysGuard::SensorFaceController(cg, qmlObject->engine());
    qmlObject->deleteLater();

    if (!m_pendingStartupPreset.isNull()) {
        m_sensorFaceController->loadPreset(m_pendingStartupPreset);
    } else {
        // Take it from the config, which is *not* accessible from plasmoid.config as is not in config.xml
        const QString preset = config().readEntry("CurrentPreset", pluginId);
        m_sensorFaceController->loadPreset(preset);
    }
}

KSysGuard::SensorFaceController *SystemMonitor::faceController() const
{
    return m_sensorFaceController;
}

void SystemMonitor::configChanged()
{
    if (m_sensorFaceController) {
        m_sensorFaceController->reloadConfig();
    }
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(systemmonitor, SystemMonitor, "metadata.json")

#include "systemmonitor.moc"
