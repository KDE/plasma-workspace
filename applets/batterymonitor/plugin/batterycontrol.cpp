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

#include "batteriesnamesmonitor_p.h"

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";

BatteryControlModel::BatteryControlModel(QObject *parent)
    : QAbstractListModel(parent)
    , namesMonitor(new BatteriesNamesMonitor)
{
    m_internalBatteries.reserve(2);

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        const QList<Solid::Device> listBattery = Solid::Device::listFromType(Solid::DeviceInterface::Battery);

        if (!listBattery.isEmpty()) {
            for (const Solid::Device &deviceBattery : listBattery) {
                deviceAdded(deviceBattery.udi());
            }

            m_hasBatteries = true;
            updateOverallBattery();

            QDBusMessage batteryRemainingTimeMessage = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                                      QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                                                      SOLID_POWERMANAGEMENT_SERVICE,
                                                                                      QStringLiteral("batteryRemainingTime"));
            QDBusPendingCall batteryRemainingTimeCall = QDBusConnection::sessionBus().asyncCall(batteryRemainingTimeMessage);
            auto batteryRemainingTimeWatcher = new QDBusPendingCallWatcher(batteryRemainingTimeCall, this);
            connect(batteryRemainingTimeWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
                QDBusReply<qulonglong> reply = *watcher;
                if (!reply.isValid()) {
                    m_remainingMsec = reply.value();
                } else {
                    qCDebug(APPLETS::BATTERYMONITOR) << "error getting battery remaining time";
                }
                watcher->deleteLater();
            });

            QDBusMessage smoothedBatteryRemainingTimeMessage = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                                              QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                                                              SOLID_POWERMANAGEMENT_SERVICE,
                                                                                              QStringLiteral("smoothedBatteryRemainingTime"));
            QDBusPendingCall smoothedBatteryRemainingTimeCall = QDBusConnection::sessionBus().asyncCall(smoothedBatteryRemainingTimeMessage);
            auto smoothedBatteryRemainingTimeWatcher = new QDBusPendingCallWatcher(smoothedBatteryRemainingTimeCall, this);
            connect(smoothedBatteryRemainingTimeWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
                QDBusReply<qulonglong> reply = *watcher;
                if (reply.isValid()) {
                    m_smoothedRemainingMsec = reply.value();
                } else {
                    qCDebug(APPLETS::BATTERYMONITOR) << "error getting smoothed battery remaining time";
                }

                watcher->deleteLater();
            });
        } else {
            m_hasBatteries = false;
            m_hasCumulative = false;
        }

        connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &BatteryControlModel::deviceAdded);
        connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &BatteryControlModel::deviceRemoved);

        QDBusMessage chargeStopThresholdMessage = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                                 QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                                                 SOLID_POWERMANAGEMENT_SERVICE,
                                                                                 QStringLiteral("chargeStopThreshold"));
        QDBusPendingCall chargeStopThresholdCall = QDBusConnection::sessionBus().asyncCall(chargeStopThresholdMessage);
        auto watcher = new QDBusPendingCallWatcher(chargeStopThresholdCall, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<int> reply = *watcher;
            if (reply.isValid()) {
                m_chargeStopThreshold = reply.value();
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "error getting charge stop threshold";
            }
            watcher->deleteLater();
        });

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
                                                   SLOT(updateBatteryChargeStopThreshold(int)))) {
            qCDebug(APPLETS::BATTERYMONITOR) << "error connecting to charge stop threshold changes via dbus";
        }

        QDBusMessage PowerSaveStatusMessage = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement"),
                                                                             QStringLiteral("/org/freedesktop/PowerManagement"),
                                                                             QStringLiteral("org.freedesktop.PowerManagement"),
                                                                             QStringLiteral("GetPowerSaveStatus"));
        QDBusPendingCall PowerSaveStatusCall = QDBusConnection::sessionBus().asyncCall(PowerSaveStatusMessage);
        auto powerSaveStatusWatcher = new QDBusPendingCallWatcher(PowerSaveStatusCall, this);
        connect(powerSaveStatusWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<bool> reply = *watcher;
            if (reply.isValid()) {
                updateAcPlugState(reply.value());
            } else {
                qCDebug(APPLETS::BATTERYMONITOR) << "Fail to retrive power save status";
            }
            watcher->deleteLater();
        });

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

