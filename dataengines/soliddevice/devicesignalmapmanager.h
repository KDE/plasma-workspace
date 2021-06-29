/*
    SPDX-FileCopyrightText: 2007 Christopher Blauvelt <cblauvelt@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

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
