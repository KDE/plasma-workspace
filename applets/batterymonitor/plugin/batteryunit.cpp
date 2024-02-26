/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "batteryunit.h"

#include "batterymonitor_debug.h"

#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>

#include <klocalizedstring.h>

uint BatteryUnit::m_unnamedBatteries = 0;

BatteryUnit::BatteryUnit(const Solid::Device &deviceBattery, QObject *parent)
    : QObject(parent)
{
    const auto *battery = deviceBattery.as<Solid::Battery>();

    m_udi = deviceBattery.udi();
    m_state = batteryStateToString(battery->chargeState());
    m_vendor = deviceBattery.vendor();
    m_product = deviceBattery.product();
    m_type = batteryTypeToString(battery);

    m_percent = battery->chargePercent();
    m_capacity = battery->capacity();
    m_energy = battery->energy();
    m_pluggedIn = battery->isPresent();
    m_isPowerSupply = battery->isPowerSupply();

    connect(battery, &Solid::Battery::chargeStateChanged, this, &BatteryUnit::updateBatteryChargeState);
    connect(battery, &Solid::Battery::chargePercentChanged, this, &BatteryUnit::updateBatteryChargePercent);
    connect(battery, &Solid::Battery::energyChanged, this, &BatteryUnit::updateBatteryEnergy);
    connect(battery, &Solid::Battery::presentStateChanged, this, &BatteryUnit::updatePluggedInState);
    connect(battery, &Solid::Battery::powerSupplyStateChanged, this, &BatteryUnit::updateBatteryPowerSupplyState);
    connect(battery, &Solid::Battery::capacityChanged, this, &BatteryUnit::updateBatteryCapacity);

    updateBatteryName();
}

BatteryUnit::~BatteryUnit()
{
    if (isUnnamedBattery) {
        --m_unnamedBatteries;
    }

    Solid::Device device(m_udi);
    Solid::Battery *battery = device.as<Solid::Battery>();
    if (battery) {
        battery->disconnect(this);
    }
}

void BatteryUnit::updateBatteryName()
{
    // Don't show battery name for primary power supply batteries. They usually have cryptic serial number names.
    bool showBatteryName = m_type != QLatin1String("Battery") || !m_isPowerSupply;
    if (!m_product.isEmpty() && m_product != QLatin1String("Unknown Battery") && showBatteryName) {
        if (!m_vendor.isEmpty()) {
            m_prettyName = (QString(m_vendor + ' ' + m_product));
        } else {
            m_prettyName = m_product;
        }
    } else {
        ++m_unnamedBatteries;
        isUnnamedBattery = true;
        if (m_unnamedBatteries > 1) {
            m_prettyName = i18nc("Placeholder is the battery number", "Battery %1", m_unnamedBatteries);
        } else {
            m_prettyName = i18n("Battery");
        }
    }
}

QString BatteryUnit::batteryStateToString(int newState) const
{
    QString state(QStringLiteral("Unknown"));
    if (newState == Solid::Battery::NoCharge) {
        state = QStringLiteral("NoCharge");
    } else if (newState == Solid::Battery::Charging) {
        state = QStringLiteral("Charging");
    } else if (newState == Solid::Battery::Discharging) {
        state = QStringLiteral("Discharging");
    } else if (newState == Solid::Battery::FullyCharged) {
        state = QStringLiteral("FullyCharged");
    }

    return state;
}

