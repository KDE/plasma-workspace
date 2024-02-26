/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "powerprofilescontrol.h"

#include "batterymonitor_debug.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QIcon>

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";

PowerProfilesControl::PowerProfilesControl(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<QVariantMap>();

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        if (!QDBusConnection::systemBus().interface()->isServiceRegistered(QStringLiteral("net.hadess.PowerProfiles"))) {
            return;
        }
        QDBusMessage profileChoices = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                     QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                     QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                     QStringLiteral("profileChoices"));

        QDBusReply<QStringList> profileChoicesReply = QDBusConnection::sessionBus().call(profileChoices);

        if (profileChoicesReply.isValid()) {
            updatePowerProfileChoices(profileChoicesReply.value());
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "error getting profile choices";
        }

        QDBusMessage currentProfile = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                     QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                     QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                     QStringLiteral("currentProfile"));
        QDBusReply<QString> currentProfileReply = QDBusConnection::sessionBus().call(currentProfile);

        if (currentProfileReply.isValid()) {
            updatePowerProfileCurrentProfile(currentProfileReply.value());
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "error getting current profile";
        }

        QDBusMessage inhibitionReason = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                       QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                       QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                       QStringLiteral("performanceInhibitedReason"));
        QDBusReply<QString> inhibitionReasonReply = QDBusConnection::sessionBus().call(inhibitionReason);

        if (inhibitionReasonReply.isValid()) {
            updatePowerProfilePerformanceInhibitedReason(inhibitionReasonReply.value());
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "error getting performance inhibited reason";
        }

        QDBusMessage degradedReason = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                     QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                     QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                     QStringLiteral("performanceDegradedReason"));
        QDBusReply<QString> degradedReasonReply = QDBusConnection::sessionBus().call(inhibitionReason);

        if (degradedReasonReply.isValid()) {
            updatePowerProfilePerformanceDegradedReason(inhibitionReasonReply.value());
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "error getting performance inhibited reason";
        }

        QDBusMessage profileHolds = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                   QStringLiteral("profileHolds"));
        QDBusReply<QList<QVariantMap>> profileHoldsReply = QDBusConnection::sessionBus().call(profileHolds);

        if (profileHoldsReply.isValid()) {
            updatePowerProfileHolds(profileHoldsReply.value());
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "error getting profile holds";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("currentProfileChanged"),
                                                   this,
                                                   SLOT(updatePowerProfileCurrentProfile(QString)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to current profile changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("profileChoicesChanged"),
                                                   this,
                                                   SLOT(updatePowerProfileChoices(QStringList)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to profile choices changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("performanceInhibitedReasonChanged"),
                                                   this,
                                                   SLOT(updatePowerProfilePerformanceInhibitedReason(QString)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to inhibition reason changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("performanceDegradedReasonChanged"),
                                                   this,
                                                   SLOT(updatePowerProfilePerformanceDegradedReason(QString)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to degradation reason changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("profileHoldsChanged"),
                                                   this,
                                                   SLOT(updatePowerProfileHolds(QList<QVariantMap>)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to profile hold changes via dbus";
        }
        m_isPowerProfileDaemonInstalled = true;
    }
}

PowerProfilesControl::~PowerProfilesControl()
{
}

QBindable<bool> PowerProfilesControl::bindableIsPowerProfileDaemonInstalled()
{
    return &m_isPowerProfileDaemonInstalled;
}

QBindable<QStringList> PowerProfilesControl::bindableProfiles()
{
    return &m_profiles;
}

QBindable<QList<QVariantMap>> PowerProfilesControl::bindableActiveProfileHolds()
{
    return &m_activeProfileHolds;
}

QBindable<QString> PowerProfilesControl::bindableActuallyActiveProfile()
{
    return &m_actuallyActiveProfile;
}

QBindable<QString> PowerProfilesControl::bindableInhibitionReason()
{
    return &m_inhibitionReason;
}

QBindable<QString> PowerProfilesControl::bindableDegradationReason()
{
    return &m_degradationReason;
}

bool PowerProfilesControl::setActuallyActiveProfile(QString profile)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                      QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                      QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                      QStringLiteral("setProfile"));
    msg << profile;
    QDBusReply<void> reply = QDBusConnection::sessionBus().call(msg);
    if (reply.isValid()) {
        return true;
    }
    Q_EMIT actuallyActiveProfileChanged(m_actuallyActiveProfile);
    return false;
}

void PowerProfilesControl::updatePowerProfileCurrentProfile(QString activeProfile)
{
    m_actuallyActiveProfile = activeProfile;
}

void PowerProfilesControl::updatePowerProfileChoices(QStringList choices)
{
    m_profiles = choices;
}

void PowerProfilesControl::updatePowerProfilePerformanceInhibitedReason(QString reason)
{
    m_inhibitionReason = reason;
}

void PowerProfilesControl::updatePowerProfilePerformanceDegradedReason(QString reason)
{
    m_degradationReason = reason;
}

void PowerProfilesControl::updatePowerProfileHolds(QList<QVariantMap> holds)
{
    QList<QVariantMap> out;
    std::transform(holds.cbegin(), holds.cend(), std::back_inserter(out), [this](const QVariantMap &hold) {
        QString prettyName;
        QString icon;

        m_data.populateApplicationData(hold[QStringLiteral("ApplicationId")].toString(), &prettyName, &icon);

        return QVariantMap{
            {QStringLiteral("Name"), prettyName},
            {QStringLiteral("Icon"), icon},
            {QStringLiteral("Reason"), hold[QStringLiteral("Reason")]},
            {QStringLiteral("Profile"), hold[QStringLiteral("Profile")]},
        };
    });
    m_activeProfileHolds = out;

    Q_EMIT activeProfileHoldsChanged(m_activeProfileHolds);
}
