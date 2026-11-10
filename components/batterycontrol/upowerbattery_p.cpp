/*
 * SPDX-FileCopyrightText: 2026 Ramil Nurmanov <ramil2004nur@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "upowerbattery_p.h"

#include "batterycontrol_debug.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QTimer>

#include <klocalizedstring.h>

#include <utility>

using namespace Qt::StringLiterals;

namespace
{
constexpr QLatin1StringView UPOWER_SERVICE("org.freedesktop.UPower");
constexpr QLatin1StringView UPOWER_DEVICE_IFACE("org.freedesktop.UPower.Device");
constexpr QLatin1StringView PROPERTIES_IFACE("org.freedesktop.DBus.Properties");

constexpr int RETRY_BASE_DELAY_MSEC = 1000;
constexpr int MAX_RETRIES = 5;

bool isBattery(UpDeviceKind kind)
{
    switch (kind) {
    case UP_DEVICE_KIND_BATTERY:
    case UP_DEVICE_KIND_UPS:
    case UP_DEVICE_KIND_MOUSE:
    case UP_DEVICE_KIND_KEYBOARD:
    case UP_DEVICE_KIND_PDA:
    case UP_DEVICE_KIND_PHONE:
    case UP_DEVICE_KIND_TABLET:
    case UP_DEVICE_KIND_GAMING_INPUT:
    case UP_DEVICE_KIND_SPEAKERS:
    case UP_DEVICE_KIND_HEADSET:
    case UP_DEVICE_KIND_HEADPHONES:
    case UP_DEVICE_KIND_BLUETOOTH_GENERIC:
    case UP_DEVICE_KIND_TOUCHPAD:
    case UP_DEVICE_KIND_WEARABLE:
        return true;
    default:
        return false;
    }
}

QString technologyName(UpDeviceTechnology technology)
{
    switch (technology) {
    case UP_DEVICE_TECHNOLOGY_LITHIUM_ION:
        return i18nc("battery technology", "Lithium Ion");
    case UP_DEVICE_TECHNOLOGY_LITHIUM_POLYMER:
        return i18nc("battery technology", "Lithium Polymer");
    case UP_DEVICE_TECHNOLOGY_LITHIUM_IRON_PHOSPHATE:
        return i18nc("battery technology", "Lithium Iron Phosphate");
    case UP_DEVICE_TECHNOLOGY_LEAD_ACID:
        return i18nc("battery technology", "Lead Acid");
    case UP_DEVICE_TECHNOLOGY_NICKEL_CADMIUM:
        return i18nc("battery technology", "Nickel Cadmium");
    case UP_DEVICE_TECHNOLOGY_NICKEL_METAL_HYDRIDE:
        return i18nc("battery technology", "Nickel Metal Hydride");
    default:
        return {};
    }
}
}

UPowerBattery::UPowerBattery(const QString &udi, QObject *parent)
    : QObject(parent)
    , m_udi(udi)
{
    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout, this, &UPowerBattery::read);

    QDBusConnection::systemBus()
        .connect(UPOWER_SERVICE, m_udi, PROPERTIES_IFACE, u"PropertiesChanged"_s, this, SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
}

void UPowerBattery::load()
{
    m_retryCount = 0;
    read();
}

void UPowerBattery::read()
{
    // A newer read supersedes the one in flight; deleting its watcher drops the outdated reply.
    delete m_readWatcher;
    m_retryTimer.stop();
    m_changesDuringRead.clear();

    QDBusMessage message = QDBusMessage::createMethodCall(UPOWER_SERVICE, m_udi, PROPERTIES_IFACE, u"GetAll"_s);
    message.setArguments({QString(UPOWER_DEVICE_IFACE)});

    m_readWatcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message), this);
    connect(m_readWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        m_readWatcher = nullptr;
        watcher->deleteLater();

        const QVariantMap changesDuringRead = std::exchange(m_changesDuringRead, {});

        const QDBusPendingReply<QVariantMap> reply = *watcher;
        if (reply.isError()) {
            qCWarning(COMPONENTS::BATTERYCONTROL) << "error reading properties of" << m_udi << ":" << reply.error();
            if (!m_loaded) {
                scheduleRetry();
            }
            return;
        }

        Properties loaded;
        loaded.update(reply.value());
        loaded.update(changesDuringRead);

        if (!m_loaded) {
            m_properties = loaded;
            m_loaded = isBattery(m_properties.kind);
            Q_EMIT loadFinished(m_udi, m_loaded);
            return;
        }

        const Properties previous = std::exchange(m_properties, loaded);
        emitChanges(previous);
    });
}

void UPowerBattery::scheduleRetry()
{
    if (m_retryCount >= MAX_RETRIES) {
        qCWarning(COMPONENTS::BATTERYCONTROL) << "giving up reading" << m_udi << "until it changes or the system resumes";
        return;
    }

    m_retryTimer.start(RETRY_BASE_DELAY_MSEC << m_retryCount++);
}

QString UPowerBattery::udi() const
{
    return m_udi;
}

QString UPowerBattery::vendor() const
{
    return m_properties.vendor;
}

QString UPowerBattery::product() const
{
    if (!m_properties.model.isEmpty()) {
        return m_properties.model;
    }

    const QString technology = technologyName(m_properties.technology);
    if (!technology.isEmpty()) {
        return i18nc("%1 is battery technology", "%1 Battery", technology);
    }
    return vendor();
}

UPowerBattery::Type UPowerBattery::type() const
{
    switch (m_properties.kind) {
    case UP_DEVICE_KIND_BATTERY:
        return Type::Primary;
    case UP_DEVICE_KIND_UPS:
        return Type::Ups;
    case UP_DEVICE_KIND_MOUSE:
        return Type::Mouse;
    case UP_DEVICE_KIND_KEYBOARD:
        return Type::Keyboard;
    case UP_DEVICE_KIND_PDA:
        return Type::Pda;
    case UP_DEVICE_KIND_PHONE:
        return Type::Phone;
    case UP_DEVICE_KIND_TABLET:
        return Type::Tablet;
    case UP_DEVICE_KIND_GAMING_INPUT:
        return Type::GamingInput;
    case UP_DEVICE_KIND_HEADPHONES:
        return Type::Headphone;
    case UP_DEVICE_KIND_HEADSET:
        return Type::Headset;
    case UP_DEVICE_KIND_TOUCHPAD:
        return Type::Touchpad;
    case UP_DEVICE_KIND_BLUETOOTH_GENERIC:
        return Type::Bluetooth;
    case UP_DEVICE_KIND_SPEAKERS:
        return Type::Speakers;
    case UP_DEVICE_KIND_WEARABLE:
        return Type::Wearable;
    default:
        return Type::Unknown;
    }
}

bool UPowerBattery::isPresent() const
{
    return m_properties.isPresent;
}

bool UPowerBattery::isPowerSupply() const
{
    return m_properties.isPowerSupply;
}

int UPowerBattery::chargePercent() const
{
    return m_properties.chargePercent();
}

int UPowerBattery::capacity() const
{
    return m_properties.capacityPercent();
}

double UPowerBattery::energy() const
{
    return m_properties.energy;
}

double UPowerBattery::energyFull() const
{
    return m_properties.energyFull;
}

UPowerBattery::ChargeState UPowerBattery::chargeState() const
{
    return m_properties.chargeState();
}

void UPowerBattery::Properties::update(const QVariantMap &changedProperties)
{
    for (auto it = changedProperties.cbegin(); it != changedProperties.cend(); ++it) {
        const QString &name = it.key();
        const QVariant &value = it.value();

        if (name == "Type"_L1) {
            kind = static_cast<UpDeviceKind>(value.toUInt());
        } else if (name == "Vendor"_L1) {
            vendor = value.toString();
        } else if (name == "Model"_L1) {
            model = value.toString();
        } else if (name == "Technology"_L1) {
            technology = static_cast<UpDeviceTechnology>(value.toUInt());
        } else if (name == "IsPresent"_L1) {
            isPresent = value.toBool();
        } else if (name == "PowerSupply"_L1) {
            isPowerSupply = value.toBool();
        } else if (name == "Percentage"_L1) {
            percentage = value.toDouble();
        } else if (name == "BatteryLevel"_L1) {
            batteryLevel = static_cast<UpDeviceLevel>(value.toUInt());
        } else if (name == "Capacity"_L1) {
            capacity = value.toDouble();
        } else if (name == "State"_L1) {
            state = static_cast<UpDeviceState>(value.toUInt());
        } else if (name == "Energy"_L1) {
            energy = value.toDouble();
        } else if (name == "EnergyFull"_L1) {
            energyFull = value.toDouble();
        }
    }
}

int UPowerBattery::Properties::chargePercent() const
{
    if (batteryLevel == UP_DEVICE_LEVEL_UNKNOWN) {
        return -1;
    }
    return qRound(percentage);
}

int UPowerBattery::Properties::capacityPercent() const
{
    return qRound(capacity);
}

UPowerBattery::ChargeState UPowerBattery::Properties::chargeState() const
{
    switch (state) {
    case UP_DEVICE_STATE_CHARGING:
        return ChargeState::Charging;
    case UP_DEVICE_STATE_DISCHARGING:
        return ChargeState::Discharging;
    case UP_DEVICE_STATE_FULLY_CHARGED:
        return ChargeState::FullyCharged;
    default:
        return ChargeState::NoCharge;
    }
}

void UPowerBattery::onPropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (interfaceName != UPOWER_DEVICE_IFACE) {
        return;
    }

    if (m_readWatcher) {
        m_changesDuringRead.insert(changedProperties);
    }

    if (m_loaded) {
        const Properties previous = m_properties;
        m_properties.update(changedProperties);
        emitChanges(previous);
    }

    if (!invalidatedProperties.isEmpty() || (!m_loaded && !m_readWatcher)) {
        load();
    }
}

void UPowerBattery::emitChanges(const Properties &previous)
{
    const Properties &current = m_properties;
    Changes changes;

    changes.setFlag(Change::Present, previous.isPresent != current.isPresent);
    changes.setFlag(Change::ChargePercent, previous.chargePercent() != current.chargePercent());
    changes.setFlag(Change::Capacity, previous.capacityPercent() != current.capacityPercent());
    changes.setFlag(Change::PowerSupply, previous.isPowerSupply != current.isPowerSupply);
    changes.setFlag(Change::ChargeState, previous.chargeState() != current.chargeState());
    changes.setFlag(Change::Energy, previous.energy != current.energy);
    changes.setFlag(Change::EnergyFull, previous.energyFull != current.energyFull);

    if (changes) {
        Q_EMIT changed(changes, m_udi);
    }
}

#include "moc_upowerbattery_p.cpp"
