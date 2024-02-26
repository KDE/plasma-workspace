/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "batterycontrol.h"

#include "batterymonitor_debug.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>

#include <klocalizedstring.h>

#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";

BatteryControl::BatteryControl(QObject *parent)
    : QObject(parent)
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        const QList<Solid::Device> listBattery = Solid::Device::listFromType(Solid::DeviceInterface::Battery);

        if (!listBattery.isEmpty()) {
            for (const Solid::Device &deviceBattery : listBattery) {
                deviceAdded(deviceBattery.udi());
            }

            m_hasBattery = true;
            updateOverallBattery();

            QDBusMessage batteryRemainingTimeCall = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                                   QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                                                   SOLID_POWERMANAGEMENT_SERVICE,
                                                                                   QStringLiteral("batteryRemainingTime"));
            QDBusReply<qulonglong> batteryRemainingTimeReply = QDBusConnection::sessionBus().call(batteryRemainingTimeCall);

            if (!batteryRemainingTimeReply.isValid()) {
                m_remainingMsec = batteryRemainingTimeReply.value();
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "error getting battery remaining time";
            }

            QDBusMessage smoothedBatteryRemainingTimeCall = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                                           QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                                                           SOLID_POWERMANAGEMENT_SERVICE,
                                                                                           QStringLiteral("smoothedBatteryRemainingTime"));
            QDBusReply<qulonglong> smoothedBatteryRemainingTimeReply = QDBusConnection::sessionBus().call(smoothedBatteryRemainingTimeCall);

            if (smoothedBatteryRemainingTimeReply.isValid()) {
                m_smoothedRemainingMsec = batteryRemainingTimeReply.value();
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "error getting smoothed battery remaining time";
            }

        } else {
            m_hasBattery = false;
            m_hasCumulative = false;
        }

        connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &BatteryControl::deviceAdded);
        connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &BatteryControl::deviceRemoved);

        QDBusMessage chargeStopThresholdCall = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                              QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                                              SOLID_POWERMANAGEMENT_SERVICE,
                                                                              QStringLiteral("chargeStopThreshold"));
        QDBusReply<int> chargeStopThresholdReply = QDBusConnection::sessionBus().call(chargeStopThresholdCall);

        if (chargeStopThresholdReply.isValid()) {
            m_chargeStopThreshold = chargeStopThresholdReply.value();
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "error getting charge stop threshold";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                   SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("batteryRemainingTimeChanged"),
                                                   this,
                                                   SLOT(batteryRemainingTimeChanged(qulonglong)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to remaining time changes";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                   SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("smoothedBatteryRemainingTimeChanged"),
                                                   this,
                                                   SLOT(smoothedBatteryRemainingTimeChanged(qulonglong)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to smoothed remaining time changes";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                   SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("chargeStopThresholdChanged"),
                                                   this,
                                                   SLOT(chargeStopThresholdChanged(int)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to charge stop threshold changes via dbus";
        }

        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement"),
                                                          QStringLiteral("/org/freedesktop/PowerManagement"),
                                                          QStringLiteral("org.freedesktop.PowerManagement"),
                                                          QStringLiteral("GetPowerSaveStatus"));
        QDBusReply<bool> reply = QDBusConnection::sessionBus().call(msg);

        if (reply.isValid()) {
            updateAcPlugState(reply.value());
        } else {
            qCDebug(APPLETS::BATTERYMONITOR) << "Fail to retrive power save status";
        }

        if (!QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.PowerManagement"),
                                                   QStringLiteral("/org/freedesktop/PowerManagement"),
                                                   QStringLiteral("org.freedesktop.PowerManagement"),
                                                   QStringLiteral("PowerSaveStatusChanged"),
                                                   this,
                                                   SLOT(updateAcPlugState(bool)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to power save status changes via dbus";
        }
    }
}

BatteryControl::~BatteryControl()
{
}

void BatteryControl::deviceRemoved(const QString &udi)
{
    qCDebug(APPLETS::BATTERYMONITOR) << "Device remove signal arrived. Udi: " << udi;
    if (m_batterySources.isEmpty()) {
        return;
    }

    for (int position = 0; position < m_batterySources.size(); ++position) {
        QObject *object = m_batterySources[position];
        auto deleteBattery = qobject_cast<BatteryUnit *>(object);

        if (deleteBattery->udi() == udi) {
            qCDebug(APPLETS::BATTERYMONITOR) << "Battery with udi: " << deleteBattery->udi() << " is removed";

            disconnect(deleteBattery, &BatteryUnit::batteryStatusChanged, this, &BatteryControl::updateOverallBattery);
            deleteBattery->deleteLater();
            m_batterySources.removeAt(position);
            Q_EMIT batteriesChanged(m_batterySources);

            m_hasBattery = !m_batterySources.isEmpty();

            updateOverallBattery();

            return;
        }
    }
}

void BatteryControl::deviceAdded(const QString &udi)
{
    qCDebug(APPLETS::BATTERYMONITOR) << "Device add signal arrived. Udi: " << udi;

    Solid::Device deviceBattery(udi);
    if (deviceBattery.isValid()) {
        const Solid::Battery *battery = deviceBattery.as<Solid::Battery>();
        if (battery) {
            auto newBattery = new BatteryUnit(deviceBattery, this);

            m_batterySources.append(newBattery);

            qCDebug(APPLETS::BATTERYMONITOR) << "Battery with udi: " << newBattery->udi() << " is added";

            connect(newBattery, &BatteryUnit::batteryStatusChanged, this, &BatteryControl::updateOverallBattery);

            m_hasBattery = true;

            Q_EMIT batteriesChanged(m_batterySources);

            updateOverallBattery();
        }
    }
}

void BatteryControl::batteryRemainingTimeChanged(qulonglong time)
{
    // qCDebug(APPLETS::BATTERYMONITOR) << "Remaining time 2:" << time;
    m_remainingMsec = time;
}

void BatteryControl::smoothedBatteryRemainingTimeChanged(qulonglong time)
{
    m_smoothedRemainingMsec = time;
}

void BatteryControl::updateAcPlugState(bool onBattery)
{
    m_pluggedIn = !onBattery;
}

void BatteryControl::updateOverallBattery()
{
    const QList<Solid::Device> listBattery = Solid::Device::listFromType(Solid::DeviceInterface::Battery);
    bool hasCumulative = false;

    double energy = 0;
    double totalEnergy = 0;
    bool allFullyCharged = true;
    bool charging = false;
    bool noCharge = false;
    double totalPercentage = 0;
    int count = 0;

    for (const Solid::Device &deviceBattery : listBattery) {
        const Solid::Battery *battery = deviceBattery.as<Solid::Battery>();

        if (battery && battery->isPowerSupply()) {
            hasCumulative = true;

            energy += battery->energy();
            totalEnergy += battery->energyFull();
            totalPercentage += battery->chargePercent();
            allFullyCharged = allFullyCharged && (battery->chargeState() == Solid::Battery::FullyCharged);
            charging = charging || (battery->chargeState() == Solid::Battery::Charging);
            noCharge = noCharge || (battery->chargeState() == Solid::Battery::NoCharge);
            ++count;
        }
    }

    if (count == 1) {
        // Energy is sometimes way off causing us to show rubbish; this is a UPower issue
        // but anyway having just one battery and the tooltip showing strange readings
        // compared to the popup doesn't look polished.
        m_percent = qRound(totalPercentage);
    } else if (totalEnergy > 0) {
        m_percent = qRound(energy / totalEnergy * 100);
    } else if (count > 0) { // UPS don't have energy, see Bug 348588
        m_percent = qRound(totalPercentage / static_cast<qreal>(count));
    } else {
        m_percent = 0;
    }

    if (hasCumulative) {
        if (allFullyCharged) {
            m_state = QStringLiteral("FullyCharged");
        } else if (charging) {
            m_state = QStringLiteral("Charging");
        } else if (noCharge) {
            m_state = QStringLiteral("NoCharge");
        } else {
            m_state = QStringLiteral("Discharging");
        }
    } else {
        m_state = QStringLiteral("Unknown");
    }

    m_hasCumulative = hasCumulative;

    qCDebug(APPLETS::BATTERYMONITOR) << "____ Overal battery updated ____ \n"
                                     << "Has cumulative          : " << (hasCumulative ? "Yes" : "No") << "\n"
                                     << "Has battery             : " << (m_hasBattery ? "Yes" : "No") << "\n"
                                     << "Plugged In              : " << (m_pluggedIn ? "Yes" : "No") << "\n"
                                     << "State                   : " << m_state << "\n"
                                     << "Charge Stop Threshold   : " << m_chargeStopThreshold << "\n"
                                     << "Remaining Msec          : " << m_remainingMsec << "\n"
                                     << "Smoothed Remaining Msec : " << m_smoothedRemainingMsec << "\n"
                                     << "Percent                 : " << m_percent << "\n";
}

QList<QObject *> BatteryControl::batteries()
{
    return m_batterySources;
}

QBindable<bool> BatteryControl::bindableHasCumulative()
{
    return &m_hasCumulative;
}

QBindable<bool> BatteryControl::bindableHasBattery()
{
    return &m_hasBattery;
}

QBindable<bool> BatteryControl::bindablePluggedIn()
{
    return &m_pluggedIn;
}

QBindable<QString> BatteryControl::bindableState()
{
    return &m_state;
}

QBindable<int> BatteryControl::bindableChargeStopThresholdChanged()
{
    return &m_chargeStopThreshold;
}

QBindable<qulonglong> BatteryControl::bindableRemainingMsec()
{
    return &m_remainingMsec;
}

QBindable<qulonglong> BatteryControl::bindableSmoothedRemainingMsec()
{
    return &m_smoothedRemainingMsec;
}

QBindable<int> BatteryControl::bindablePercent()
{
    return &m_percent;
}

#include "moc_batterycontrol.cpp"
