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
    QList<Solid::Device> m_devices;
    QHash<QString, ActionsControl *> m_actions;
    QHash<QString, QString> m_encryptedContainerMap;
    Solid::Predicate m_predicateDeviceMatch;
    Solid::Predicate m_encryptedPredicate;
    const QList<Solid::DeviceInterface::Type> m_types;
    bool m_isVisible;

    std::shared_ptr<SpaceMonitor> m_spaceMonitor;
    std::shared_ptr<DevicesStateMonitor> m_stateMonitor;
    std::shared_ptr<DeviceErrorMonitor> m_errorMonitor;
};
