/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "unmountaction.h"

#include <QString>

#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/PortableMediaPlayer>

UnmountAction::UnmountAction(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent)
    : ActionInterface(storageInfo, parent)
    , m_stateInfo(stateInfo)
{
    m_hasStorageAccess = false;
    m_isRoot = false;

    if (m_storageInfo->device().is<Solid::StorageAccess>()) {
        const Solid::StorageAccess *storageaccess = m_storageInfo->device().as<Solid::StorageAccess>();
        if (storageaccess) {
            m_hasStorageAccess = true;
            m_isRoot = storageaccess->filePath() == u"/";
        }
    }

    connect(m_stateInfo.get(), &StateInfo::stateChanged, this, &UnmountAction::updateIsValid);
}

UnmountAction::~UnmountAction() = default;

QString UnmountAction::name() const
{
    return QStringLiteral("Unmount");
}

QString UnmountAction::icon() const
{
    return {};
}

QString UnmountAction::text() const
{
    return {};
}

bool UnmountAction::isValid() const
{
    return m_hasStorageAccess && m_storageInfo->isRemovable() && !m_isRoot && m_stateInfo->isMounted();
}

void UnmountAction::triggered()
{
    Solid::Device device = m_storageInfo->device();
    if (device.is<Solid::OpticalDisc>()) {
        auto *drive = device.as<Solid::OpticalDrive>();
        if (!drive) {
            drive = device.parent().as<Solid::OpticalDrive>();
        }
        if (drive) {
            drive->eject();
        }
        return;
    }

    auto *access = device.as<Solid::StorageAccess>();
    if (access && access->isAccessible()) {
        access->teardown();
    }
}

void UnmountAction::updateIsValid(const QString &udi)
{
    Q_UNUSED(udi);

    Q_EMIT isValidChanged(name(), isValid());
}

#include "moc_unmountaction.cpp"
