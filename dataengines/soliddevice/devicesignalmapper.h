/*
    SPDX-FileCopyrightText: 2007 Christopher Blauvelt <cblauvelt@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QMap>
#include <QObject>
#include <QSignalMapper>

#include <QDebug>

#include <solid/battery.h>
#include <solid/block.h>
#include <solid/camera.h>
#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/opticaldisc.h>
#include <solid/opticaldrive.h>
#include <solid/portablemediaplayer.h>
#include <solid/processor.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>
#include <solid/storagevolume.h>

class DeviceSignalMapper : public QSignalMapper
{
    Q_OBJECT

public:
    explicit DeviceSignalMapper(QObject *parent = nullptr);
    ~DeviceSignalMapper() override;

    void setMapping(QObject *device, const QString &udi);

Q_SIGNALS:
    void deviceChanged(const QString &udi, const QString &property, QVariant value);

protected:
    QMap<QObject *, QString> signalmap;
};

class BatterySignalMapper : public DeviceSignalMapper
{
    Q_OBJECT

public:
    explicit BatterySignalMapper(QObject *parent = nullptr);
    ~BatterySignalMapper() override;

public Q_SLOTS:
    void chargePercentChanged(int value);
    void chargeStateChanged(int newState);
    void presentStateChanged(bool newState);
};

class StorageAccessSignalMapper : public DeviceSignalMapper
{
    Q_OBJECT

public:
    explicit StorageAccessSignalMapper(QObject *parent = nullptr);
    ~StorageAccessSignalMapper() override;

public Q_SLOTS:
    void accessibilityChanged(bool accessible);
};