QString BatteryUnit::batteryTypeToString(const Solid::Battery *battery) const
{
    switch (battery->type()) {
    case Solid::Battery::PrimaryBattery:
        return QStringLiteral("Battery");
    case Solid::Battery::UpsBattery:
        return QStringLiteral("Ups");
    case Solid::Battery::MonitorBattery:
        return QStringLiteral("Monitor");
    case Solid::Battery::MouseBattery:
        return QStringLiteral("Mouse");
    case Solid::Battery::KeyboardBattery:
        return QStringLiteral("Keyboard");
    case Solid::Battery::PdaBattery:
        return QStringLiteral("Pda");
    case Solid::Battery::PhoneBattery:
        return QStringLiteral("Phone");
    case Solid::Battery::GamingInputBattery:
        return QStringLiteral("GamingInput");
    case Solid::Battery::BluetoothBattery:
        return QStringLiteral("Bluetooth");
    case Solid::Battery::TabletBattery:
        return QStringLiteral("Tablet");
    case Solid::Battery::HeadphoneBattery:
        return QStringLiteral("Headphone");
    case Solid::Battery::HeadsetBattery:
        return QStringLiteral("Headset");
    case Solid::Battery::TouchpadBattery:
        return QStringLiteral("Touchpad");
    default:
        return QStringLiteral("Unknown");
    }
}

QString BatteryUnit::state()
{
    return m_state;
}

QString BatteryUnit::vendor()
{
    return m_vendor;
}

QString BatteryUnit::product()
{
    return m_product;
}

QString BatteryUnit::type()
{
    return m_type;
}

QString BatteryUnit::prettyName()
{
    return m_prettyName;
}

QBindable<int> BatteryUnit::bindablePercent()
{
    return &m_percent;
}

QBindable<int> BatteryUnit::bindableCapacity()
{
    return &m_capacity;
}

QBindable<double> BatteryUnit::bindableEnergy()
{
    return &m_energy;
}

QBindable<bool> BatteryUnit::bindableIsPowerSupply()
{
    return &m_isPowerSupply;
}

QBindable<bool> BatteryUnit::bindablePluggedIn()
{
    return &m_pluggedIn;
}

QBindable<QString> BatteryUnit::bindableState()
{
    return &m_state;
}

void BatteryUnit::updateBatteryChargeState(int newState, const QString &udi)
{
    Q_UNUSED(udi);

    m_state = batteryStateToString(newState);

    Q_EMIT batteryStatusChanged();

    qCDebug(APPLETS::BATTERYMONITOR) << "battery " << m_prettyName << " : "
                                     << "battery charge state updated: " << newState;
}

void BatteryUnit::updateBatteryChargePercent(int newValue, const QString &udi)
{
    Q_UNUSED(udi);

    m_percent = newValue;

    Q_EMIT batteryStatusChanged();

    qCDebug(APPLETS::BATTERYMONITOR) << "battery " << m_prettyName << " : "
                                     << "battery charge percent updated: " << newValue;
}

void BatteryUnit::updateBatteryEnergy(double newValue, const QString &udi)
{
    Q_UNUSED(udi);

    m_energy = newValue;

    Q_EMIT batteryStatusChanged();

    qCDebug(APPLETS::BATTERYMONITOR) << "battery " << m_prettyName << " : "
                                     << "battery energy updated: " << newValue;
}

void BatteryUnit::updateBatteryPowerSupplyState(bool onBattery, const QString &udi)
{
    Q_UNUSED(udi);

    m_isPowerSupply = !onBattery;

    qCDebug(APPLETS::BATTERYMONITOR) << "battery " << m_prettyName << " : "
                                     << "battery power supply state updated: " << !onBattery;
}

void BatteryUnit::updatePluggedInState(bool newState, const QString &udi)
{
    Q_UNUSED(udi);

    m_pluggedIn = newState;

    Q_EMIT batteryStatusChanged();

    qCDebug(APPLETS::BATTERYMONITOR) << "battery " << m_prettyName << " : "
                                     << "battery AC plug state updated: " << newState;
}

void BatteryUnit::updateBatteryCapacity(int newValue, const QString &udi)
{
    Q_UNUSED(udi);

    m_capacity = newValue;

    Q_EMIT batteryStatusChanged();

    qCDebug(APPLETS::BATTERYMONITOR) << "battery " << m_prettyName << " : "
                                     << "battery capacity updated: " << newValue;
}

QString BatteryUnit::udi() const
{
    return m_udi;
}

#include "moc_batteryunit.cpp"
