/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDBusPendingCall>
#include <QObject>
#include <QObjectBindableProperty>
#include <QStringList>
#include <qqmlregistration.h>

#include "applicationdata_p.h"

class PowerProfilesControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isPowerProfileDaemonInstalled READ default NOTIFY isPowerProfileDaemonInstalledChanged BINDABLE bindableIsPowerProfileDaemonInstalled);
    Q_PROPERTY(QStringList profiles READ default NOTIFY profilesChanged BINDABLE bindableProfiles)
    Q_PROPERTY(QList<QVariantMap> activeProfileHolds READ default NOTIFY activeProfileHoldsChanged BINDABLE bindableActiveProfileHolds)
    Q_PROPERTY(QString actuallyActiveProfile READ default NOTIFY actuallyActiveProfileChanged BINDABLE bindableActuallyActiveProfile)
    Q_PROPERTY(QString inhibitionReason READ default NOTIFY inhibitionReasonChanged BINDABLE bindableInhibitionReason)
    Q_PROPERTY(QString degradationReason READ default NOTIFY degradationReasonChanged BINDABLE bindableDegradationReason)

public:
    explicit PowerProfilesControl(QObject *parent = nullptr);
    ~PowerProfilesControl();

    Q_INVOKABLE bool setActuallyActiveProfile(QString profile);

private:
    QBindable<bool> bindableIsPowerProfileDaemonInstalled();
    QBindable<QStringList> bindableProfiles();
    QBindable<QList<QVariantMap>> bindableActiveProfileHolds();
    QBindable<QString> bindableActuallyActiveProfile();
    QBindable<QString> bindableInhibitionReason();
    QBindable<QString> bindableDegradationReason();

private Q_SLOTS:
    void updatePowerProfileCurrentProfile(QString profile);
    void updatePowerProfileChoices(QStringList choices);
    void updatePowerProfilePerformanceInhibitedReason(QString reason);
    void updatePowerProfilePerformanceDegradedReason(QString reason);
    void updatePowerProfileHolds(QList<QVariantMap> holds);

Q_SIGNALS:
    void isPowerProfileDaemonInstalledChanged(bool status);
    void profilesChanged(QStringList profiles);
    void activeProfileHoldsChanged(QList<QVariantMap>);
    void actuallyActiveProfileChanged(QString profile);
    void inhibitionReasonChanged(QString reason);
    void degradationReasonChanged(QString reason);

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerProfilesControl,
                                         bool,
                                         m_isPowerProfileDaemonInstalled,
                                         false,
                                         &PowerProfilesControl::isPowerProfileDaemonInstalledChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QStringList, m_profiles, &PowerProfilesControl::profilesChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QList<QVariantMap>, m_activeProfileHolds, &PowerProfilesControl::activeProfileHoldsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QString, m_actuallyActiveProfile, &PowerProfilesControl::actuallyActiveProfileChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QString, m_inhibitionReason, &PowerProfilesControl::inhibitionReasonChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QString, m_degradationReason, &PowerProfilesControl::degradationReasonChanged)

    ApplicationData m_data;
};
