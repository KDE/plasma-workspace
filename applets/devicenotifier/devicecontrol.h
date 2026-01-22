/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include "actionscontrol.h"

#include <QAbstractListModel>
#include <qqmlregistration.h>

#include "storageinfo.h"

#include "devicemessagemonitor_p.h"
#include "devicestatemonitor_p.h"
#include "spacemonitor_p.h"

class DeviceControl : public QAbstractListModel
{
    Q_OBJECT

public:
    enum DeviceModels {
        Udi = Qt::UserRole + 1,
        Description,
        Type,
        Icon,
        Emblems,
        IsBusy,
        IsRemovable,
        FreeSpace,
        Size,
        FreeSpaceText,
        SizeText,
        Mounted,
        State,
        OperationResult,
        Timestamp,
        Message,
        Actions,
    };

    Q_ENUM(DeviceModels)

    explicit DeviceControl(QObject *parent = nullptr);
    ~DeviceControl() override;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void onDeviceAdded(const QString &udi);
    void onDeviceRemoved(const QString &udi);
    void onDeviceChanged(const QMap<QString, int> &props);

    void onDeviceSizeChanged(const QString &udi);
    void onDeviceStatusChanged(const QString &udi);
    void onDeviceMessageChanged(const QString &udi);

private:
    void deviceDelayRemove(const QString &udi, const QString &parentUdi);

    struct DeviceInfo {
        std::shared_ptr<StorageInfo> storageInfo;
    };

    QList<DeviceInfo> m_devices;
    QSet<QString> m_devicesUdi;
    QHash<QString, ActionsControl *> m_actions;

    QHash<QString, QList<std::shared_ptr<StorageInfo>>> m_parentDevices;

    struct RemoveTimerData {
        std::shared_ptr<QTimer> timer;
        QString udi;
        QString parentUdi;
    };
    QHash<QString, RemoveTimerData> m_removeTimers;

    std::shared_ptr<SpaceMonitor> m_spaceMonitor;
    std::shared_ptr<DevicesStateMonitor> m_stateMonitor;
    std::shared_ptr<DeviceMessageMonitor> m_messageMonitor;
};