BatteryControlModel::~BatteryControlModel()
{
}

int BatteryControlModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_batterySources.size();
}

QVariant BatteryControlModel::data(const QModelIndex &index, int role) const
{
    Solid::Device deviceBattery(m_batterySources[index.row()]);
    if (!deviceBattery.isValid()) {
        return {};
    }

    auto battery = deviceBattery.as<Solid::Battery>();
    if (!battery) {
        return {};
    }

    switch (role) {
    case Percent:
        return QVariant::fromValue(battery->chargePercent());
    case Capacity:
        return QVariant::fromValue(battery->capacity());
    case Energy:
        return QVariant::fromValue(battery->energy());
    case PluggedIn:
        return QVariant::fromValue(battery->isPresent());
    case IsPowerSupply:
        return QVariant::fromValue(battery->isPowerSupply());
    case ChargeState:
        return QVariant::fromValue(updateBatteryState(battery));
    case PrettyName:
        return QVariant::fromValue(namesMonitor->updateBatteryName(deviceBattery, battery));
    case Type:
        return QVariant::fromValue(batteryTypeToString(battery));
    }
    return {};
}

QHash<int, QByteArray> BatteryControlModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Percent] = "Percent";
    roles[Capacity] = "Capacity";
    roles[Energy] = "Energy";
    roles[PluggedIn] = "PluggedIn";
    roles[IsPowerSupply] = "IsPowerSupply";
    roles[ChargeState] = "ChargeState";
    roles[PrettyName] = "PrettyName";
    roles[Type] = "Type";
    return roles;
}

void BatteryControlModel::deviceAdded(const QString &udi)
{
    qCDebug(APPLETS::BATTERYMONITOR) << "Device add signal arrived. Udi: " << udi;

    Solid::Device deviceBattery(udi);
    if (!deviceBattery.isValid()) {
        return;
    }
    Solid::Battery *battery = deviceBattery.as<Solid::Battery>();

    if (!battery) {
        return;
    }

    if (battery->type() == Solid::Battery::PrimaryBattery) {
        m_internalBatteries.append(udi);

        connect(battery, &Solid::Battery::presentStateChanged, this, &BatteryControlModel::updateOverallBattery);
        connect(battery, &Solid::Battery::energyChanged, this, &BatteryControlModel::updateOverallBattery);
        connect(battery, &Solid::Battery::energyFullChanged, this, &BatteryControlModel::updateOverallBattery);
        connect(battery, &Solid::Battery::chargePercentChanged, this, &BatteryControlModel::updateOverallBattery);
        connect(battery, &Solid::Battery::chargeStateChanged, this, &BatteryControlModel::updateOverallBattery);
    }

    connect(battery, &Solid::Battery::chargeStateChanged, this, &BatteryControlModel::updateBatteryChargeState);
    connect(battery, &Solid::Battery::chargePercentChanged, this, &BatteryControlModel::updateBatteryChargePercent);
    connect(battery, &Solid::Battery::energyChanged, this, &BatteryControlModel::updateBatteryEnergy);
    connect(battery, &Solid::Battery::presentStateChanged, this, &BatteryControlModel::updatePluggedInState);
    connect(battery, &Solid::Battery::powerSupplyStateChanged, this, &BatteryControlModel::updateBatteryPowerSupplyState);
    connect(battery, &Solid::Battery::capacityChanged, this, &BatteryControlModel::updateBatteryCapacity);

    int position = m_batterySources.size();

    qCDebug(APPLETS::BATTERYMONITOR) << "Position for battery with udi : " << udi << "intitialized : " << position;

    m_batteryPositions[udi] = position;

    qCDebug(APPLETS::BATTERYMONITOR) << "Update Battery Position. Udi: " << udi << "Position: " << m_batteryPositions[udi];

    beginInsertRows(QModelIndex(), position, position);
    m_batterySources.append(udi);
    endInsertRows();

    qCDebug(APPLETS::BATTERYMONITOR) << "Battery with udi: " << udi << " is added";

    m_hasBatteries = true;

    updateOverallBattery();
}

