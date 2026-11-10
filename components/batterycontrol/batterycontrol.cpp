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
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDBusServiceWatcher>

#include <klocalizedstring.h>
#include <sessionmanagement.h>

#include <algorithm>
#include <utility>

#include "batteriesnamesmonitor_p.h"
#include "upowerbattery_p.h"

static const QString SOLID_POWERMANAGEMENT_SERVICE = QStringLiteral("org.kde.Solid.PowerManagement");
static const QString SOLID_POWERMANAGEMENT_PATH = QStringLiteral("/org/kde/Solid/PowerManagement");
static const QString SOLID_POWERMANAGEMENT_IFACE = QStringLiteral("org.kde.Solid.PowerManagement");

static const QString UPOWER_SERVICE = QStringLiteral("org.freedesktop.UPower");
static const QString UPOWER_PATH = QStringLiteral("/org/freedesktop/UPower");
static const QString UPOWER_IFACE = QStringLiteral("org.freedesktop.UPower");

namespace
{
using Change = UPowerBattery::Change;

constexpr UPowerBattery::Changes overallBatteryChanges = Change::Present | Change::ChargePercent | Change::ChargeState | Change::Energy | Change::EnergyFull;

QList<int> rolesForChanges(UPowerBattery::Changes changes)
{
    QList<int> roles;
    if (changes.testFlag(Change::Present)) {
        roles << BatteryControlModel::PluggedIn;
    }
    if (changes.testFlag(Change::ChargePercent)) {
        roles << BatteryControlModel::Percent;
    }
    if (changes.testFlag(Change::Capacity)) {
        roles << BatteryControlModel::Capacity;
    }
    if (changes.testFlag(Change::PowerSupply)) {
        roles << BatteryControlModel::IsPowerSupply;
    }
    if (changes.testFlag(Change::ChargeState)) {
        roles << BatteryControlModel::ChargeState;
    }
    if (changes.testFlag(Change::Energy)) {
        roles << BatteryControlModel::Energy;
    }
    return roles;
}
}

BatteryControlModel::BatteryControlModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_namesMonitor(new BatteriesNamesMonitor)
    , m_solidWatcher(new QDBusServiceWatcher)
    , m_upowerWatcher(new QDBusServiceWatcher)
{
    m_internalBatteries.reserve(2);

    QDBusConnection systemBus = QDBusConnection::systemBus();

    m_upowerWatcher->setConnection(systemBus);
    m_upowerWatcher->setWatchMode(QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
    m_upowerWatcher->addWatchedService(UPOWER_SERVICE);
    connect(m_upowerWatcher.get(), &QDBusServiceWatcher::serviceRegistered, this, &BatteryControlModel::loadDevices);
    connect(m_upowerWatcher.get(), &QDBusServiceWatcher::serviceUnregistered, this, &BatteryControlModel::onUPowerServiceUnregistered);

    systemBus.connect(UPOWER_SERVICE, UPOWER_PATH, UPOWER_IFACE, QStringLiteral("DeviceAdded"), this, SLOT(onUPowerDeviceAdded(QDBusObjectPath)));
    systemBus.connect(UPOWER_SERVICE, UPOWER_PATH, UPOWER_IFACE, QStringLiteral("DeviceRemoved"), this, SLOT(onUPowerDeviceRemoved(QDBusObjectPath)));

    auto sessionManagement = new SessionManagement(this);
    connect(sessionManagement, &SessionManagement::resumingFromSuspend, this, &BatteryControlModel::onResumingFromSuspend);

    m_hasBatteries = false;
    m_hasCumulative = false;
    loadDevices();

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
    if (serviceName == SOLID_POWERMANAGEMENT_SERVICE) {
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
    }
}

int BatteryControlModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_batterySources.size();
}

QVariant BatteryControlModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }

    const auto it = m_batteries.find(m_batterySources[index.row()]);
    if (it == m_batteries.end()) {
        return {};
    }

    const UPowerBattery *battery = it->second.get();

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
        return QVariant::fromValue(m_namesMonitor->updateBatteryName(*battery));
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

    if (m_batteries.contains(udi) || m_pendingBatteries.contains(udi)) {
        return;
    }

    m_batteryOrder[udi] = m_nextBatteryOrder++;

    auto newBattery = std::make_unique<UPowerBattery>(udi);
    UPowerBattery *battery = newBattery.get();
    connect(battery, &UPowerBattery::loadFinished, this, &BatteryControlModel::onBatteryLoadFinished);
    m_pendingBatteries[udi] = std::move(newBattery);
    battery->load();
}

void BatteryControlModel::onBatteryLoadFinished(const QString &udi, bool isBattery)
{
    const auto it = m_pendingBatteries.find(udi);
    if (it == m_pendingBatteries.end()) {
        return;
    }

    std::unique_ptr<UPowerBattery> battery = std::move(it->second);
    m_pendingBatteries.erase(it);

    if (isBattery) {
        addBattery(std::move(battery));
    } else {
        m_batteryOrder.remove(udi);
        battery->disconnect(this);
        battery.release()->deleteLater();
    }
}

