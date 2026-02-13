/*
 * SPDX-FileCopyrightText: 2026 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ejectaction.h"

#include <QString>

#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/PortableMediaPlayer>

EjectAction::EjectAction(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent)
    : ActionInterface(storageInfo, stateInfo, parent)
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

    connect(m_stateInfo.get(), &StateInfo::stateChanged, this, &EjectAction::updateIsValid);
}

EjectAction::~EjectAction() = default;

QString EjectAction::name() const
{
    return QStringLiteral("Eject");
}

QString EjectAction::icon() const
{
    return {};
}

QString EjectAction::text() const
{
    return {};
}

bool EjectAction::isValid() const
{
    return m_hasStorageAccess && m_storageInfo->isRemovable() && !m_isRoot && m_stateInfo->isMounted();
}

void EjectAction::triggered()
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
    if (access) {
        access->teardown();
    }
}

void EjectAction::updateIsValid(const QString &udi)
{
    Q_UNUSED(udi);

    Q_EMIT isValidChanged(name(), isValid());
}

#include "moc_ejectaction.cpp"