void BatteryControlModel::deviceRemoved(const QString &udi)
{
    qCDebug(APPLETS::BATTERYMONITOR) << "Device remove signal arrived. Udi: " << udi;
    if (m_batterySources.isEmpty()) {
        return;
    }

    auto position = m_batteryPositions.constFind(udi);
    if (position == m_batteryPositions.constEnd()) {
        return;
    }

    namesMonitor->removeBatteryName(udi);

    m_internalBatteries.removeOne(udi);

    if (auto deleteBattery = Solid::Device(udi).as<Solid::Battery>()) {
        deleteBattery->disconnect(this);
    }

    qCDebug(APPLETS::BATTERYMONITOR) << "battery with udi: " << udi << "at index: " << *position << "is removed";

    for (int newPosition = *position + 1; newPosition < m_batterySources.size(); ++newPosition) {
        m_batteryPositions[m_batterySources[newPosition]] = newPosition - 1;
    }

    beginRemoveRows(QModelIndex(), *position, *position);
    m_batterySources.removeAt(*position);
    endRemoveRows();

    m_batteryPositions.erase(position);

    m_hasBatteries = !m_batterySources.isEmpty();

    updateOverallBattery();
}

void BatteryControlModel::updateBatteryCapacity(int newState, const QString &udi)
{
    Q_UNUSED(newState);
    QModelIndex index = BatteryControlModel::index(m_batteryPositions[udi]);
    Q_EMIT dataChanged(index, index, {Capacity});
}

void BatteryControlModel::updateBatteryChargeState(int newState, const QString &udi)
{
    Q_UNUSED(newState);
    QModelIndex index = BatteryControlModel::index(m_batteryPositions[udi]);
    Q_EMIT dataChanged(index, index, {ChargeState});
}

void BatteryControlModel::updateBatteryChargePercent(int newValue, const QString &udi)
{
    Q_UNUSED(newValue);
    QModelIndex index = BatteryControlModel::index(m_batteryPositions[udi]);
    Q_EMIT dataChanged(index, index, {Percent});
}

void BatteryControlModel::updateBatteryEnergy(double newValue, const QString &udi)
{
    Q_UNUSED(newValue);
    QModelIndex index = BatteryControlModel::index(m_batteryPositions[udi]);
    Q_EMIT dataChanged(index, index, {Energy});
}

void BatteryControlModel::updateBatteryPowerSupplyState(bool newState, const QString &udi)
{
    Q_UNUSED(newState);
    QModelIndex index = BatteryControlModel::index(m_batteryPositions[udi]);
    Q_EMIT dataChanged(index, index, {IsPowerSupply});
}

void BatteryControlModel::updatePluggedInState(bool onBattery, const QString &udi)
{
    Q_UNUSED(onBattery);
    QModelIndex index = BatteryControlModel::index(m_batteryPositions[udi]);
    Q_EMIT dataChanged(index, index, {PluggedIn});
}

void BatteryControlModel::batteryRemainingTimeChanged(qulonglong time)
{
    // qCDebug(APPLETS::BATTERYMONITOR) << "Remaining time 2:" << time;
    m_remainingMsec = time;
}

void BatteryControlModel::smoothedBatteryRemainingTimeChanged(qulonglong time)
{
    m_smoothedRemainingMsec = time;
}

void BatteryControlModel::updateBatteryChargeStopThreshold(int threshold)
{
    m_chargeStopThreshold = threshold;
}

void BatteryControlModel::updateAcPlugState(bool onBattery)
{
    m_pluggedIn = !onBattery;
}

