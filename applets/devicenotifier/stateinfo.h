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

#include "storageinfo.h"

/**
 * This class is connected with solid, and monitors state of devices
 */
class StateInfo : public QObject
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
        Ejecting,
        EjectDone,
        Checking,
        CheckDone,
        Repairing,
        RepairDone,
    };

    Q_ENUM(State)

    explicit StateInfo(const std::shared_ptr<StorageInfo> &storageInfo, QObject *parent = nullptr);
    ~StateInfo() override;

    bool isBusy() const;
    bool isMounted() const;
    bool isChecked() const;
    bool needRepair() const;
    bool isSafelyRemovable() const;
    QDateTime getDeviceTimeStamp() const;

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
    State getState() const;
    Solid::ErrorType getOperationResult() const;
    QVariant getOperationInfo() const;

public Q_SLOTS:
    void setNotPresentState(const QString &udi);

private Q_SLOTS:
    void setAccessibilityState(bool isAccessible, const QString &udi);
    void setMountingState(const QString &udi);
    void setUnmountingState(const QString &udi);
    void setEjectingState(const QString &udi);
    void setCheckingState(const QString &udi);
    void setRepairingState(const QString &udi);
    void setIdleState(Solid::ErrorType operationResult, QVariant operationInfo, const QString &udi);

Q_SIGNALS:

    /**
     * Emitted when state of device is changed
     */
    void stateChanged(const QString &udi);

private:
    bool m_isBusy;
    bool m_isMounted;
    bool m_isChecked;
    bool m_needRepair;
    Solid::ErrorType m_operationResult;
    QVariant m_operationInfo;
    State m_state;
    QDateTime m_deviceTimeStamp;

    std::shared_ptr<StorageInfo> m_storageInfo;
};
