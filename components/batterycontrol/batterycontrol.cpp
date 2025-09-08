/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "batterycontrol.h"

#include "batterycontrol_debug.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDBusServiceWatcher>

#include <klocalizedstring.h>

#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>

#include "batteriesnamesmonitor_p.h"

static const QString SOLID_POWERMANAGEMENT_SERVICE = QStringLiteral("org.kde.Solid.PowerManagement");
static const QString SOLID_POWERMANAGEMENT_PATH = QStringLiteral("/org/kde/Solid/PowerManagement");
static const QString SOLID_POWERMANAGEMENT_IFACE = QStringLiteral("org.kde.Solid.PowerManagement");

BatteryControlModel::BatteryControlModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_namesMonitor(new BatteriesNamesMonitor)
    , m_solidWatcher(new QDBusServiceWatcher)
{
    // Watch for PowerDevil's power management service
    m_solidWatcher->setConnection(QDBusConnection::sessionBus());
    m_solidWatcher->setWatchMode(QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
    m_solidWatcher->addWatchedService(SOLID_POWERMANAGEMENT_SERVICE);

    connect(m_solidWatcher.get(), &QDBusServiceWatcher::serviceRegistered, this, &BatteryControlModel::onServiceRegistered);
    connect(m_solidWatcher.get(), &QDBusServiceWatcher::serviceUnregistered, this, &BatteryControlModel::onServiceUnregistered);
    // If it's up and running already, let's cache it
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        onServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE);
    }
}

BatteryControlModel::~BatteryControlModel() = default;

void BatteryControlModel::onServiceRegistered(const QString &serviceName)
{
    m_internalBatteries.reserve(2);

    if (serviceName == SOLID_POWERMANAGEMENT_SERVICE) {
        const QList<Solid::Device> listBattery = Solid::Device::listFromType(Solid::DeviceInterface::Battery);

        if (!listBattery.isEmpty()) {
            for (const Solid::Device &deviceBattery : listBattery) {
                deviceAdded(deviceBattery.udi());
            }

            m_hasBatteries = true;
            updateOverallBattery();

            QDBusMessage batteryRemainingTimeMessage = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                                      SOLID_POWERMANAGEMENT_PATH,
                                                                                      SOLID_POWERMANAGEMENT_IFACE,
                                                                                      QStringLiteral("batteryRemainingTime"));
            QDBusPendingCall batteryRemainingTimeCall = QDBusConnection::sessionBus().asyncCall(batteryRemainingTimeMessage);
            auto batteryRemainingTimeWatcher = new QDBusPendingCallWatcher(batteryRemainingTimeCall, this);
            connect(batteryRemainingTimeWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
                QDBusReply<qulonglong> reply = *watcher;
                if (reply.isValid()) {
                    m_remainingMsec = reply.value();
                } else {
                    qCDebug(COMPONENTS::BATTERYCONTROL) << "error getting battery remaining time";
                }
                watcher->deleteLater();
            });

            QDBusMessage smoothedBatteryRemainingTimeMessage = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                                              SOLID_POWERMANAGEMENT_PATH,
                                                                                              SOLID_POWERMANAGEMENT_IFACE,
                                                                                              QStringLiteral("smoothedBatteryRemainingTime"));
            QDBusPendingCall smoothedBatteryRemainingTimeCall = QDBusConnection::sessionBus().asyncCall(smoothedBatteryRemainingTimeMessage);
            auto smoothedBatteryRemainingTimeWatcher = new QDBusPendingCallWatcher(smoothedBatteryRemainingTimeCall, this);
            connect(smoothedBatteryRemainingTimeWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
                QDBusReply<qulonglong> reply = *watcher;
                if (reply.isValid()) {
                    m_smoothedRemainingMsec = reply.value();
                } else {
                    qCDebug(COMPONENTS::BATTERYCONTROL) << "error getting smoothed battery remaining time";
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
                                                                                 SOLID_POWERMANAGEMENT_PATH,
                                                                                 SOLID_POWERMANAGEMENT_IFACE,
                                                                                 QStringLiteral("chargeStopThreshold"));
        QDBusPendingCall chargeStopThresholdCall = QDBusConnection::sessionBus().asyncCall(chargeStopThresholdMessage);
        auto watcher = new QDBusPendingCallWatcher(chargeStopThresholdCall, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusReply<int> reply = *watcher;
            if (reply.isValid()) {
                m_chargeStopThreshold = reply.value();
            } else {
                qCDebug(COMPONENTS::BATTERYCONTROL) << "error getting charge stop threshold";
            }
            watcher->deleteLater();
        });

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   SOLID_POWERMANAGEMENT_PATH,
                                                   SOLID_POWERMANAGEMENT_IFACE,
                                                   QStringLiteral("batteryRemainingTimeChanged"),
                                                   this,
                                                   SLOT(batteryRemainingTimeChanged(qulonglong)))) {
            qCDebug(COMPONENTS::BATTERYCONTROL) << "error connecting to remaining time changes";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   SOLID_POWERMANAGEMENT_PATH,
                                                   SOLID_POWERMANAGEMENT_IFACE,
                                                   QStringLiteral("smoothedBatteryRemainingTimeChanged"),
                                                   this,
                                                   SLOT(smoothedBatteryRemainingTimeChanged(qulonglong)))) {
            qCDebug(COMPONENTS::BATTERYCONTROL) << "error connecting to smoothed remaining time changes";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   SOLID_POWERMANAGEMENT_PATH,
                                                   SOLID_POWERMANAGEMENT_IFACE,
                                                   QStringLiteral("chargeStopThresholdChanged"),
                                                   this,
                                                   SLOT(updateBatteryChargeStopThreshold(int)))) {
            qCDebug(COMPONENTS::BATTERYCONTROL) << "error connecting to charge stop threshold changes via dbus";
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
                qCDebug(COMPONENTS::BATTERYCONTROL) << "Fail to retrieve power save status";
            }
            watcher->deleteLater();
        });

        if (!QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.PowerManagement"),
                                                   QStringLiteral("/org/freedesktop/PowerManagement"),
                                                   QStringLiteral("org.freedesktop.PowerManagement"),
                                                   QStringLiteral("PowerSaveStatusChanged"),
                                                   this,
                                                   SLOT(updateAcPlugState(bool)))) {
            qCDebug(COMPONENTS::BATTERYCONTROL) << "error connecting to power save status changes via dbus";
        }
    }
}

