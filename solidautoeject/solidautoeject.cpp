/* This file is part of the KDE Project
   Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "solidautoeject.h"

#include <kpluginfactory.h>

#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/opticaldrive.h>

K_PLUGIN_FACTORY(SolidAutoEjectFactory, registerPlugin<SolidAutoEject>();)
K_EXPORT_PLUGIN(SolidAutoEjectFactory("solidautoeject"))

SolidAutoEject::SolidAutoEject(QObject* parent, const QList<QVariant>&)
    : KDEDModule(parent)
{
    QList<Solid::Device> drives = Solid::Device::listFromQuery("IS OpticalDrive");
    foreach (const Solid::Device &drive, drives) {
        connectDevice(drive);
    }

    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)),
            this, SLOT(onDeviceAdded(QString)));
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
    if ( device.as<Solid::OpticalDrive>() ) {
        connect(device.as<Solid::OpticalDrive>(), SIGNAL(ejectPressed(QString)),
                this, SLOT(onEjectPressed(QString)));
    }
}

#include "solidautoeject.moc"
