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
    , m_supportsMTP(false)
    , m_hasStorageAccess(false)
    , m_stateMonitor(DevicesStateMonitor::instance())
{
    Solid::Device device(udi);

    QStringList supportedProtocols;

    if (device.is<Solid::Camera>()) {
        auto *camera = device.as<Solid::Camera>();
        if (camera) {
            supportedProtocols = camera->supportedProtocols();
        }
    }

    if (device.is<Solid::PortableMediaPlayer>()) {
        auto *mediaplayer = device.as<Solid::PortableMediaPlayer>();
        if (mediaplayer) {
            supportedProtocols = mediaplayer->supportedProtocols();
        }
    }

    m_supportsMTP = supportedProtocols.contains(QLatin1String("mtp"));

    // It's possible for there to be no StorageAccess (e.g. MTP devices don't have one)
    if (device.is<Solid::StorageAccess>()) {
        auto *access = device.as<Solid::StorageAccess>();
        if (access) {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Mount action: have storage access";
            m_hasStorageAccess = true;
        }
    }

    connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &MountAction::updateIsValid);
}

MountAction::~MountAction() = default;

QString MountAction::name() const
{
    return QStringLiteral("Mount");
}

void MountAction::triggered()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "MountAction: Triggered! Begin mounting";

    Solid::Device device(m_udi);

    if (device.is<Solid::StorageAccess>()) {
        auto *access = device.as<Solid::StorageAccess>();
        if (access && !access->isAccessible()) {
            access->setup();
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
    Solid::Device device(m_udi);
    if (device.is<Solid::StorageAccess>()) {
        auto *access = device.as<Solid::StorageAccess>();
        if (access && access->canCheck() && !m_stateMonitor->isChecked(m_udi)) {
            return i18nc("@action:button Mount a disk without verifying for errors", "Mount without verifying");
        }
    }
    return i18n("Mount");
}

#include "moc_mountaction.cpp"
