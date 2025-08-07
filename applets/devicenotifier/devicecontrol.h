/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QAbstractListModel>
#include <QTimer>
#include <qqmlregistration.h>

#include "actionsinfo.h"
#include "messageinfo.h"
#include "spaceinfo.h"
#include "stateinfo.h"
#include "storageinfo.h"

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

    static std::shared_ptr<DeviceControl> instance();
    ~DeviceControl() override;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void onDeviceAdded(const QString &udi);
    void onDeviceRemoved(const QString &udi);

    void onDeviceSizeChanged(const QString &udi);
    void onDeviceStatusChanged(const QString &udi);
    void onDeviceMessageChanged(const QString &udi);

private:
    explicit DeviceControl(QObject *parent = nullptr);

    void deviceDelayRemove(const QString &udi, const QString &parentUdi);

    struct DeviceInfo {
        std::shared_ptr<StorageInfo> storageInfo;
        std::shared_ptr<StateInfo> stateInfo;
        std::shared_ptr<SpaceInfo> spaceInfo;
        std::shared_ptr<MessageInfo> messageInfo;
        std::shared_ptr<ActionsInfo> actionsInfo;
    };

    QList<DeviceInfo> m_devices;
    QSet<QString> m_devicesUdi;

    QHash<QString, QList<std::shared_ptr<StorageInfo>>> m_parentDevices;

    struct RemoveTimerData {
        std::shared_ptr<QTimer> timer;
        QString udi;
        QString parentUdi;
    };
    QHash<QString, RemoveTimerData> m_removeTimers;
};
