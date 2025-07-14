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

MountAction::MountAction(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent)
    : ActionInterface(storageInfo, parent)
    , m_supportsMTP(false)
    , m_hasStorageAccess(false)
    , m_stateInfo(stateInfo)
{
    const Solid::Device &device = m_storageInfo->device();

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

    connect(m_stateInfo.get(), &StateInfo::stateChanged, this, &MountAction::updateIsValid);
}

MountAction::~MountAction() = default;

QString MountAction::name() const
{
    return QStringLiteral("Mount");
}

void MountAction::triggered()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "MountAction: Triggered! Begin mounting";

    Solid::Device device = m_storageInfo->device();

    if (device.is<Solid::StorageAccess>()) {
        auto *access = device.as<Solid::StorageAccess>();
        if (access && !access->isAccessible()) {
            access->setup();
        }
    }
}

bool MountAction::isValid() const
{
    return m_hasStorageAccess && m_storageInfo->isRemovable() && !m_stateInfo->isMounted() && !m_supportsMTP;
}

void MountAction::updateIsValid()
{
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
