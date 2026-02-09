/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stateinfo.h"

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

StateInfo::StateInfo(const std::shared_ptr<StorageInfo> &storageInfo, QObject *parent)
    : QObject(parent)
    , m_isBusy(false)
    , m_isMounted(false)
    , m_isChecked(false)
    , m_needRepair(false)
    , m_operationInfo(Solid::NoError)
    , m_state(Idle)
    , m_deviceTimeStamp(QDateTime::currentDateTimeUtc())
    , m_storageInfo(storageInfo)
{
    const Solid::Device &device = m_storageInfo->device();

    if (device.is<Solid::OpticalDisc>()) {
        const Solid::OpticalDrive *drive = getAncestorAs<Solid::OpticalDrive>(device);
        if (drive) {
            connect(drive, &Solid::OpticalDrive::ejectRequested, this, &StateInfo::setUnmountingState);
            connect(drive, &Solid::OpticalDrive::ejectDone, this, &StateInfo::setIdleState);
        }
    }

    if (device.is<Solid::StorageVolume>()) {
        const Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        if (access) {
            connect(access, &Solid::StorageAccess::accessibilityChanged, this, &StateInfo::setAccessibilityState);
            connect(access, &Solid::StorageAccess::setupRequested, this, &StateInfo::setMountingState);
            connect(access, &Solid::StorageAccess::setupDone, this, &StateInfo::setIdleState);
            connect(access, &Solid::StorageAccess::teardownRequested, this, &StateInfo::setUnmountingState);
            connect(access, &Solid::StorageAccess::teardownDone, this, &StateInfo::setIdleState);
            if (access->canCheck()) {
                connect(access, &Solid::StorageAccess::checkRequested, this, &StateInfo::setCheckingState);
                connect(access, &Solid::StorageAccess::checkDone, this, &StateInfo::setIdleState);
            }
            if (access->canRepair()) {
                connect(access, &Solid::StorageAccess::repairRequested, this, &StateInfo::setRepairingState);
                connect(access, &Solid::StorageAccess::repairDone, this, &StateInfo::setIdleState);
            }
            qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << device.udi() << " : is mounted : " << access->isAccessible();
            m_isMounted = access->isAccessible();
        }
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << device.udi() << " : created";
}

StateInfo::~StateInfo()
{
    const Solid::Device &device = m_storageInfo->device();

    if (device.is<Solid::StorageVolume>()) {
        const Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
        if (access) {
            disconnect(access, nullptr, this, nullptr);
        }
    } else if (m_storageInfo->device().is<Solid::OpticalDisc>()) {
        const Solid::OpticalDrive *drive = getAncestorAs<Solid::OpticalDrive>(device);
        if (drive) {
            disconnect(drive, nullptr, this, nullptr);
        }
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << device.udi() << " : destroyed";
}

bool StateInfo::isBusy() const
{
    return m_isBusy;
}

bool StateInfo::isMounted() const
{
    return m_isMounted;
}

bool StateInfo::isChecked() const
{
    return m_isChecked;
}

bool StateInfo::needRepair() const
{
    return m_needRepair;
}

bool StateInfo::isSafelyRemovable() const
{
    const Solid::Device &device = m_storageInfo->device();
    if (device.is<Solid::StorageVolume>()) {
        auto drive = getAncestorAs<Solid::StorageDrive>(device);
        if (!drive /* Already removed from elsewhere */ || !drive->isValid()) {
            return true;
        }
        return !drive->isInUse() && (drive->isHotpluggable() || drive->isRemovable());
    }

    auto access = device.as<Solid::StorageAccess>();
    if (access) {
        return !access->isAccessible();
    }
    // If this check fails, the device has been already physically
    // ejected, so no need to say that it is safe to remove it
    return false;
}

QDateTime StateInfo::getDeviceTimeStamp() const
{
    return m_deviceTimeStamp;
}

StateInfo::State StateInfo::getState() const
{
    return m_state;
}

Solid::ErrorType StateInfo::getOperationResult() const
{
    return m_operationResult;
}

QVariant StateInfo::getOperationInfo() const
{
    return m_operationInfo;
}

void StateInfo::setMountingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : state changed to Mounting";

    m_isBusy = true;
    m_state = Mounting;

    Q_EMIT stateChanged(udi);
}

void StateInfo::setUnmountingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : state changed to Unmounting";

    m_isBusy = true;
    m_state = Unmounting;

    Q_EMIT stateChanged(udi);
}

void StateInfo::setCheckingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : state changed to Checking";

    m_isBusy = true;
    m_state = Checking;

    Q_EMIT stateChanged(udi);
}

void StateInfo::setRepairingState(const QString &udi)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : state changed to Repairing";

    m_isBusy = true;
    m_state = Repairing;

    Q_EMIT stateChanged(udi);
}

void StateInfo::setAccessibilityState(bool isAccessible, const QString &udi)
{
    if (m_isMounted != isAccessible) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : accessibility state changed to " << isAccessible;
        m_isMounted = isAccessible;
        Q_EMIT stateChanged(udi);
    }
}

void StateInfo::setNotPresentState(const QString &udi)
{
    // UnmountDone differs from NotPresent because it is triggered by
    // Solid::StorageAccess::teardownDone rather than by external removal.
    // So there is no need to update it if the state is UnmountDone.
    if (m_state != UnmountDone && m_storageInfo->device().udi() == udi) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : state changed to NotPresent";
        m_isBusy = false;
        m_isMounted = false;
        m_state = NotPresent;
        m_operationInfo = Solid::NoError;
        m_operationResult = {};
        Q_EMIT stateChanged(udi);
    }
}

void StateInfo::setIdleState(Solid::ErrorType operationResult, QVariant operationInfo, const QString &udi)
{
    m_isBusy = false;
    m_operationResult = operationResult;
    m_operationInfo = operationInfo;
    qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : Operation result is: " << operationResult << " operation info: " << operationInfo;
    if (m_state == Checking) {
        auto access = m_storageInfo->device().as<Solid::StorageAccess>();
        m_isChecked = true;
        m_needRepair = (operationResult == Solid::NoError) ? !operationInfo.toBool() && access->canRepair() : false;
        qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : check done, need repair : " << m_needRepair;
        m_state = CheckDone;
    } else if (m_state == Repairing) {
        m_needRepair = (operationResult != Solid::NoError);
        qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : repair done, need repair : " << m_needRepair;
        m_state = RepairDone;
    } else if (m_state == Mounting) {
        auto access = m_storageInfo->device().as<Solid::StorageAccess>();
        m_isMounted = access->isAccessible();
        qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : Mount signal arrived. State changed : " << access->isAccessible();
        m_state = MountDone;
    } else if (m_state == Unmounting) {
        auto access = m_storageInfo->device().as<Solid::StorageAccess>();
        m_isMounted = access->isAccessible();
        qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : Unmount signal arrived. State changed : " << access->isAccessible();
        m_state = UnmountDone;
    } else {
        m_state = Idle;
    }
    qCDebug(APPLETS::DEVICENOTIFIER) << "State Info " << udi << " : state changed: " << m_operationResult << "; Is mounted: " << m_isMounted;

    Q_EMIT stateChanged(udi);
}

#include "moc_stateinfo.cpp"
