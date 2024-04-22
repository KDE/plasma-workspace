/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QAbstractListModel>
#include <QDBusPendingCall>
#include <QObject>
#include <QObjectBindableProperty>
#include <qqmlregistration.h>

#include <solid/battery.h>

#include "batteriesnamesmonitor_p.h"

class BatteryControlModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hasCumulative READ default NOTIFY hasCumulativeChanged BINDABLE bindableHasCumulative)
    Q_PROPERTY(bool hasBatteries READ default NOTIFY hasBatteriesChanged BINDABLE bindableHasBatteries)
    Q_PROPERTY(bool pluggedIn READ default NOTIFY pluggedInChanged BINDABLE bindablePluggedIn)
    Q_PROPERTY(bool hasInternalBatteries READ default NOTIFY hasInternalBatteriesChanged BINDABLE bindableHasInternalBatteries)
    Q_PROPERTY(Solid::Battery::ChargeState state READ default NOTIFY stateChanged BINDABLE bindableState)
    Q_PROPERTY(int chargeStopThreshold READ default NOTIFY chargeStopThresholdChanged BINDABLE bindableChargeStopThresholdChanged)
    Q_PROPERTY(qulonglong remainingMsec READ default NOTIFY remainingMsecChanged BINDABLE bindableRemainingMsec)
    Q_PROPERTY(qulonglong smoothedRemainingMsec READ default NOTIFY smoothedRemainingMsecChanged BINDABLE bindableSmoothedRemainingMsec)
    Q_PROPERTY(int percent READ default NOTIFY percentChanged BINDABLE bindablePercent)

public:
    enum BatteryRoles {
        Percent = Qt::UserRole + 1,
        Capacity,
        Energy,
        PluggedIn,
        IsPowerSupply,
        ChargeState,
        PrettyName,
        Type,
    };
    Q_ENUM(BatteryRoles)

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    explicit BatteryControlModel(QObject *parent = nullptr);
    ~BatteryControlModel() override;

private:
    QBindable<bool> bindableHasCumulative();
    QBindable<bool> bindableHasBatteries();
    QBindable<bool> bindablePluggedIn();
    QBindable<bool> bindableHasInternalBatteries();
    QBindable<Solid::Battery::ChargeState> bindableState();
    QBindable<int> bindableChargeStopThresholdChanged();
    QBindable<qulonglong> bindableRemainingMsec();
    QBindable<qulonglong> bindableSmoothedRemainingMsec();
    QBindable<int> bindablePercent();

    Solid::Battery::ChargeState updateBatteryState(const Solid::Battery *battery) const;
    QString batteryTypeToString(const Solid::Battery *battery) const;

private Q_SLOTS:
    void batteryRemainingTimeChanged(qulonglong time);
    void smoothedBatteryRemainingTimeChanged(qulonglong time);
    void updateBatteryChargeStopThreshold(int threshold);
    void updateAcPlugState(bool onBattery);
    void updateOverallBattery();

    void updateBatteryCapacity(int newState, const QString &udi);
    void updateBatteryChargeState(int newState, const QString &udi);
    void updateBatteryChargePercent(int newValue, const QString &udi);
    void updateBatteryEnergy(double newValue, const QString &udi);
    void updateBatteryPowerSupplyState(bool newState, const QString &udi);
    void updatePluggedInState(bool onBattery, const QString &udi);

    void deviceAdded(const QString &udi);
    void deviceRemoved(const QString &udi);

Q_SIGNALS:
    void hasCumulativeChanged(bool status);
    void hasBatteriesChanged(bool statuts);
    void pluggedInChanged(bool status);
    void hasInternalBatteriesChanged(bool status);
    void stateChanged(Solid::Battery::ChargeState state);
    void chargeStopThresholdChanged(int threshold);
    void remainingMsecChanged(qulonglong time);
    void smoothedRemainingMsecChanged(qulonglong time);
    void percentChanged(int percent);

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControlModel, bool, m_hasCumulative, false, &BatteryControlModel::hasCumulativeChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControlModel, bool, m_hasBatteries, false, &BatteryControlModel::hasBatteriesChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControlModel, bool, m_hasInternalBatteries, false, &BatteryControlModel::hasInternalBatteriesChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControlModel, bool, m_pluggedIn, false, &BatteryControlModel::pluggedInChanged);
    Q_OBJECT_BINDABLE_PROPERTY(BatteryControlModel, Solid::Battery::ChargeState, m_state, &BatteryControlModel::stateChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControlModel, int, m_chargeStopThreshold, 0, &BatteryControlModel::chargeStopThresholdChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControlModel, qulonglong, m_remainingMsec, 0, &BatteryControlModel::remainingMsecChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControlModel, qulonglong, m_smoothedRemainingMsec, 0, &BatteryControlModel::smoothedRemainingMsecChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControlModel, int, m_percent, 0, &BatteryControlModel::percentChanged);

    std::unique_ptr<BatteriesNamesMonitor> namesMonitor;

    QList<QString> m_batterySources;
    QHash<QString, uint> m_batteryPositions;
    QList<QString> m_internalBatteries;
};
