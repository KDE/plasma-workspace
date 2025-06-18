/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include "actionscontrol.h"

#include <QAbstractListModel>
#include <qqmlregistration.h>

#include "deviceerrormonitor_p.h"
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
        OperationResult,
        Timestamp,
        Error,
        ErrorMessage,
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
    void onDeviceErrorChanged(const QString &udi);

private:
    void deviceDelayRemove(const QString &udi, const QString &parentUdi);

    QList<Solid::Device> m_devices;
    QHash<QString, ActionsControl *> m_actions;

    // save device type to properly sort and icon and description as workaround because
    // solid removes it and list model show empty device(without icon and description).
    // first = type
    // second.first = icon
    // second.second = description
    QHash<QString, std::pair<QString, std::pair<QString, QString>>> m_deviceTypes;

    QHash<QString, QList<Solid::Device>> m_parentDevices;

    struct RemoveTimerData {
        QTimer *timer = nullptr;
        QString udi;
        QString parentUdi;
    };
    QHash<QString, RemoveTimerData> m_removeTimers;
    Solid::Predicate m_predicateDeviceMatch;
    Solid::Predicate m_encryptedPredicate;
    const QList<Solid::DeviceInterface::Type> m_types;
    bool m_isVisible = false;

    std::shared_ptr<SpaceMonitor> m_spaceMonitor;
    std::shared_ptr<DevicesStateMonitor> m_stateMonitor;
    std::shared_ptr<DeviceErrorMonitor> m_errorMonitor;
};
