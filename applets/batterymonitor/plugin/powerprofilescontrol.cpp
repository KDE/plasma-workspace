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
#include <sys/socket.h>

static constexpr QLatin1StringView SOLID_POWERMANAGEMENT_SERVICE("org.kde.Solid.PowerManagement");

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
        QDBusPendingCall profileChoicesCall = QDBusConnection::sessionBus().asyncCall(profileChoices);
        auto profileChoicesWatcher = new QDBusPendingCallWatcher(profileChoicesCall, this);
        connect(profileChoicesWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<QStringList> reply = *watcher;

            if (reply.isValid()) {
                updatePowerProfileChoices(reply.value());
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "error getting profile choices";
            }
            watcher->deleteLater();
        });

        QDBusMessage configuredProfile = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                     QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                     QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                     QStringLiteral("configuredProfile"));
        QDBusPendingCall configuredProfileCall = QDBusConnection::sessionBus().asyncCall(configuredProfile);
        auto configuredProfileWatcher = new QDBusPendingCallWatcher(configuredProfileCall, this);
        connect(configuredProfileWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<QString> reply = *watcher;
            if (reply.isValid()) {
                updatePowerProfileConfiguredProfile(reply.value());
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "error getting current profile";
            }
            watcher->deleteLater();
        });

        QDBusMessage currentProfile = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                     QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                     QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                     QStringLiteral("currentProfile"));
        QDBusPendingCall currentProfileCall = QDBusConnection::sessionBus().asyncCall(currentProfile);
        auto currentProfileWatcher = new QDBusPendingCallWatcher(currentProfileCall, this);
        connect(currentProfileWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<QString> reply = *watcher;
            if (reply.isValid()) {
                updatePowerProfileCurrentProfile(reply.value());
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "error getting current profile";
            }
            watcher->deleteLater();
        });

        QDBusMessage inhibitionReason = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                       QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                       QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                       QStringLiteral("performanceInhibitedReason"));
        QDBusPendingCall inhibitionReasonCall = QDBusConnection::sessionBus().asyncCall(inhibitionReason);
        auto inhibitionReasonWatcher = new QDBusPendingCallWatcher(inhibitionReasonCall, this);
        connect(inhibitionReasonWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<QString> reply;
            if (reply.isValid()) {
                updatePowerProfilePerformanceInhibitedReason(reply.value());
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "error getting performance inhibited reason";
            }
            watcher->deleteLater();
        });

        QDBusMessage degradedReason = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                     QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                     QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                     QStringLiteral("performanceDegradedReason"));
        QDBusPendingCall degradedReasonCall = QDBusConnection::sessionBus().asyncCall(inhibitionReason);
        auto degradedReasonWatcher = new QDBusPendingCallWatcher(degradedReasonCall, this);
        connect(degradedReasonWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<QString> reply = *watcher;
            if (reply.isValid()) {
                updatePowerProfilePerformanceDegradedReason(reply.value());
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "error getting performance inhibited reason";
            }
            watcher->deleteLater();
        });

        QDBusMessage profileHolds = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                                   QStringLiteral("profileHolds"));
        QDBusPendingCall profileHoldsCall = QDBusConnection::sessionBus().asyncCall(profileHolds);
        auto profileHoldsWatcher = new QDBusPendingCallWatcher(profileHoldsCall, this);
        connect(profileHoldsWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<QList<QVariantMap>> reply = *watcher;
            if (reply.isValid()) {
                updatePowerProfileHolds(reply.value());
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "error getting profile holds";
            }
            watcher->deleteLater();
        });

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("configuredProfileChanged"),
                                                   this,
                                                   SLOT(updatePowerProfileConfiguredProfile(QString)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to current profile changes via dbus";
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

bool PowerProfilesControl::isSilent()
{
    return m_isSilent;
}

void PowerProfilesControl::setIsSilent(bool status)
{
    m_isSilent = status;
}

QBindable<bool> PowerProfilesControl::bindableIsPowerProfileDaemonInstalled()
{
    return &m_isPowerProfileDaemonInstalled;
}

QBindable<QStringList> PowerProfilesControl::bindableProfiles()
{
    return &m_profiles;
}

QBindable<QString> PowerProfilesControl::bindableConfiguredProfile()
{
    return &m_configuredProfile;
}

QBindable<QString> PowerProfilesControl::bindableActiveProfile()
{
    return &m_activeProfile;
}

QBindable<QString> PowerProfilesControl::bindableProfileError()
{
    return &m_profileError;
}

QBindable<QString> PowerProfilesControl::bindableInhibitionReason()
{
    return &m_inhibitionReason;
}

QBindable<QString> PowerProfilesControl::bindableDegradationReason()
{
    return &m_degradationReason;
}

QBindable<QList<QVariantMap>> PowerProfilesControl::bindableProfileHolds()
{
    return &m_profileHolds;
}

void PowerProfilesControl::setProfile(const QString &profile)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Solid.PowerManagement"),
                                                      QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                      QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                      QStringLiteral("setProfile"));
    msg << profile;
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, profile](QDBusPendingCallWatcher *watcher) {
        QDBusReply<void> reply = *watcher;
        if (reply.isValid()) {
            m_activeProfile = profile;
            showPowerProfileOsd(profile);
        } else {
            m_profileError = profile;
        }
        watcher->deleteLater();
    });
}

void PowerProfilesControl::showPowerProfileOsd(const QString &profile)
{
    if (isSilent()) {
        return;
    }
    QDBusMessage osdMsg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                         QStringLiteral("/org/kde/osdService"),
                                                         QStringLiteral("org.kde.osdService"),
                                                         QStringLiteral("powerProfileChanged"));
    osdMsg << profile;
    QDBusConnection::sessionBus().asyncCall(osdMsg);
}

void PowerProfilesControl::updatePowerProfileChoices(const QStringList &choices)
{
    m_profiles = choices;
}

void PowerProfilesControl::updatePowerProfileConfiguredProfile(const QString &configuredProfile)
{
    m_configuredProfile = configuredProfile;
}

void PowerProfilesControl::updatePowerProfileCurrentProfile(const QString &activeProfile)
{
    m_activeProfile = activeProfile;
}

void PowerProfilesControl::updatePowerProfilePerformanceInhibitedReason(const QString &reason)
{
    m_inhibitionReason = reason;
}

void PowerProfilesControl::updatePowerProfilePerformanceDegradedReason(const QString &reason)
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
    m_profileHolds = std::move(out);

    Q_EMIT profileHoldsChanged(m_profileHolds);
}