void BatteryControlModel::addBattery(std::unique_ptr<UPowerBattery> newBattery)
{
    const QString udi = newBattery->udi();
    UPowerBattery *battery = newBattery.get();
    m_batteries[udi] = std::move(newBattery);

    // Keep internal batteries first and order each group by when UPower announced the device,
    // independently of the order in which property replies arrive.
    const auto announcedBefore = [this](const QString &lhs, const QString &rhs) {
        return m_batteryOrder.value(lhs) < m_batteryOrder.value(rhs);
    };

    int position = 0;
    if (battery->type() == UPowerBattery::Type::Primary) {
        position = std::ranges::lower_bound(std::as_const(m_internalBatteries), udi, announcedBefore) - m_internalBatteries.cbegin();

        m_internalBatteries.insert(position, udi);

        m_hasInternalBatteries = true;

        qCDebug(COMPONENTS::BATTERYCONTROL) << "Is have internal batteries: true";
    } else {
        const auto externalBegin = m_batterySources.cbegin() + m_internalBatteries.size();
        position = std::lower_bound(externalBegin, m_batterySources.cend(), udi, announcedBefore) - m_batterySources.cbegin();
    }

    connect(battery, &UPowerBattery::changed, this, [this](UPowerBattery::Changes changes, const QString &udi) {
        onBatteryChanged(udi, rolesForChanges(changes), changes.testAnyFlags(overallBatteryChanges));
    });

    qCDebug(COMPONENTS::BATTERYCONTROL) << "Position for battery with udi : " << udi << "initialized : " << position;

    m_batteryPositions[udi] = position;

    qCDebug(COMPONENTS::BATTERYCONTROL) << "Update Battery Position. Udi: " << udi << "Position: " << m_batteryPositions[udi];

    beginInsertRows(QModelIndex(), position, position);

    m_batterySources.insert(position, udi);

    for (int newPosition = position + 1; newPosition < m_batterySources.size(); ++newPosition) {
        m_batteryPositions[m_batterySources[newPosition]] = newPosition;
    }

    endInsertRows();

    qCDebug(COMPONENTS::BATTERYCONTROL) << "Battery with udi: " << udi << " is added";

    m_hasBatteries = true;

    updateOverallBattery();
}

void BatteryControlModel::deviceRemoved(const QString &udi)
{
    qCDebug(COMPONENTS::BATTERYCONTROL) << "Device remove signal arrived. Udi: " << udi;

    if (m_pendingBatteries.erase(udi) > 0) {
        m_batteryOrder.remove(udi);
        return;
    }

    if (m_batterySources.isEmpty()) {
        return;
    }

    auto position = m_batteryPositions.constFind(udi);
    if (position == m_batteryPositions.constEnd()) {
        return;
    }

    m_namesMonitor->removeBatteryName(udi);

    const bool wasInternal = m_internalBatteries.removeOne(udi);
    if (wasInternal) {
        m_hasInternalBatteries = !m_internalBatteries.isEmpty();
        qCDebug(COMPONENTS::BATTERYCONTROL) << "Is have internal batteries: " << m_hasInternalBatteries;
    }

    qCDebug(COMPONENTS::BATTERYCONTROL) << "battery with udi: " << udi << "at index: " << *position << "is removed";

    beginRemoveRows(QModelIndex(), *position, *position);

    for (int newPosition = *position + 1; newPosition < m_batterySources.size(); ++newPosition) {
        m_batteryPositions[m_batterySources[newPosition]] = newPosition - 1;
    }

    m_batterySources.removeAt(*position);
    endRemoveRows();

    m_batteryPositions.erase(position);

    m_hasBatteries = !m_batterySources.isEmpty();

    m_batteries.erase(udi);
    m_batteryOrder.remove(udi);

    if (wasInternal) {
        updateOverallBattery();
    }
}

void BatteryControlModel::onUPowerDeviceAdded(const QDBusObjectPath &path)
{
    deviceAdded(path.path());
}

void BatteryControlModel::onUPowerDeviceRemoved(const QDBusObjectPath &path)
{
    deviceRemoved(path.path());
}

void BatteryControlModel::loadDevices()
{
    // A newer enumeration supersedes the one in flight; deleting its watcher drops the outdated reply.
    delete m_enumerationWatcher;

    const QDBusMessage message = QDBusMessage::createMethodCall(UPOWER_SERVICE, UPOWER_PATH, UPOWER_IFACE, QStringLiteral("EnumerateDevices"));
    m_enumerationWatcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message), this);
    connect(m_enumerationWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        m_enumerationWatcher = nullptr;
        watcher->deleteLater();

        const QDBusPendingReply<QList<QDBusObjectPath>> reply = *watcher;
        if (reply.isError()) {
            qCWarning(COMPONENTS::BATTERYCONTROL) << "error enumerating UPower devices:" << reply.error();
            return;
        }

        const QList<QDBusObjectPath> paths = reply.value();
        for (const QDBusObjectPath &path : paths) {
            deviceAdded(path.path());
        }
    });
}

