/*
 *   Copyright (C) 2011 Viranch Mehta <viranch.mehta@gmail.com>
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

#include "soliddevicejob.h"

#include <solid/device.h>
#include <solid/opticaldisc.h>
#include <solid/opticaldrive.h>
#include <solid/storageaccess.h>

#include <QDebug>

void SolidDeviceJob::start()
{
    Solid::Device device(m_dest);
    QString operation = operationName();

    if (operation == QLatin1String("mount")) {
        if (device.is<Solid::StorageAccess>()) {
            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
            if (access && !access->isAccessible()) {
                access->setup();
            }
        }
    } else if (operation == QLatin1String("unmount")) {
        if (device.is<Solid::OpticalDisc>()) {
            Solid::OpticalDrive *drive = device.as<Solid::OpticalDrive>();
            if (!drive) {
                drive = device.parent().as<Solid::OpticalDrive>();
            }
            if (drive) {
                drive->eject();
            }
        } else if (device.is<Solid::StorageAccess>()) {
            Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
            if (access && access->isAccessible()) {
                access->teardown();
            }
        }
    }

    emitResult();
}
