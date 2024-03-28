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

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";
static const char FDO_POWERMANAGEMENT_SERVICE[] = "org.freedesktop.PowerManagement";

PowerManagmentControl::PowerManagmentControl(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<QList<InhibitionInfo>>();
    qDBusRegisterMetaType<InhibitionInfo>();

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        QDBusMessage isLidPresentCall = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                       QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                                       SOLID_POWERMANAGEMENT_SERVICE,
                                                                       QStringLiteral("isLidPresent"));
        QDBusReply<bool> isLidPresentReply = QDBusConnection::sessionBus().call(isLidPresentCall);

        if (isLidPresentReply.isValid()) {
            m_isLidPresent = isLidPresentReply.value();

            QDBusMessage triggersLidActionCall = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                                QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                                                SOLID_POWERMANAGEMENT_SERVICE,
                                                                                QStringLiteral("triggersLidAction"));
            QDBusReply<bool> triggersLidActionReply = QDBusConnection::sessionBus().call(triggersLidActionCall);

            if (triggersLidActionReply.isValid()) {
                m_triggersLidAction = triggersLidActionReply.value();
            }

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

        QDBusMessage inhibitionsCall = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                      QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                                      QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                                      QStringLiteral("ListInhibitions"));
        QDBusReply<QList<InhibitionInfo>> inhibitionsReply = QDBusConnection::sessionBus().call(inhibitionsCall);

        if (inhibitionsReply.isValid()) {
            onInhibitionsChanged(inhibitionsReply.value(), {});
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "Failed to retrive inhibitions";
        }

        QDBusMessage hasInhibitCall = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                     QStringLiteral("/org/freedesktop/PowerManagement"),
                                                                     QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                                     QStringLiteral("HasInhibit"));
        QDBusReply<bool> hasInhibitReply = QDBusConnection::sessionBus().call(hasInhibitCall);

        if (hasInhibitReply.isValid()) {
            m_hasInhibition = hasInhibitReply.value();
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "Failed to retrive has inhibit";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                   QStringLiteral("InhibitionsChanged"),
                                                   this,
                                                   SLOT(onInhibitionsChanged(QList<InhibitionInfo>, QStringList)))) {
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
    if (m_sleepInhibitionCookie != -1) {
        stopSuppressingSleep();
    }
    if (m_lockInhibitionCookie != -1) {
        stopSuppressingScreenPowerManagement();
    }
}

void PowerManagmentControl::beginSuppressingSleep(QString reason)
{
    if (m_sleepInhibitionCookie != -1) { // an inhibition request is already active; don't trigger another one
        return;
    }
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                                                      QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("Inhibit"));
    msg << QCoreApplication::applicationName() << reason;
    QDBusReply<uint> reply = QDBusConnection::sessionBus().call(msg);
    m_sleepInhibitionCookie = reply.isValid() ? reply.value() : -1;
}

void PowerManagmentControl::stopSuppressingSleep()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                                                      QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("UnInhibit"));
    msg << m_sleepInhibitionCookie;
    QDBusReply<void> reply = QDBusConnection::sessionBus().call(msg);
    m_sleepInhibitionCookie = reply.isValid() ? -1 : m_sleepInhibitionCookie; // reset cookie if the stop request was successful
}

void PowerManagmentControl::beginSuppressingScreenPowerManagement(QString reason)
{
    if (m_lockInhibitionCookie != -1) { // an inhibition request is already active; don't trigger another one
        return;
    }
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                      QStringLiteral("/ScreenSaver"),
                                                      QStringLiteral("org.freedesktop.ScreenSaver"),
                                                      QStringLiteral("Inhibit"));
    msg << QCoreApplication::applicationName() << reason;
    QDBusReply<uint> reply = QDBusConnection::sessionBus().call(msg);
    m_lockInhibitionCookie = reply.isValid() ? reply.value() : -1;
}

void PowerManagmentControl::stopSuppressingScreenPowerManagement()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                      QStringLiteral("/ScreenSaver"),
                                                      QStringLiteral("org.freedesktop.ScreenSaver"),
                                                      QStringLiteral("UnInhibit"));
    msg << m_lockInhibitionCookie;
    QDBusReply<void> reply = QDBusConnection::sessionBus().call(msg);
    m_lockInhibitionCookie = reply.isValid() ? -1 : m_lockInhibitionCookie; // reset cookie if the stop request was successful
}

void PowerManagmentControl::releaseInhibition(uint cookie)
{
    QDBusMessage inhibitionsCall = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                  QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                                  QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                                  QStringLiteral("ReleaseInhibition"));
    msg << cookie;
    QDBusReply<void> reply = QDBusConnection::sessionBus().call(msg);
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

void PowerManagmentControl::onInhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed)
{
    QList<QVariantMap> out = m_inhibitions;

    for (auto removedIt = removed.constBegin(); removedIt != removed.constEnd(); ++removedIt) {
        for (int index = 0; index < out.size(); ++index) {
            if (out[index][QStringLiteral("Name")] == *removedIt && *removedIt != QStringLiteral("plasmashell")
                && *removedIt != QStringLiteral("plasmoidviewer")) {
                out.removeAt(index);
                break;
            }
        }
    }

    for (auto it = added.constBegin(); it != added.constEnd(); ++it) {
        const QString &name = (*it).first;
        if (name == QStringLiteral("plasmashell") || name == QStringLiteral("plasmoidviewer")) {
            continue;
        }
        QString prettyName;
        QString icon;
        const QString &reason = (*it).second;

        m_data.populateApplicationData(name, &prettyName, &icon);

        out.append(QVariantMap{{QStringLiteral("Name"), name},
                               {QStringLiteral("PrettyName"), prettyName},
                               {QStringLiteral("Icon"), icon},
                               {QStringLiteral("Reason"), reason}});
    }

    m_inhibitions = out;
}

void PowerManagmentControl::onHasInhibitionChanged(bool status)
{
    m_hasInhibition = status;
}

#include "moc_powermanagmentcontrol.cpp"
