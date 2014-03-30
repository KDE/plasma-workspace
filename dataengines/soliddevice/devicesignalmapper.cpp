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

#include "devicesignalmapper.h"

DeviceSignalMapper::DeviceSignalMapper(QObject *parent) : QSignalMapper(parent)
{
}

DeviceSignalMapper::~DeviceSignalMapper()
{
}

void DeviceSignalMapper::setMapping(QObject* device, const QString &udi)
{
    signalmap[device] = udi;
}

AcAdapterSignalMapper::AcAdapterSignalMapper(QObject *parent) : DeviceSignalMapper(parent)
{
}

AcAdapterSignalMapper::~AcAdapterSignalMapper()
{
}

void AcAdapterSignalMapper::plugStateChanged(bool newState)
{
    emit(deviceChanged(signalmap[sender()], "Plugged In", newState));
}


ButtonSignalMapper::ButtonSignalMapper(QObject *parent) : DeviceSignalMapper(parent)
{
}

ButtonSignalMapper::~ButtonSignalMapper()
{
}

void ButtonSignalMapper::pressed(Solid::Button::ButtonType type)
{
    Q_UNUSED(type)
    emit(deviceChanged(signalmap[sender()], "Pressed", true));
}

BatterySignalMapper::BatterySignalMapper(QObject *parent) : DeviceSignalMapper(parent)
{
}

BatterySignalMapper::~BatterySignalMapper()
{
}

void BatterySignalMapper::chargePercentChanged(int value)
{
    emit(deviceChanged(signalmap[sender()], "Charge Percent", value));
}

void BatterySignalMapper::chargeStateChanged(int newState)
{
    QStringList chargestate;
    chargestate << "Fully Charged" << "Charging" << "Discharging";
    emit(deviceChanged(signalmap[sender()], "Charge State", chargestate.at(newState)));
}

void BatterySignalMapper::plugStateChanged(bool newState)
{
    emit(deviceChanged(signalmap[sender()], "Plugged In", newState));
}

StorageAccessSignalMapper::StorageAccessSignalMapper(QObject *parent) : DeviceSignalMapper(parent)
{
}

StorageAccessSignalMapper::~StorageAccessSignalMapper()
{
}

void StorageAccessSignalMapper::accessibilityChanged(bool accessible)
{
    emit(deviceChanged(signalmap[sender()], "Accessible", accessible));
}

#include "devicesignalmapper.moc"
