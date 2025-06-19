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
    , m_supportsMTP(false)
    , m_hasStorageAccess(false)
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

    // It's possible for there to be no StorageAccess (e.g. MTP devices don't have one)
    if (device.is<Solid::StorageAccess>()) {
        Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        if (access) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Mount action: have storage access";
            m_hasStorageAccess = true;
        }
    }

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
            if (!m_stateMonitor->isChecked(m_udi) && access->canCheck()) {
                connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &MountAction::deviceStateChanged);
                access->check();
            } else {
                access->setup();
            }
        }
    }
}

bool MountAction::isValid() const
{
    return m_hasStorageAccess && m_stateMonitor->isRemovable(m_udi) && !m_stateMonitor->isMounted(m_udi) && !m_supportsMTP;
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

void MountAction::deviceStateChanged(const QString &udi)
{
    if (udi != m_udi || m_stateMonitor->getOperationResult(m_udi) != DevicesStateMonitor::CheckDone) {
        return;
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Mount action check done, need repair: " << m_stateMonitor->needRepair(m_udi);
    disconnect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &MountAction::deviceStateChanged);
    if (!m_stateMonitor->needRepair(m_udi)) {
        MountAction::triggered();
    }
}

#include "moc_mountaction.cpp"
