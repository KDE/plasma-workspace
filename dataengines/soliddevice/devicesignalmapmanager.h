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

#ifndef DEVICE_SIGNALMAP_MANAGER_H
#define DEVICE_SIGNALMAP_MANAGER_H

#include <QDebug>

#include "devicesignalmapper.h"

class DeviceSignalMapManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceSignalMapManager(QObject *parent = nullptr);
    ~DeviceSignalMapManager() override;

    void mapDevice(Solid::Battery *battery, const QString &udi);
    void mapDevice(Solid::StorageAccess *storageaccess, const QString &udi);

    void unmapDevice(Solid::Battery *battery);
    void unmapDevice(Solid::StorageAccess *storageaccess);

private:
    QMap<Solid::DeviceInterface::Type, DeviceSignalMapper *> signalmap;
    QObject *user;
};

#endif
