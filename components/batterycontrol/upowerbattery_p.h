/*
 * SPDX-FileCopyrightText: 2026 Ramil Nurmanov <ramil2004nur@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QFlags>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

#include "upowertypes_p.h"

class QDBusPendingCallWatcher;

class UPowerBattery : public QObject
{
    Q_OBJECT

public:
    enum class ChargeState {
        NoCharge,
        Charging,
        Discharging,
        FullyCharged,
    };

    enum class Type {
        Unknown,
        Primary,
        Ups,
        Mouse,
        Keyboard,
        Pda,
        Phone,
        GamingInput,
        Bluetooth,
        Tablet,
        Headphone,
        Headset,
        Touchpad,
        Wearable,
        Speakers,
    };

    enum class Change {
        Present = 1 << 0,
        ChargePercent = 1 << 1,
        Capacity = 1 << 2,
        PowerSupply = 1 << 3,
        ChargeState = 1 << 4,
        Energy = 1 << 5,
        EnergyFull = 1 << 6,
    };
    Q_DECLARE_FLAGS(Changes, Change)

    explicit UPowerBattery(const QString &udi, QObject *parent = nullptr);

    void load();

    QString udi() const;
    QString vendor() const;
    QString product() const;
    Type type() const;
    bool isPresent() const;
    bool isPowerSupply() const;
    int chargePercent() const;
    int capacity() const;
    double energy() const;
    double energyFull() const;
    ChargeState chargeState() const;

Q_SIGNALS:
    void loadFinished(const QString &udi, bool isBattery);

    void changed(UPowerBattery::Changes changes, const QString &udi);

private Q_SLOTS:
    void onPropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    struct Properties {
        UpDeviceKind kind = UP_DEVICE_KIND_UNKNOWN;
        QString vendor;
        QString model;
        UpDeviceTechnology technology = UP_DEVICE_TECHNOLOGY_UNKNOWN;
        bool isPresent = false;
        bool isPowerSupply = false;
        double percentage = 0;
        UpDeviceLevel batteryLevel = UP_DEVICE_LEVEL_NONE;
        double capacity = 0;
        UpDeviceState state = UP_DEVICE_STATE_UNKNOWN;
        double energy = 0;
        double energyFull = 0;

        void update(const QVariantMap &changedProperties);
        int chargePercent() const;
        int capacityPercent() const;
        ChargeState chargeState() const;
    };

    void read();
    void scheduleRetry();

    void emitChanges(const Properties &previous);

    QString m_udi;
    Properties m_properties;
    QVariantMap m_changesDuringRead;
    QDBusPendingCallWatcher *m_readWatcher = nullptr;
    QTimer m_retryTimer;
    int m_retryCount = 0;
    bool m_loaded = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(UPowerBattery::Changes)
