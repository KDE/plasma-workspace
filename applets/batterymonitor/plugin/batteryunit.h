/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QObjectBindableProperty>
#include <QString>
#include <qqmlregistration.h>

#include <solid/battery.h>

class BatteryUnit : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int percent READ default NOTIFY percentChanged BINDABLE bindablePercent)
    Q_PROPERTY(int capacity READ default NOTIFY capacityChanged BINDABLE bindableCapacity)
    Q_PROPERTY(double energy READ default NOTIFY energyChanged BINDABLE bindableEnergy)
    Q_PROPERTY(bool pluggedIn READ default NOTIFY pluggedInChanged BINDABLE bindablePluggedIn)
    Q_PROPERTY(bool isPowerSupply READ default NOTIFY isPowerSupplyChanged BINDABLE bindableIsPowerSupply)
    Q_PROPERTY(QString state READ default NOTIFY stateChanged BINDABLE bindableState)

    Q_PROPERTY(QString vendor READ vendor NOTIFY vendorChanged)
    Q_PROPERTY(QString product READ product NOTIFY productChanged)
    Q_PROPERTY(QString type READ type NOTIFY typeChanged)
    Q_PROPERTY(QString prettyName READ prettyName NOTIFY prettyNameChanged)

public:
    explicit BatteryUnit(const Solid::Device &deviceBattery, QObject *parent = nullptr);
    ~BatteryUnit();

    QString udi() const;

public Q_SLOTS:
    void updateBatteryCapacity(int newState, const QString &udi);
    void updateBatteryChargeState(int newState, const QString &udi);
    void updateBatteryChargePercent(int newValue, const QString &udi);
    void updateBatteryEnergy(double newValue, const QString &udi);
    void updateBatteryPowerSupplyState(bool newState, const QString &udi);
    void updatePluggedInState(bool onBattery, const QString &udi);

private:
    QString batteryStateToString(int newState) const;
    void updateBatteryName();
    QString batteryTypeToString(const Solid::Battery *battery) const;

    QString state();
    QString vendor();
    QString product();
    QString type();
    QString prettyName();

    QBindable<int> bindablePercent();
    QBindable<int> bindableCapacity();
    QBindable<double> bindableEnergy();
    QBindable<bool> bindableIsPowerSupply();
    QBindable<bool> bindablePluggedIn();
    QBindable<QString> bindableState();

Q_SIGNALS:
    void stateChanged(QString state);
    void vendorChanged(QString vendor);
    void productChanged(QString product);
    void typeChanged(QString type);
    void prettyNameChanged(QString prettyName);
    void percentChanged(int percent);
    void capacityChanged(int capacity);
    void energyChanged(double energy);
    void isPowerSupplyChanged(bool status);
    void pluggedInChanged(bool status);

    void batteryStatusChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryUnit, int, m_percent, 0, &BatteryUnit::percentChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryUnit, int, m_capacity, 0, &BatteryUnit::capacityChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryUnit, double, m_energy, 0, &BatteryUnit::energyChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryUnit, bool, m_isPowerSupply, false, &BatteryUnit::isPowerSupplyChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryUnit, bool, m_pluggedIn, false, &BatteryUnit::pluggedInChanged)
    Q_OBJECT_BINDABLE_PROPERTY(BatteryUnit, QString, m_state, &BatteryUnit::stateChanged)

    QString m_vendor;
    QString m_product;
    QString m_type;
    QString m_prettyName;

    QString m_udi;

    bool isUnnamedBattery = false;

    static uint m_unnamedBatteries;
};
