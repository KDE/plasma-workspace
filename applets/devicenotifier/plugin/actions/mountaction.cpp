/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mountaction.h"

#include <devicenotifier_debug.h>

#include <Solid/Camera>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/PortableMediaPlayer>

#include <KLocalizedString>

MountAction::MountAction(const QString &udi, QObject *parent)
    : ActionInterface(udi, parent)
    , m_stateMonitor(DevicesStateMonitor::instance())
{
    Solid::Device device(udi);

    QStringList supportedProtocols;

    if (device.is<Solid::Camera>()) {
        Solid::Camera *camera = device.as<Solid::Camera>();
        if (camera) {
            supportedProtocols = camera->supportedProtocols();
        }
    }

    if (device.is<Solid::PortableMediaPlayer>()) {
        Solid::PortableMediaPlayer *mediaplayer = device.as<Solid::PortableMediaPlayer>();
        if (mediaplayer) {
            supportedProtocols = mediaplayer->supportedProtocols();
        }
    }

    m_supportsMTP = supportedProtocols.contains(QLatin1String("mtp"));

    connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &MountAction::updateIsValid);
}

MountAction::~MountAction()
{
}

QString MountAction::name() const
{
    return QStringLiteral("Mount");
}

void MountAction::triggered()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "MountAction: Triggered! Begin mounting";

    Solid::Device device(m_udi);

    if (device.is<Solid::StorageAccess>()) {
        Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        if (access && !access->isAccessible()) {
            access->setup();
        }
    }
}

bool MountAction::isValid() const
{
    return m_stateMonitor->isRemovable(m_udi) && !m_stateMonitor->isMounted(m_udi) && !m_supportsMTP;
}

void MountAction::updateIsValid(const QString &udi)
{
    if (udi != m_udi) {
        return;
    }
    Q_EMIT isValidChanged(name(), isValid());
}

QString MountAction::icon() const
{
    return QStringLiteral("media-mount");
}

QString MountAction::text() const
{
    return i18n("Mount");
}

#include "moc_mountaction.cpp"