void BatteryControlModel::onServiceUnregistered(const QString &serviceName)
{
    if (serviceName == SOLID_POWERMANAGEMENT_SERVICE) {
        disconnect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &BatteryControlModel::deviceAdded);
        disconnect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &BatteryControlModel::deviceRemoved);

        QDBusConnection::sessionBus().disconnect(SOLID_POWERMANAGEMENT_SERVICE,
                                                 SOLID_POWERMANAGEMENT_PATH,
                                                 SOLID_POWERMANAGEMENT_IFACE,
                                                 QStringLiteral("batteryRemainingTimeChanged"),
                                                 this,
                                                 SLOT(batteryRemainingTimeChanged(qulonglong)));
        QDBusConnection::sessionBus().disconnect(SOLID_POWERMANAGEMENT_SERVICE,
                                                 SOLID_POWERMANAGEMENT_PATH,
                                                 SOLID_POWERMANAGEMENT_IFACE,
                                                 QStringLiteral("smoothedBatteryRemainingTimeChanged"),
                                                 this,
                                                 SLOT(smoothedBatteryRemainingTimeChanged(qulonglong)));
        QDBusConnection::sessionBus().disconnect(SOLID_POWERMANAGEMENT_SERVICE,
                                                 SOLID_POWERMANAGEMENT_PATH,
                                                 SOLID_POWERMANAGEMENT_IFACE,
                                                 QStringLiteral("chargeStopThresholdChanged"),
                                                 this,
                                                 SLOT(updateBatteryChargeStopThreshold(int)));
        QDBusConnection::sessionBus().disconnect(QStringLiteral("org.freedesktop.PowerManagement"),
                                                 QStringLiteral("/org/freedesktop/PowerManagement"),
                                                 QStringLiteral("org.freedesktop.PowerManagement"),
                                                 QStringLiteral("PowerSaveStatusChanged"),
                                                 this,
                                                 SLOT(updateAcPlugState(bool)));

        const QList<QString> batteries = m_batterySources;
        for (const auto &udi : batteries) {
            // clear m_batterySources, m_batteryPositions, m_internalBatteries, m_namesMonitor
            deviceRemoved(udi);
        }
        m_hasBatteries = false;
        m_hasInternalBatteries = false;
        m_hasCumulative = false;
        m_pluggedIn = false;
        m_state = NoCharge;
        m_chargeStopThreshold = 0;
        m_remainingMsec = 0;
        m_smoothedRemainingMsec = 0;
        m_percent = 0;
    }
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
        return QVariant::fromValue(m_namesMonitor->updateBatteryName(deviceBattery, battery));
    case Type:
        return QVariant::fromValue(batteryTypeToString(battery));
    }
    return {};
}

