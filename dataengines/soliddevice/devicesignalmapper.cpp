/*
    SPDX-FileCopyrightText: 2007 Christopher Blauvelt <cblauvelt@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "devicesignalmapper.h"

DeviceSignalMapper::DeviceSignalMapper(QObject *parent)
    : QSignalMapper(parent)
{
}

DeviceSignalMapper::~DeviceSignalMapper()
{
}

void DeviceSignalMapper::setMapping(QObject *device, const QString &udi)
{
    signalmap[device] = udi;
}

BatterySignalMapper::BatterySignalMapper(QObject *parent)
    : DeviceSignalMapper(parent)
{
}

BatterySignalMapper::~BatterySignalMapper()
{
}

void BatterySignalMapper::chargePercentChanged(int value)
{
    Q_EMIT deviceChanged(signalmap[sender()], QStringLiteral("Charge Percent"), value);
}

void BatterySignalMapper::chargeStateChanged(int newState)
{
    QStringList chargestate;
    chargestate << QStringLiteral("Fully Charged") << QStringLiteral("Charging") << QStringLiteral("Discharging");
    Q_EMIT deviceChanged(signalmap[sender()], QStringLiteral("Charge State"), chargestate.at(newState));
}

void BatterySignalMapper::presentStateChanged(bool newState)
{
    Q_EMIT deviceChanged(signalmap[sender()], QStringLiteral("Plugged In"), newState);
}

StorageAccessSignalMapper::StorageAccessSignalMapper(QObject *parent)
    : DeviceSignalMapper(parent)
{
}

StorageAccessSignalMapper::~StorageAccessSignalMapper()
{
}

void StorageAccessSignalMapper::accessibilityChanged(bool accessible)
{
    Q_EMIT deviceChanged(signalmap[sender()], QStringLiteral("Accessible"), accessible);
}
