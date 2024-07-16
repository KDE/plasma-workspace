/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "devicestatemonitor_p.h"

#include "devicenotifier_debug.h"

#include <QTimer>

#include <Solid/Camera>
#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/GenericInterface>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/PortableMediaPlayer>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>

DevicesStateMonitor::DevicesStateMonitor(QObject *parent)
    : QObject(parent)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor created";
}

DevicesStateMonitor::~DevicesStateMonitor()
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor removed";
}

std::shared_ptr<DevicesStateMonitor> DevicesStateMonitor::instance()
{
    static std::weak_ptr<DevicesStateMonitor> s_clip;
    if (s_clip.expired()) {
        std::shared_ptr<DevicesStateMonitor> ptr{new DevicesStateMonitor};
        s_clip = ptr;
        return ptr;
    }
    return s_clip.lock();
}

void DevicesStateMonitor::addMonitoringDevice(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : addDevice signal arrived for " << udi;
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << "is already monitoring. Don't add another one";
        return;
    }

    auto it = m_devicesStates.emplace(udi, std::make_pair(false, Idle));

    Solid::Device device(udi);
    if (device.is<Solid::OpticalDisc>()) {
        Solid::OpticalDrive *drive = getAncestorAs<Solid::OpticalDrive>(device);
        if (drive) {
            connect(drive, &Solid::OpticalDrive::ejectRequested, this, &DevicesStateMonitor::setUnmountingState);
            connect(drive, &Solid::OpticalDrive::ejectDone, this, &DevicesStateMonitor::setIdleState);
        }
    } else if (device.is<Solid::StorageVolume>()) {
        Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        if (access) {
            connect(access, &Solid::StorageAccess::setupRequested, this, &DevicesStateMonitor::setMountingState);
            connect(access, &Solid::StorageAccess::setupDone, this, &DevicesStateMonitor::setIdleState);
            connect(access, &Solid::StorageAccess::teardownRequested, this, &DevicesStateMonitor::setUnmountingState);
            connect(access, &Solid::StorageAccess::teardownDone, this, &DevicesStateMonitor::setIdleState);

            qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " state : " << access->isAccessible();
            it->first = access->isAccessible();
        }
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << "successfully added";
    Q_EMIT stateChanged(udi);
}

void DevicesStateMonitor::removeMonitoringDevice(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Remove Signal arrived for " << udi;
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        m_devicesStates.erase(it);

        Solid::Device device(udi);
        if (device.is<Solid::StorageVolume>()) {
            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
            if (access) {
                disconnect(access, nullptr, this, nullptr);
            }
        } else if (device.is<Solid::OpticalDisc>()) {
            Solid::OpticalDrive *drive = getAncestorAs<Solid::OpticalDrive>(device);
            if (drive) {
                disconnect(drive, nullptr, this, nullptr);
            }
        }

        qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " successfully removed";
    } else {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " was not monitored";
    }
}

bool DevicesStateMonitor::isRemovable(const QString &udi) const
{
    Solid::Device device(udi);
    if (device.is<Solid::StorageDrive>()) {
        Solid::StorageDrive *storagedrive = device.as<Solid::StorageDrive>();
        if (storagedrive) {
            return storagedrive->isRemovable();
        }
    }

    Solid::StorageDrive *drive = getAncestorAs<Solid::StorageDrive>(device);
    if (drive) {
        // remove check for isHotpluggable() when plasmoids are changed to check for both properties
        return drive->isRemovable() || drive->isHotpluggable();
    }

    if (device.is<Solid::Camera>()) {
        Solid::Camera *camera = device.as<Solid::Camera>();
        if (camera) {
            return true;
        }
    }

    if (device.is<Solid::PortableMediaPlayer>()) {
        Solid::PortableMediaPlayer *mediaplayer = device.as<Solid::PortableMediaPlayer>();
        if (!mediaplayer) {
            return true;
        }
    }
    return false;
}

bool DevicesStateMonitor::isMounted(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->first;
    }
    return false;
}

DevicesStateMonitor::OperationResult DevicesStateMonitor::getOperationResult(const QString &udi) const
{
    if (auto it = m_devicesStates.constFind(udi); it != m_devicesStates.constEnd()) {
        return it->second;
    }
    return Idle;
}

void DevicesStateMonitor::setMountingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " state changed";
    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        it->second = Working;
        Q_EMIT stateChanged(udi);
    }
}

void DevicesStateMonitor::setUnmountingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " state changed";
    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        it->second = Working;
        Q_EMIT stateChanged(udi);
    }
}

void DevicesStateMonitor::setIdleState(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
    Solid::Device device(udi);

    if (!device.isValid()) {
        return;
    }

    if (auto it = m_devicesStates.find(udi); it != m_devicesStates.end()) {
        if (error == Solid::NoError) {
            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
            it->first = access->isAccessible();
            qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " state changed : " << access->isAccessible();
            it->second = Successful;
        } else {
            qCDebug(APPLETS::DEVICENOTIFIER) << "Devices State Monitor : Device " << udi << " Error! state don't changed. Error data: " << errorData.toString();
            it->second = Unsuccessful;
        }
        Q_EMIT stateChanged(udi);

        // update state after some time to Idle
        QTimer *stateTimer = new QTimer(this);
        stateTimer->setSingleShot(true);
        stateTimer->setInterval(std::chrono::seconds(2));
        stateTimer->callOnTimeout([this, device, stateTimer]() {
            if (auto it = m_devicesStates.find(device.udi()); it != m_devicesStates.end() && device.isValid()) {
                it->second = Idle;
                Q_EMIT stateChanged(device.udi());
            }
            stateTimer->deleteLater();
        });
        stateTimer->start();
    }
}

#include "moc_devicestatemonitor_p.cpp"