void BatteryControlModel::onUPowerServiceUnregistered()
{
    delete std::exchange(m_enumerationWatcher, nullptr);
    m_pendingBatteries.clear();
    m_batteryOrder.clear();

    if (m_batterySources.isEmpty()) {
        return;
    }

    beginResetModel();
    for (const QString &udi : std::as_const(m_batterySources)) {
        m_namesMonitor->removeBatteryName(udi);
    }
    m_batteries.clear();
    m_batterySources.clear();
    m_batteryPositions.clear();
    m_internalBatteries.clear();
    endResetModel();

    m_hasInternalBatteries = false;
    m_hasBatteries = false;
    updateOverallBattery();
}

void BatteryControlModel::onResumingFromSuspend()
{
    for (const auto &[udi, battery] : m_batteries) {
        battery->load();
    }
    for (const auto &[udi, battery] : m_pendingBatteries) {
        battery->load();
    }
}

void BatteryControlModel::onBatteryChanged(const QString &udi, const QList<int> &roles, bool overallChanged)
{
    if (overallChanged && m_internalBatteries.contains(udi)) {
        updateOverallBattery();
    }

    if (!roles.isEmpty()) {
        const QModelIndex index = BatteryControlModel::index(m_batteryPositions[udi]);
        Q_EMIT dataChanged(index, index, roles);
    }
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
        const UPowerBattery *battery = m_batteries.at(internalBattery).get();
        if (battery->isPresent()) {
            hasCumulative = true;

            energy += battery->energy();
            totalEnergy += battery->energyFull();
            totalPercentage += battery->chargePercent();
            allFullyCharged = allFullyCharged && (battery->chargeState() == UPowerBattery::ChargeState::FullyCharged);
            charging = charging || (battery->chargeState() == UPowerBattery::ChargeState::Charging);
            discharging = discharging || (battery->chargeState() == UPowerBattery::ChargeState::Discharging);
            noCharge = noCharge || (battery->chargeState() == UPowerBattery::ChargeState::NoCharge);
            ++count;
        }
    }

    if (hasCumulative) {
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

        m_pluggedIn = !discharging;

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
        m_percent = 0;
        m_pluggedIn = true;
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

BatteryControlModel::ChargeStateEnum BatteryControlModel::updateBatteryState(const UPowerBattery *battery) const
{
    // When we are using a charge threshold, the kernel
    // may stop charging within a percentage point of the actual threshold
    // and this is considered correct behavior, so we have to handle
    // that. See https://bugzilla.kernel.org/show_bug.cgi?id=215531.
    if ((battery->chargePercent() >= m_chargeStopThreshold - 1 && battery->chargePercent() <= m_chargeStopThreshold + 1)
        // Also, Upower may give us a status of "Not charging" rather than
        // "Fully charged", so we need to account for that as well. See
        // https://gitlab.freedesktop.org/upower/upower/-/issues/142.
        && (battery->chargeState() == UPowerBattery::ChargeState::NoCharge || battery->chargeState() == UPowerBattery::ChargeState::FullyCharged)) {
        return FullyCharged;
    }

    // Otherwise, just look at the charge state
    switch (battery->chargeState()) {
    case UPowerBattery::ChargeState::NoCharge:
        return NoCharge;
    case UPowerBattery::ChargeState::FullyCharged:
        return FullyCharged;
    case UPowerBattery::ChargeState::Charging:
        return Charging;
    case UPowerBattery::ChargeState::Discharging:
        return Discharging;
    }
    Q_UNREACHABLE_RETURN(NoCharge);
}

QString BatteryControlModel::batteryTypeToString(const UPowerBattery *battery) const
{
    switch (battery->type()) {
    case UPowerBattery::Type::Primary:
        return QStringLiteral("Battery");
    case UPowerBattery::Type::Ups:
        return QStringLiteral("Ups");
    case UPowerBattery::Type::Mouse:
        return QStringLiteral("Mouse");
    case UPowerBattery::Type::Keyboard:
        return QStringLiteral("Keyboard");
    case UPowerBattery::Type::Pda:
        return QStringLiteral("Pda");
    case UPowerBattery::Type::Phone:
        return QStringLiteral("Phone");
    case UPowerBattery::Type::GamingInput:
        return QStringLiteral("GamingInput");
    case UPowerBattery::Type::Bluetooth:
        return QStringLiteral("Bluetooth");
    case UPowerBattery::Type::Tablet:
        return QStringLiteral("Tablet");
    case UPowerBattery::Type::Headphone:
        return QStringLiteral("Headphone");
    case UPowerBattery::Type::Headset:
        return QStringLiteral("Headset");
    case UPowerBattery::Type::Touchpad:
        return QStringLiteral("Touchpad");
    case UPowerBattery::Type::Wearable:
        return QStringLiteral("Wearable");
    case UPowerBattery::Type::Speakers:
        return QStringLiteral("Speakers");
    case UPowerBattery::Type::Unknown:
        return QStringLiteral("Unknown");
    }
    Q_UNREACHABLE_RETURN(QStringLiteral("Unknown"));
}

#include "moc_batterycontrol.cpp"
