/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
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
