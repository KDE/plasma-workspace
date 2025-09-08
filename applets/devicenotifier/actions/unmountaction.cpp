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

UnmountAction::UnmountAction(const QString &udi, QObject *parent)
    : ActionInterface(udi, parent)
    , m_stateMonitor(DevicesStateMonitor::instance())
{
    Solid::Device device(m_udi);

    m_hasStorageAccess = false;
    m_isRoot = false;

    if (device.is<Solid::StorageAccess>()) {
        auto *storageaccess = device.as<Solid::StorageAccess>();
        if (storageaccess) {
            m_hasStorageAccess = true;
            m_isRoot = storageaccess->filePath() == u"/";
        }
    }

    connect(m_stateMonitor.get(), &DevicesStateMonitor::stateChanged, this, &UnmountAction::updateIsValid);
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
    return m_hasStorageAccess && m_stateMonitor->isRemovable(m_udi) && !m_isRoot && m_stateMonitor->isMounted(m_udi);
}

void UnmountAction::triggered()
{
    Solid::Device device(m_udi);
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
    if (udi != m_udi) {
        return;
    }

    Q_EMIT isValidChanged(name(), isValid());
}

#include "moc_unmountaction.cpp"
