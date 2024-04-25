/*
    SPDX-FileCopyrightText: 2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2007-2008 Sebastian Kuegler <sebas@kde.org>
    SPDX-FileCopyrightText: 2008 Dario Freddi <drf54321@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <Plasma5Support/DataEngine>

#include <solid/battery.h>

#include <QDBusConnection>
#include <QHash>
#include <QPair>

class SessionManagement;

struct SolidInhibition {
    uint cookie;
    QString appName;
    QString reason;
};

/**
 * This class provides runtime information about the battery and AC status
 * for use in power management Plasma applets.
 */
class PowermanagementEngine : public Plasma5Support::DataEngine
{
    Q_OBJECT

public:
    PowermanagementEngine(QObject *parent);
    ~PowermanagementEngine() override;
    QStringList sources() const override;
    Plasma5Support::Service *serviceForSource(const QString &source) override;

protected:
    bool sourceRequestEvent(const QString &name) override;
    bool updateSourceEvent(const QString &source) override;
    void init();

private Q_SLOTS:
    void updateBatteryChargeState(int newState, const QString &udi);
    void updateBatteryPresentState(bool newState, const QString &udi);
    void updateBatteryChargePercent(int newValue, const QString &udi);
    void updateBatteryEnergy(double newValue, const QString &udi);
    void updateBatteryPowerSupplyState(bool newState, const QString &udi);
    void updateAcPlugState(bool onBattery);
    void updateBatteryNames();
    void updateOverallBattery();

    void deviceRemoved(const QString &udi);
    void deviceAdded(const QString &udi);
    void batteryRemainingTimeChanged(qulonglong time);
    void smoothedBatteryRemainingTimeChanged(qulonglong time);
    void triggersLidActionChanged(bool triggers);
    void hasInhibitionChanged(bool inhibited);
    void updateInhibitions(const QList<SolidInhibition> &inhibitions);
    void inhibitionsChanged(const QList<SolidInhibition> &added, const QList<uint> &removed);
    void chargeStopThresholdChanged(int threshold);

    void updatePowerProfileDaemonInstalled(const bool &installed);
    void updatePowerProfileCurrentProfile(const QString &profile);
    void updatePowerProfileChoices(const QStringList &choices);
    void updatePowerProfilePerformanceInhibitedReason(const QString &reason);
    void updatePowerProfilePerformanceDegradedReason(const QString &reason);
    void updatePowerProfileHolds(const QList<QVariantMap> &holds);

private:
    template<typename ReplyType>
    inline void createPowerManagementDBusMethodCallAndNotifyChanged(const QString &method, std::function<void(ReplyType)> &&callback);

    template<typename ReplyType>
    inline void createPowerProfileDBusMethodCallAndNotifyChanged(const QString &method, std::function<void(ReplyType)> &&callback);

    void populateApplicationData(const QString &name, QString *prettyName, QString *icon);
    QString batteryTypeToString(const Solid::Battery *battery) const;
    QStringList basicSourceNames() const;
    QString batteryStateToString(int newState) const;

    QStringList m_sources;

    QHash<QString, QString> m_batterySources; // <udi, Battery0>
    QHash<QString, QPair<QString, QString>> m_applicationInfo; // <appname, <pretty name, icon>>

    SessionManagement *m_session;
};
