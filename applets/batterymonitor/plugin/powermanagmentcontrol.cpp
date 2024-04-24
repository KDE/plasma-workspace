/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "powermanagmentcontrol.h"

#include "batterymonitor_debug.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QString>

#include <KService>
#include <qcontainerfwd.h>

#include "inhibitmonitor_p.h"

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";
static const char FDO_POWERMANAGEMENT_SERVICE[] = "org.freedesktop.PowerManagement";

PowerManagmentControl::PowerManagmentControl(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<QVariantMap>();

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        m_isManuallyInhibited = InhibitMonitor::self().getInhibit();
        connect(&InhibitMonitor::self(), &InhibitMonitor::isManuallyInhibitedChanged, this, &PowerManagmentControl::onIsManuallyInhibitedChanged);
        connect(&InhibitMonitor::self(), &InhibitMonitor::isManuallyInhibitedChangeError, this, &PowerManagmentControl::onisManuallyInhibitedErrorChanged);

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
                                                   SLOT(onInhibitionsChanged(QList<uint>, QList<uint>)))) {
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

PowerManagmentControl::~PowerManagmentControl()
{
}

void PowerManagmentControl::inhibit(const QString &reason)
{
    InhibitMonitor::self().inhibit(reason, m_isSilent);
}

void PowerManagmentControl::uninhibit()
{
    InhibitMonitor::self().uninhibit(m_isSilent);
}

void PowerManagmentControl::releaseInhibition(uint cookie)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                      QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                      QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                      QStringLiteral("ReleaseInhibition"));
    msg << cookie;
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
}

bool PowerManagmentControl::isSilent()
{
    return m_isSilent;
}

void PowerManagmentControl::setIsSilent(bool status)
{
    m_isSilent = status;
}

QBindable<QList<QVariantMap>> PowerManagmentControl::bindableInhibitions()
{
    return &m_inhibitions;
}

QBindable<bool> PowerManagmentControl::bindableHasInhibition()
{
    return &m_hasInhibition;
}

QBindable<bool> PowerManagmentControl::bindableIsLidPresent()
{
    return &m_isLidPresent;
}

QBindable<bool> PowerManagmentControl::bindableTriggersLidAction()
{
    return &m_triggersLidAction;
}

QBindable<bool> PowerManagmentControl::bindableIsManuallyInhibited()
{
    return &m_isManuallyInhibited;
}

QBindable<bool> PowerManagmentControl::bindableIsManuallyInhibitedError()
{
    return &m_isManuallyInhibitedError;
}

void PowerManagmentControl::onInhibitionsChanged(const QList<uint> &added, const QList<uint> &removed)
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

void PowerManagmentControl::onHasInhibitionChanged(bool status)
{
    m_hasInhibition = status;
}

void PowerManagmentControl::onIsManuallyInhibitedChanged(bool status)
{
    m_isManuallyInhibited = status;
}

void PowerManagmentControl::onisManuallyInhibitedErrorChanged(bool status)
{
    m_isManuallyInhibitedError = status;
}

void PowerManagmentControl::updateInhibitions(const QList<SolidInhibition> &inhibitions)
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

#include "moc_powermanagmentcontrol.cpp"
