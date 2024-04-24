/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "powermanagementcontrol.h"

#include "batterymonitor_debug.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QString>

#include <KService>

#include "inhibitmonitor_p.h"

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";
static const char FDO_POWERMANAGEMENT_SERVICE[] = "org.freedesktop.PowerManagement";

QDBusArgument &operator<<(QDBusArgument &argument, const SolidInhibition &inhibition)
{
    argument.beginStructure();
    argument << inhibition.cookie << inhibition.appName << inhibition.reason;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, SolidInhibition &inhibition)
{
    argument.beginStructure();
    argument >> inhibition.cookie >> inhibition.appName >> inhibition.reason;
    argument.endStructure();
    return argument;
}

PowerManagementControl::PowerManagementControl(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<QList<SolidInhibition>>();
    qDBusRegisterMetaType<SolidInhibition>();

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        m_isManuallyInhibited = InhibitMonitor::self().getInhibit();
        connect(&InhibitMonitor::self(), &InhibitMonitor::isManuallyInhibitedChanged, this, &PowerManagementControl::onIsManuallyInhibitedChanged);
        connect(&InhibitMonitor::self(), &InhibitMonitor::isManuallyInhibitedChangeError, this, &PowerManagementControl::onisManuallyInhibitedErrorChanged);

        QDBusMessage isLidPresentMessage = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                          QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                                          SOLID_POWERMANAGEMENT_SERVICE,
                                                                          QStringLiteral("isLidPresent"));
        QDBusPendingCall isLidPresentCall = QDBusConnection::sessionBus().asyncCall(isLidPresentMessage);
        auto isLidPresentWatcher = new QDBusPendingCallWatcher(isLidPresentCall, this);
        connect(isLidPresentWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<bool> reply = *watcher;
            if (reply.isValid()) {
                m_isLidPresent = reply.value();

                QDBusMessage triggersLidActionMessage =
                    QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/HandleButtonEvents"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.HandleButtonEvents"),
                                                   QStringLiteral("triggersLidAction"));
                QDBusPendingCall triggersLidActionCall = QDBusConnection::sessionBus().asyncCall(triggersLidActionMessage);
                auto triggersLidActionWatcher = new QDBusPendingCallWatcher(triggersLidActionCall, this);
                connect(triggersLidActionWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
                    QDBusReply<bool> reply = *watcher;
                    if (reply.isValid()) {
                        m_triggersLidAction = reply.value();
                    }
                    watcher->deleteLater();
                });
                if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                           QStringLiteral("/org/kde/Solid/PowerManagement/Actions/HandleButtonEvents"),
                                                           QStringLiteral("org.kde.Solid.PowerManagement.Actions.HandleButtonEvents"),
                                                           QStringLiteral("triggersLidActionChanged"),
                                                           this,
                                                           SLOT(triggersLidActionChanged(bool)))) {
                    qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to lid action trigger changes via dbus";
                }
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "Lid is not present";
            }
            watcher->deleteLater();
        });

        onInhibitionsChanged({}, {});

        QDBusMessage hasInhibitMessage = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                        QStringLiteral("/org/freedesktop/PowerManagement"),
                                                                        QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                                        QStringLiteral("HasInhibit"));
        QDBusPendingCall hasInhibitReply = QDBusConnection::sessionBus().asyncCall(hasInhibitMessage);
        auto *hasInhibitWatcher = new QDBusPendingCallWatcher(hasInhibitReply, this);
        connect(hasInhibitWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<bool> reply = *watcher;
            if (reply.isValid()) {
                m_hasInhibition = reply.value();
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "Failed to retrive has inhibit";
            }
            watcher->deleteLater();
        });

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                   QStringLiteral("InhibitionsChanged"),
                                                   this,
                                                   SLOT(onInhibitionsChanged(QList<SolidInhibition>, QList<uint>)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "Error connecting to inhibition changes via dbus";
        }
    }

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(FDO_POWERMANAGEMENT_SERVICE)) {
        if (!QDBusConnection::sessionBus().connect(FDO_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/freedesktop/PowerManagement"),
                                                   QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                   QStringLiteral("HasInhibitChanged"),
                                                   this,
                                                   SLOT(onHasInhibitionChanged(bool)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "Error connecting to fdo inhibition changes via dbus";
        }
    }
}

PowerManagementControl::~PowerManagementControl()
{
}

void PowerManagementControl::inhibit(const QString &reason)
{
    InhibitMonitor::self().inhibit(reason, m_isSilent);
}

void PowerManagementControl::uninhibit()
{
    InhibitMonitor::self().uninhibit(m_isSilent);
}

void PowerManagementControl::releaseInhibition(uint cookie)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                      QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                      QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                      QStringLiteral("ReleaseInhibition"));
    msg << cookie;
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
}

