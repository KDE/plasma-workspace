/*
 *   Copyright (C) 2007 Christopher Blauvelt <cblauvelt@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "devicesignalmapmanager.h"

DeviceSignalMapManager::DeviceSignalMapManager(QObject *parent) : QObject(parent)
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
        connect(map, SIGNAL(deviceChanged(QString,QString,QVariant)), user, SLOT(deviceChanged(QString,QString,QVariant)));
    } else {
        map = (BatterySignalMapper*)signalmap[Solid::DeviceInterface::Battery];
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
        connect(map, SIGNAL(deviceChanged(QString,QString,QVariant)), user, SLOT(deviceChanged(QString,QString,QVariant)));
    } else {
        map = (StorageAccessSignalMapper*)signalmap[Solid::DeviceInterface::StorageAccess];
    }

    connect(storageaccess, &Solid::StorageAccess::accessibilityChanged, map, &StorageAccessSignalMapper::accessibilityChanged);
    map->setMapping(storageaccess, udi);
}

void DeviceSignalMapManager::unmapDevice(Solid::Battery *battery)
{
    BatterySignalMapper *map = (BatterySignalMapper*)signalmap.value(Solid::DeviceInterface::Battery);
    if (!map) {
        return;
    }

    disconnect(battery, &Solid::Battery::chargePercentChanged, map, &BatterySignalMapper::chargePercentChanged);
    disconnect(battery, &Solid::Battery::chargeStateChanged, map, &BatterySignalMapper::chargeStateChanged);
    disconnect(battery, &Solid::Battery::presentStateChanged, map, &BatterySignalMapper::presentStateChanged);
}

void DeviceSignalMapManager::unmapDevice(Solid::StorageAccess *storageaccess)
{
    StorageAccessSignalMapper *map = (StorageAccessSignalMapper*)signalmap.value(Solid::DeviceInterface::StorageAccess);
    if (!map) {
        return;
    }

    disconnect(storageaccess, &Solid::StorageAccess::accessibilityChanged, map, &StorageAccessSignalMapper::accessibilityChanged);
}


