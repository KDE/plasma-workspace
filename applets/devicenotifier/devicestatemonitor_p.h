/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Solid/Device>
#include <Solid/StorageAccess>

#include <QDateTime>

#include <qqmlintegration.h>

// TODO: implement in libsolid2
template<class DevIface>
DevIface *getAncestorAs(const Solid::Device &device)
{
    for (Solid::Device parent = device.parent(); parent.isValid(); parent = parent.parent()) {
        if (parent.is<DevIface>()) {
            return parent.as<DevIface>();
        }
    }
    return nullptr;
}

/**
 * This class is connected with solid, and monitors state of devices
 */
class DevicesStateMonitor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton that monitors device states")

public:
    enum State {
        NotPresent = 0,
        Idle,
        Mounting,
        MountDone,
        Unmounting,
        UnmountDone,
        Checking,
        CheckDone,
        Repairing,
        RepairDone,
    };

    Q_ENUM(State)

    static std::shared_ptr<DevicesStateMonitor> instance();
    ~DevicesStateMonitor() override;

    void addMonitoringDevice(const QString &udi);
    void removeMonitoringDevice(const QString &udi);

    bool isBusy(const QString &udi) const;
    bool isRemovable(const QString &udi) const;
    bool isMounted(const QString &udi) const;
    bool isChecked(const QString &udi) const;
    bool needRepair(const QString &udi) const;
    QDateTime getDeviceTimeStamp(const QString &udi) const;

    /**
     * Return current status of device:
     *  NotPresent: when device was removed
     *  Idle: when device not do anything
     *  Working: If device in work(mounted, unmounted...)
     *  Successful: When last work was successful
     *  Unsuccessful: When last work was unsuccessful
     *
     * Successful and unsuccessful states are changed to Idle after some time
     */
    State getState(const QString &udi) const;
    Solid::ErrorType getOperationResult(const QString &udi) const;
    QVariant getOperationInfo(const QString &udi) const;

private:
    explicit DevicesStateMonitor(QObject *parent = nullptr);

    void updateEncryptedContainer(const QString &udi);

private Q_SLOTS:
    void setAccessibilityState(bool isAccessible, const QString &udi);
    void setMountingState(const QString &udi);
    void setUnmountingState(const QString &udi);
    void setCheckingState(const QString &udi);
    void setRepairingState(const QString &udi);
    void setIdleState(Solid::ErrorType operationResult, QVariant operationInfo, const QString &udi);

Q_SIGNALS:

    /**
     * Emitted when state of device is changed
     */
    void stateChanged(const QString &udi);

private:
    struct DeviceInfo {
        bool isBusy;
        bool isRemovable;
        bool isMounted;
        bool isChecked;
        bool needRepair;
        Solid::ErrorType operationResult;
        QVariant operationInfo;
        State state;
        QDateTime deviceTimeStamp;
    };

    QHash<QString, QString> m_encryptedContainerMap;
    QHash<QString, DeviceInfo> m_devicesStates;
};