QHash<int, QByteArray> BatteryControlModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Percent] = QByteArrayLiteral("Percent");
    roles[Capacity] = QByteArrayLiteral("Capacity");
    roles[Energy] = QByteArrayLiteral("Energy");
    roles[PluggedIn] = QByteArrayLiteral("PluggedIn");
    roles[IsPowerSupply] = QByteArrayLiteral("IsPowerSupply");
    roles[ChargeState] = QByteArrayLiteral("ChargeState");
    roles[PrettyName] = QByteArrayLiteral("PrettyName");
    roles[Type] = QByteArrayLiteral("Type");
    return roles;
}

void BatteryControlModel::deviceAdded(const QString &udi)
{
    qCDebug(COMPONENTS::BATTERYCONTROL) << "Device add signal arrived. Udi: " << udi;

    Solid::Device deviceBattery(udi);
    if (!deviceBattery.isValid()) {
        return;
    }
    auto *battery = deviceBattery.as<Solid::Battery>();

    if (!battery) {
        return;
    }

    if (battery->type() == Solid::Battery::PrimaryBattery) {
        m_internalBatteries.append(udi);

        m_hasInternalBatteries = true;

        qCDebug(COMPONENTS::BATTERYCONTROL) << "Is have internal batteries: true";

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

    qCDebug(COMPONENTS::BATTERYCONTROL) << "Position for battery with udi : " << udi << "initialized : " << position;

    m_batteryPositions[udi] = position;

    qCDebug(COMPONENTS::BATTERYCONTROL) << "Update Battery Position. Udi: " << udi << "Position: " << m_batteryPositions[udi];

    beginInsertRows(QModelIndex(), position, position);
    m_batterySources.append(udi);
    endInsertRows();

    qCDebug(COMPONENTS::BATTERYCONTROL) << "Battery with udi: " << udi << " is added";

    m_hasBatteries = true;

    updateOverallBattery();
}

void BatteryControlModel::deviceRemoved(const QString &udi)
{
    qCDebug(COMPONENTS::BATTERYCONTROL) << "Device remove signal arrived. Udi: " << udi;
    if (m_batterySources.isEmpty()) {
        return;
    }

    auto position = m_batteryPositions.constFind(udi);
    if (position == m_batteryPositions.constEnd()) {
        return;
    }

    m_namesMonitor->removeBatteryName(udi);

    if (m_internalBatteries.removeOne(udi)) {
        m_hasInternalBatteries = !m_internalBatteries.isEmpty();
        qCDebug(COMPONENTS::BATTERYCONTROL) << "Is have internal batteries: " << m_hasInternalBatteries;
    }

    if (auto deleteBattery = Solid::Device(udi).as<Solid::Battery>()) {
        deleteBattery->disconnect(this);
    }

    qCDebug(COMPONENTS::BATTERYCONTROL) << "battery with udi: " << udi << "at index: " << *position << "is removed";

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
            m_state = FullyCharged;
        } else if (charging) {
            m_state = Charging;
        } else if (discharging) {
            m_state = Discharging;
        } else if (noCharge) {
            // When we are using a charge threshold, the kernel
            // may stop charging within a percentage point of the actual threshold
            // and this is considered correct behavior, so we have to handle
            // that. See https://bugzilla.kernel.org/show_bug.cgi?id=215531.
            // Also, Upower may give us a status of "Not charging" rather than
            // "Fullycharged", so we need to account for that as well. See
            // https://gitlab.freedesktop.org/upower/upower/-/issues/142.
            if (m_pluggedIn && (m_percent >= m_chargeStopThreshold - 1 && m_percent <= m_chargeStopThreshold + 1)) {
                m_state = FullyCharged;
            } else {
                m_state = NoCharge;
            }
        }
    } else {
        m_state = NoCharge;
    }

    m_hasCumulative = hasCumulative;

    qCDebug(COMPONENTS::BATTERYCONTROL) << "____ Overall battery updated ____ \n"
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

QBindable<BatteryControlModel::ChargeStateEnum> BatteryControlModel::bindableState()
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

BatteryControlModel::ChargeStateEnum BatteryControlModel::updateBatteryState(const Solid::Battery *battery) const
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
        return FullyCharged;
    }

    // Otherwise, just look at the charge state
    switch (battery->chargeState()) {
    case Solid::Battery::ChargeState::NoCharge:
        return NoCharge;
    case Solid::Battery::ChargeState::FullyCharged:
        return FullyCharged;
    case Solid::Battery::ChargeState::Charging:
        return Charging;
    case Solid::Battery::ChargeState::Discharging:
        return Discharging;
    }
    Q_UNREACHABLE_RETURN(NoCharge);
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
