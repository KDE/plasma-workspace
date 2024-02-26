/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDBusPendingCall>
#include <QObject>
#include <QObjectBindableProperty>
#include <qqmlregistration.h>

#include <solid/battery.h>

#include "batteryunit.h"

class BatteryControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hasCumulative READ default NOTIFY hasCumulativeChanged BINDABLE bindableHasCumulative)
    Q_PROPERTY(bool hasBattery READ default NOTIFY hasBatteryChanged BINDABLE bindableHasBattery)
    Q_PROPERTY(bool pluggedIn READ default NOTIFY pluggedInChanged BINDABLE bindablePluggedIn)
    Q_PROPERTY(QString state READ default NOTIFY stateChanged BINDABLE bindableState)
    Q_PROPERTY(int chargeStopThreshold READ default NOTIFY chargeStopThresholdChanged BINDABLE bindableChargeStopThresholdChanged)
    Q_PROPERTY(qulonglong remainingMsec READ default NOTIFY remainingMsecChanged BINDABLE bindableRemainingMsec)
    Q_PROPERTY(qulonglong smoothedRemainingMsec READ default NOTIFY smoothedRemainingMsecChanged BINDABLE bindableSmoothedRemainingMsec)
    Q_PROPERTY(int percent READ default NOTIFY percentChanged BINDABLE bindablePercent)
    Q_PROPERTY(QList<QObject *> batteries READ batteries NOTIFY batteriesChanged)

public:
    BatteryControl(QObject *parent = nullptr);
    ~BatteryControl();

private:
    QList<QObject *> batteries();
    QBindable<bool> bindableHasCumulative();
    QBindable<bool> bindableHasBattery();
    QBindable<bool> bindablePluggedIn();
    QBindable<QString> bindableState();
    QBindable<int> bindableChargeStopThresholdChanged();
    QBindable<qulonglong> bindableRemainingMsec();
    QBindable<qulonglong> bindableSmoothedRemainingMsec();
    QBindable<int> bindablePercent();

private Q_SLOTS:
    void batteryRemainingTimeChanged(qulonglong time);
    void smoothedBatteryRemainingTimeChanged(qulonglong time);
    void updateAcPlugState(bool onBattery);
    void updateOverallBattery();

    void deviceAdded(const QString &udi);
    void deviceRemoved(const QString &udi);

Q_SIGNALS:
    void hasCumulativeChanged(bool status);
    void hasBatteryChanged(bool statuts);
    void pluggedInChanged(bool status);
    void stateChanged(QString state);
    void chargeStopThresholdChanged(int threshold);
    void remainingMsecChanged(qulonglong time);
    void smoothedRemainingMsecChanged(qulonglong time);
    void percentChanged(int percent);
    void batteriesChanged(QList<QObject *> batteries);

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControl, bool, m_hasCumulative, false, &BatteryControl::hasCumulativeChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControl, bool, m_hasBattery, false, &BatteryControl::hasBatteryChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControl, bool, m_pluggedIn, false, &BatteryControl::pluggedInChanged);
    Q_OBJECT_BINDABLE_PROPERTY(BatteryControl, QString, m_state, &BatteryControl::stateChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControl, int, m_chargeStopThreshold, 0, &BatteryControl::chargeStopThresholdChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControl, qulonglong, m_remainingMsec, 0, &BatteryControl::remainingMsecChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControl, qulonglong, m_smoothedRemainingMsec, 0, &BatteryControl::smoothedRemainingMsecChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BatteryControl, int, m_percent, 0, &BatteryControl::percentChanged);

    QList<QObject *> m_batterySources;
};
