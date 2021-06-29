/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KNotifications/KNotificationJobUiDelegate>
#include <KService>

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

KSysGuard::SensorFaceController *SystemMonitor::workaroundController(QQuickItem *context) const
{
    KConfigGroup cg = config();
    return new KSysGuard::SensorFaceController(cg, qmlEngine(context));
}

void SystemMonitor::configChanged()
{
    if (m_sensorFaceController) {
        m_sensorFaceController->reloadConfig();
    }
}

void SystemMonitor::openSystemMonitor()
{
    auto job = new KIO::ApplicationLauncherJob(KService::serviceByDesktopName("org.kde.plasma-systemmonitor"));
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
    job->start();
}

K_PLUGIN_CLASS_WITH_JSON(SystemMonitor, "metadata.json")

#include "systemmonitor.moc"
