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
    Q_PROPERTY(
        QString actuallyActiveProfileError READ default WRITE default NOTIFY actuallyActiveProfileErrorChanged BINDABLE bindableActuallyActiveProfileError)
    Q_PROPERTY(QString inhibitionReason READ default NOTIFY inhibitionReasonChanged BINDABLE bindableInhibitionReason)
    Q_PROPERTY(QString degradationReason READ default NOTIFY degradationReasonChanged BINDABLE bindableDegradationReason)
    Q_PROPERTY(bool isManuallyInPerformanceMode READ default NOTIFY isManuallyInPerformanceModeChanged BINDABLE
                   bindableIsManuallyInPerformanceMode) // to be set on power profile requested through the applet
    Q_PROPERTY(bool isManuallyInPowerSaveMode READ default NOTIFY isManuallyInPowerSaveModeChanged BINDABLE
                   bindableIsManuallyInPowerSaveMode) // to be set on power profile requested through the applet)
    Q_PROPERTY(bool isSilent READ isSilent WRITE setIsSilent)

public:
    Q_INVOKABLE void setActuallyActiveProfile(const QString &reason);

    explicit PowerProfilesControl(QObject *parent = nullptr);
    ~PowerProfilesControl() override;

private:
    bool isSilent();
    void setIsSilent(bool status);
    void showPowerProfileOsd(const QString &reason);

    QBindable<bool> bindableIsPowerProfileDaemonInstalled();
    QBindable<bool> bindableIsManuallyInPerformanceMode();
    QBindable<bool> bindableIsManuallyInPowerSaveMode();
    QBindable<QStringList> bindableProfiles();
    QBindable<QList<QVariantMap>> bindableActiveProfileHolds();
    QBindable<QString> bindableActuallyActiveProfile();
    QBindable<QString> bindableActuallyActiveProfileError();
    QBindable<QString> bindableInhibitionReason();
    QBindable<QString> bindableDegradationReason();

private Q_SLOTS:
    void updatePowerProfileCurrentProfile(const QString &profile);
    void updatePowerProfileChoices(const QStringList &choices);
    void updatePowerProfilePerformanceInhibitedReason(const QString &reason);
    void updatePowerProfilePerformanceDegradedReason(const QString &reason);
    void updatePowerProfileHolds(QList<QVariantMap> holds);

Q_SIGNALS:
    void isPowerProfileDaemonInstalledChanged(bool status);
    void isManuallyInPerformanceModeChanged(bool status);
    void isManuallyInPowerSaveModeChanged(bool status);
    void profilesChanged(const QStringList &profiles);
    void activeProfileHoldsChanged(const QList<QVariantMap> &profileHolds);
    void actuallyActiveProfileChanged(const QString &profile);
    void actuallyActiveProfileErrorChanged(const QString &status);
    void inhibitionReasonChanged(const QString &reason);
    void degradationReasonChanged(const QString &reason);

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerProfilesControl,
                                         bool,
                                         m_isPowerProfileDaemonInstalled,
                                         false,
                                         &PowerProfilesControl::isPowerProfileDaemonInstalledChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerProfilesControl,
                                         bool,
                                         m_isManuallyInPerformanceMode,
                                         false,
                                         &PowerProfilesControl::isManuallyInPerformanceModeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerProfilesControl,
                                         bool,
                                         m_isManuallyInPowerSaveMode,
                                         false,
                                         &PowerProfilesControl::isManuallyInPowerSaveModeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QStringList, m_profiles, &PowerProfilesControl::profilesChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QList<QVariantMap>, m_activeProfileHolds, &PowerProfilesControl::activeProfileHoldsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QString, m_actuallyActiveProfile, &PowerProfilesControl::actuallyActiveProfileChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QString, m_actuallyActiveProfileError, &PowerProfilesControl::actuallyActiveProfileErrorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QString, m_inhibitionReason, &PowerProfilesControl::inhibitionReasonChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerProfilesControl, QString, m_degradationReason, &PowerProfilesControl::degradationReasonChanged)

    bool m_isSilent = false;

    ApplicationData m_data;
};
