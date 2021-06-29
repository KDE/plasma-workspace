/*
    SPDX-FileCopyrightText: 2007 Christopher Blauvelt <cblauvelt@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "devicesignalmapmanager.h"

DeviceSignalMapManager::DeviceSignalMapManager(QObject *parent)
    : QObject(parent)
{
    user = parent;
}

DeviceSignalMapManager::~DeviceSignalMapManager()
{
}

void DeviceSignalMapManager::mapDevice(Solid::Battery *battery, const QString &udi)
{
    BatterySignalMapper *map = nullptr;
    if (!signalmap.contains(Solid::DeviceInterface::Battery)) {
        map = new BatterySignalMapper(this);
        signalmap[Solid::DeviceInterface::Battery] = map;
        connect(map, SIGNAL(deviceChanged(QString, QString, QVariant)), user, SLOT(deviceChanged(QString, QString, QVariant)));
    } else {
        map = (BatterySignalMapper *)signalmap[Solid::DeviceInterface::Battery];
    }

    connect(battery, &Solid::Battery::chargePercentChanged, map, &BatterySignalMapper::chargePercentChanged);
    connect(battery, &Solid::Battery::chargeStateChanged, map, &BatterySignalMapper::chargeStateChanged);
    connect(battery, &Solid::Battery::presentStateChanged, map, &BatterySignalMapper::presentStateChanged);
    map->setMapping(battery, udi);
}

void DeviceSignalMapManager::mapDevice(Solid::StorageAccess *storageaccess, const QString &udi)
{
    StorageAccessSignalMapper *map = nullptr;
    if (!signalmap.contains(Solid::DeviceInterface::StorageAccess)) {
        map = new StorageAccessSignalMapper(this);
        signalmap[Solid::DeviceInterface::StorageAccess] = map;
        connect(map, SIGNAL(deviceChanged(QString, QString, QVariant)), user, SLOT(deviceChanged(QString, QString, QVariant)));
    } else {
        map = (StorageAccessSignalMapper *)signalmap[Solid::DeviceInterface::StorageAccess];
    }

    connect(storageaccess, &Solid::StorageAccess::accessibilityChanged, map, &StorageAccessSignalMapper::accessibilityChanged);
    map->setMapping(storageaccess, udi);
}

void DeviceSignalMapManager::unmapDevice(Solid::Battery *battery)
{
    BatterySignalMapper *map = (BatterySignalMapper *)signalmap.value(Solid::DeviceInterface::Battery);
    if (!map) {
        return;
    }

    disconnect(battery, &Solid::Battery::chargePercentChanged, map, &BatterySignalMapper::chargePercentChanged);
    disconnect(battery, &Solid::Battery::chargeStateChanged, map, &BatterySignalMapper::chargeStateChanged);
    disconnect(battery, &Solid::Battery::presentStateChanged, map, &BatterySignalMapper::presentStateChanged);
}

void DeviceSignalMapManager::unmapDevice(Solid::StorageAccess *storageaccess)
{
    StorageAccessSignalMapper *map = (StorageAccessSignalMapper *)signalmap.value(Solid::DeviceInterface::StorageAccess);
    if (!map) {
        return;
    }

    disconnect(storageaccess, &Solid::StorageAccess::accessibilityChanged, map, &StorageAccessSignalMapper::accessibilityChanged);
}