bool PowerManagementControl::isSilent()
{
    return m_isSilent;
}

void PowerManagementControl::setIsSilent(bool status)
{
    m_isSilent = status;
}

QBindable<QList<QVariantMap>> PowerManagementControl::bindableInhibitions()
{
    return &m_inhibitions;
}

QBindable<bool> PowerManagementControl::bindableHasInhibition()
{
    return &m_hasInhibition;
}

QBindable<bool> PowerManagementControl::bindableIsLidPresent()
{
    return &m_isLidPresent;
}

QBindable<bool> PowerManagementControl::bindableTriggersLidAction()
{
    return &m_triggersLidAction;
}

QBindable<bool> PowerManagementControl::bindableIsManuallyInhibited()
{
    return &m_isManuallyInhibited;
}

QBindable<bool> PowerManagementControl::bindableIsManuallyInhibitedError()
{
    return &m_isManuallyInhibitedError;
}

void PowerManagementControl::onInhibitionsChanged(const QList<SolidInhibition> &added, const QList<uint> &removed)
{
    Q_UNUSED(added);
    Q_UNUSED(removed);

    QDBusMessage inhibitionsMessage = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                     QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                                     QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                                     QStringLiteral("ListInhibitions"));
    QDBusPendingCall inhibitionsCall = QDBusConnection::sessionBus().asyncCall(inhibitionsMessage);
    auto inhibitionsWatcher = new QDBusPendingCallWatcher(inhibitionsCall, this);
    connect(inhibitionsWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusReply<QList<SolidInhibition>> reply = *watcher;
        if (reply.isValid()) {
            updateInhibitions(reply.value());
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "Failed to retrive inhibitions";
        }
        watcher->deleteLater();
    });
}

void PowerManagementControl::onHasInhibitionChanged(bool status)
{
    m_hasInhibition = status;
}

void PowerManagementControl::onIsManuallyInhibitedChanged(bool status)
{
    m_isManuallyInhibited = status;
}

void PowerManagementControl::onisManuallyInhibitedErrorChanged(bool status)
{
    m_isManuallyInhibitedError = status;
}

void PowerManagementControl::updateInhibitions(const QList<SolidInhibition> &inhibitions)
{
    QList<QVariantMap> out;

    for (auto it = inhibitions.constBegin(); it != inhibitions.constEnd(); ++it) {
        const uint cookie = (*it).cookie;
        const QString &name = (*it).appName;
        if (name == QStringLiteral("plasmashell") || name == QStringLiteral("org.kde.plasmashell")) {
            continue;
        }
        QString prettyName;
        QString icon;
        const QString &reason = (*it).reason;

        m_data.populateApplicationData(name, &prettyName, &icon);

        out.append(QVariantMap{{QStringLiteral("Cookie"), cookie},
                               {QStringLiteral("Name"), name},
                               {QStringLiteral("PrettyName"), prettyName},
                               {QStringLiteral("Icon"), icon},
                               {QStringLiteral("Reason"), reason}});
    }

    m_inhibitions = out;
}

#include "moc_powermanagementcontrol.cpp"