void BatteryControlModel::updateOverallBattery()
{
    if (m_internalBatteries.isEmpty()) {
        m_hasInternalBatteries = false;
        return;
    }

    bool hasInternalBatteries = false;

    bool hasCumulative = false;

    double energy = 0;
    double totalEnergy = 0;
    bool allFullyCharged = true;
    bool charging = false;
    bool discharging = false;
    bool noCharge = false;
    double totalPercentage = 0;
    int count = 0;

    for (const QString &internalBattery : std::as_const(m_internalBatteries)) {
        Solid::Device deviceBattery(internalBattery);
        const Solid::Battery *battery = deviceBattery.as<Solid::Battery>();
        if (battery && battery->isPresent()) {
            hasInternalBatteries = true;
            if (battery->isPowerSupply()) {
                hasCumulative = true;

                energy += battery->energy();
                totalEnergy += battery->energyFull();
                totalPercentage += battery->chargePercent();
                allFullyCharged = allFullyCharged && (battery->chargeState() == Solid::Battery::FullyCharged);
                charging = charging || (battery->chargeState() == Solid::Battery::Charging);
                discharging = discharging || (battery->chargeState() == Solid::Battery::Discharging);
                noCharge = noCharge || (battery->chargeState() == Solid::Battery::NoCharge);
                ++count;
            }
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
            m_state = Solid::Battery::FullyCharged;
        } else if (charging) {
            m_state = Solid::Battery::Charging;
        } else if (discharging) {
            m_state = Solid::Battery::Discharging;
        } else if (noCharge) {
            // When we are using a charge threshold, the kernel
            // may stop charging within a percentage point of the actual threshold
            // and this is considered correct behavior, so we have to handle
            // that. See https://bugzilla.kernel.org/show_bug.cgi?id=215531.
            // Also, Upower may give us a status of "Not charging" rather than
            // "Fullycharged", so we need to account for that as well. See
            // https://gitlab.freedesktop.org/upower/upower/-/issues/142.
            if (m_pluggedIn && (m_percent >= m_chargeStopThreshold - 1 && m_percent <= m_chargeStopThreshold + 1)) {
                m_state = Solid::Battery::FullyCharged;
            } else {
                m_state = Solid::Battery::NoCharge;
            }
        }
    } else {
        m_state = Solid::Battery::NoCharge;
    }

    m_hasCumulative = hasCumulative;
    m_hasInternalBatteries = hasInternalBatteries;
    qCDebug(APPLETS::BATTERYMONITOR) << "Is have internal batteries: " << hasInternalBatteries;

    qCDebug(APPLETS::BATTERYMONITOR) << "____ Overal battery updated ____ \n"
                                     << "Has cumulative          : " << (hasCumulative ? "Yes" : "No") << "\n"
                                     << "Has battery             : " << (m_hasBatteries ? "Yes" : "No") << "\n"
                                     << "Plugged In              : " << (m_pluggedIn ? "Yes" : "No") << "\n"
                                     << "State                   : " << m_state << "\n"
                                     << "Charge Stop Threshold   : " << m_chargeStopThreshold << "\n"
                                     << "Remaining Msec          : " << m_remainingMsec << "\n"
                                     << "Smoothed Remaining Msec : " << m_smoothedRemainingMsec << "\n"
                                     << "Percent                 : " << m_percent << "\n";
}

QBindable<bool> BatteryControlModel::bindableHasCumulative()
{
    return &m_hasCumulative;
}

QBindable<bool> BatteryControlModel::bindableHasBatteries()
{
    return &m_hasBatteries;
}

QBindable<bool> BatteryControlModel::bindablePluggedIn()
{
    return &m_pluggedIn;
}

QBindable<bool> BatteryControlModel::bindableHasInternalBatteries()
{
    return &m_hasInternalBatteries;
}

QBindable<Solid::Battery::ChargeState> BatteryControlModel::bindableState()
{
    return &m_state;
}

QBindable<int> BatteryControlModel::bindableChargeStopThresholdChanged()
{
    return &m_chargeStopThreshold;
}

QBindable<qulonglong> BatteryControlModel::bindableRemainingMsec()
{
    return &m_remainingMsec;
}

QBindable<qulonglong> BatteryControlModel::bindableSmoothedRemainingMsec()
{
    return &m_smoothedRemainingMsec;
}

QBindable<int> BatteryControlModel::bindablePercent()
{
    return &m_percent;
}

Solid::Battery::ChargeState BatteryControlModel::updateBatteryState(const Solid::Battery *battery) const
{
    // When we are using a charge threshold, the kernel
    // may stop charging within a percentage point of the actual threshold
    // and this is considered correct behavior, so we have to handle
    // that. See https://bugzilla.kernel.org/show_bug.cgi?id=215531.
    if ((battery->chargePercent() >= m_chargeStopThreshold - 1 && battery->chargePercent() <= m_chargeStopThreshold + 1)
        // Also, Upower may give us a status of "Not charging" rather than
        // "Fully charged", so we need to account for that as well. See
        // https://gitlab.freedesktop.org/upower/upower/-/issues/142.
        && (battery->chargeState() == Solid::Battery::NoCharge || battery->chargeState() == Solid::Battery::FullyCharged)) {
        return Solid::Battery::FullyCharged;
    }

    // Otherwise, just look at the charge state
    return battery->chargeState();
}

QString BatteryControlModel::batteryTypeToString(const Solid::Battery *battery) const
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

#include "moc_batterycontrol.cpp"
