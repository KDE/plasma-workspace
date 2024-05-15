/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "systemmonitor.h"

#include <QDebug>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QWindow>

#include <sensors/SensorQuery.h>

#include <KConfigLoader>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KService>
#include <PlasmaQuick/SharedQmlEngine>

SystemMonitor::SystemMonitor(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
{
    setHasConfigurationInterface(true);
}

SystemMonitor::~SystemMonitor() = default;

void SystemMonitor::init()
{
    configChanged();

    // FIXME HACK: better way to get the engine At least AppletQuickItem should have an engine() getter
    auto qmlObject = new PlasmaQuick::SharedQmlEngine();
    KConfigGroup cg = config();
    m_sensorFaceController = new KSysGuard::SensorFaceController(cg, qmlObject->engine().get(), new QQmlEngine(this));
    qmlObject->deleteLater();

    // NOTE: taking the pluginId from the child applet (cpu monitor, memory, whatever) is done implicitly by not embedding metadata in this applet
    const QString preset = config().readEntry("CurrentPreset", pluginMetaData().pluginId());
    // We have initialized our preset, subsequent calls should use the root-plugin id
    config().writeEntry("CurrentPreset", "org.kde.plasma.systemmonitor");
    m_sensorFaceController->loadPreset(preset);
}

KSysGuard::SensorFaceController *SystemMonitor::faceController() const
{
    return m_sensorFaceController;
}

KSysGuard::SensorFaceController *SystemMonitor::workaroundController(QQuickItem *context) const
{
    KConfigGroup cg = config();
    return new KSysGuard::SensorFaceController(cg, qmlEngine(context), new QQmlEngine(context));
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

K_PLUGIN_CLASS(SystemMonitor)

#include "systemmonitor.moc"
