/*
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "solidautoeject.h"

#include <kpluginfactory.h>

#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/opticaldrive.h>

K_PLUGIN_CLASS_WITH_JSON(SolidAutoEject, "solidautoeject.json")

SolidAutoEject::SolidAutoEject(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    const QList<Solid::Device> drives = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDrive);
    for (const Solid::Device &drive : drives) {
        connectDevice(drive);
    }

    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &SolidAutoEject::onDeviceAdded);
}

SolidAutoEject::~SolidAutoEject()
{
}

void SolidAutoEject::onDeviceAdded(const QString &udi)
{
    connectDevice(Solid::Device(udi));
}

void SolidAutoEject::onEjectPressed(const QString &udi)
{
    Solid::Device dev(udi);
    dev.as<Solid::OpticalDrive>()->eject();
}

void SolidAutoEject::connectDevice(const Solid::Device &device)
{
    connect(device.as<Solid::OpticalDrive>(), &Solid::OpticalDrive::ejectPressed, this, &SolidAutoEject::onEjectPressed);
}

#include "solidautoeject.moc"
