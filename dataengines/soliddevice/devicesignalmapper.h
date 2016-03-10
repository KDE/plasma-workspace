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

#ifndef DEVICE_SIGNAL_MAPPER_H
#define DEVICE_SIGNAL_MAPPER_H

#include <QObject>
#include <QSignalMapper>
#include <QMap>

#include <QDebug>

#include <solid/devicenotifier.h>
#include <solid/device.h>
#include <solid/processor.h>
#include <solid/block.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>
#include <solid/opticaldrive.h>
#include <solid/storagevolume.h>
#include <solid/opticaldisc.h>
#include <solid/camera.h>
#include <solid/portablemediaplayer.h>
#include <solid/battery.h>

class DeviceSignalMapper : public QSignalMapper
{
    Q_OBJECT

    public:
        DeviceSignalMapper(QObject *parent=0);
        ~DeviceSignalMapper() override;
        
        void setMapping(QObject* device, const QString &udi);

    Q_SIGNALS:
        void deviceChanged(const QString& udi, const QString &property, QVariant value);
        
    protected:
        QMap<QObject*, QString> signalmap;
};

class BatterySignalMapper : public DeviceSignalMapper
{
    Q_OBJECT

    public:
        BatterySignalMapper(QObject *parent=0);
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
        StorageAccessSignalMapper(QObject *parent=0);
        ~StorageAccessSignalMapper() override;

    public Q_SLOTS:
        void accessibilityChanged(bool accessible);
};

#endif 
