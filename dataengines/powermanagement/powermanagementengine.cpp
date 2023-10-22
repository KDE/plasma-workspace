/*
    SPDX-FileCopyrightText: 2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2007-2008 Sebastian Kuegler <sebas@kde.org>
    SPDX-FileCopyrightText: 2007 Maor Vanmak <mvanmak1@gmail.com>
    SPDX-FileCopyrightText: 2008 Dario Freddi <drf54321@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "powermanagementengine.h"

// kde-workspace/libs
#include <sessionmanagement.h>

// solid specific includes
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>

#include <KAuthorized>
#include <KIdleTime>
#include <KService>
#include <klocalizedstring.h>

#include <QDebug>

#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QIcon>

#include "powermanagementservice.h"
#include <Plasma5Support/DataContainer>

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";
static const char FDO_POWERMANAGEMENT_SERVICE[] = "org.freedesktop.PowerManagement";

namespace
{
template<typename ReplyType>
void createAsyncDBusMethodCallAndCallback(QObject *parent,
                                          const QString &destination,
                                          const QString &path,
                                          const QString &interface,
                                          const QString &method,
                                          std::function<void(ReplyType)> &&callback)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(destination, path, interface, method);
    QDBusPendingReply<ReplyType> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, parent);
    parent->connect(watcher, &QDBusPendingCallWatcher::finished, parent, [callback](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<ReplyType> reply = *watcher;
        if (!reply.isError()) {
            callback(reply.value());
        }
        watcher->deleteLater();
    });
}
}

PowermanagementEngine::PowermanagementEngine(QObject *parent)
    : Plasma5Support::DataEngine(parent)
    , m_sources(basicSourceNames())
    , m_session(new SessionManagement(this))
{
    Q_UNREACHABLE();
    qDBusRegisterMetaType<QList<InhibitionInfo>>();
    qDBusRegisterMetaType<InhibitionInfo>();
    qDBusRegisterMetaType<QList<QVariant>>();
    qDBusRegisterMetaType<QList<QVariantMap>>();
    init();
}

PowermanagementEngine::~PowermanagementEngine()
{
}

void PowermanagementEngine::init()
{
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &PowermanagementEngine::deviceAdded);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &PowermanagementEngine::deviceRemoved);

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                   QStringLiteral("brightnessChanged"),
                                                   this,
                                                   SLOT(screenBrightnessChanged(int)))) {
            qDebug() << "error connecting to Brightness changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                   QStringLiteral("brightnessMaxChanged"),
                                                   this,
                                                   SLOT(maximumScreenBrightnessChanged(int)))) {
            qDebug() << "error connecting to max brightness changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                   QStringLiteral("keyboardBrightnessChanged"),
                                                   this,
                                                   SLOT(keyboardBrightnessChanged(int)))) {
            qDebug() << "error connecting to Keyboard Brightness changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                   QStringLiteral("keyboardBrightnessMaxChanged"),
                                                   this,
                                                   SLOT(maximumKeyboardBrightnessChanged(int)))) {
            qDebug() << "error connecting to max keyboard Brightness changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/HandleButtonEvents"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.HandleButtonEvents"),
                                                   QStringLiteral("triggersLidActionChanged"),
                                                   this,
                                                   SLOT(triggersLidActionChanged(bool)))) {
            qDebug() << "error connecting to lid action trigger changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                   QStringLiteral("InhibitionsChanged"),
                                                   this,
                                                   SLOT(inhibitionsChanged(QList<InhibitionInfo>, QStringList)))) {
            qDebug() << "error connecting to inhibition changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                   SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("batteryRemainingTimeChanged"),
                                                   this,
                                                   SLOT(batteryRemainingTimeChanged(qulonglong)))) {
            qDebug() << "error connecting to remaining time changes";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                   SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("smoothedBatteryRemainingTimeChanged"),
                                                   this,
                                                   SLOT(smoothedBatteryRemainingTimeChanged(qulonglong)))) {
            qDebug() << "error connecting to smoothed remaining time changes";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                   SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("chargeStopThresholdChanged"),
                                                   this,
                                                   SLOT(chargeStopThresholdChanged(int)))) {
            qDebug() << "error connecting to charge stop threshold changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("currentProfileChanged"),
                                                   this,
                                                   SLOT(updatePowerProfileCurrentProfile(QString)))) {
            qDebug() << "error connecting to current profile changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("profileChoicesChanged"),
                                                   this,
                                                   SLOT(updatePowerProfileChoices(QStringList)))) {
            qDebug() << "error connecting to profile choices changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("performanceInhibitedReasonChanged"),
                                                   this,
                                                   SLOT(updatePowerProfilePerformanceInhibitedReason(QString)))) {
            qDebug() << "error connecting to inhibition reason changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("performanceDegradedReasonChanged"),
                                                   this,
                                                   SLOT(updatePowerProfilePerformanceDegradedReason(QString)))) {
            qDebug() << "error connecting to degradation reason changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                   QStringLiteral("profileHoldsChanged"),
                                                   this,
                                                   SLOT(updatePowerProfileHolds(QList<QVariantMap>)))) {
            qDebug() << "error connecting to profile hold changes via dbus";
        }
    }

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(FDO_POWERMANAGEMENT_SERVICE)) {
        if (!QDBusConnection::sessionBus().connect(FDO_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/freedesktop/PowerManagement"),
                                                   QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                   QStringLiteral("HasInhibitChanged"),
                                                   this,
                                                   SLOT(hasInhibitionChanged(bool)))) {
            qDebug() << "error connecting to fdo inhibition changes via dbus";
        }
    }
}

QStringList PowermanagementEngine::basicSourceNames() const
{
    QStringList sources;
    sources << QStringLiteral("Battery") << QStringLiteral("AC Adapter") << QStringLiteral("Sleep States") << QStringLiteral("PowerDevil")
            << QStringLiteral("Inhibitions") << QStringLiteral("Power Profiles") << QStringLiteral("PowerManagement");
    return sources;
}

QStringList PowermanagementEngine::sources() const
{
    return m_sources;
}

bool PowermanagementEngine::sourceRequestEvent(const QString &name)
{
    if (name == QLatin1String("Battery")) {
        const QList<Solid::Device> listBattery = Solid::Device::listFromType(Solid::DeviceInterface::Battery);
        m_batterySources.clear();

        if (listBattery.isEmpty()) {
            setData(QStringLiteral("Battery"), QStringLiteral("Has Battery"), false);
            setData(QStringLiteral("Battery"), QStringLiteral("Has Cumulative"), false);
            return true;
        }

        uint index = 0;
        QStringList batterySources;

        for (const Solid::Device &deviceBattery : listBattery) {
            const Solid::Battery *battery = deviceBattery.as<Solid::Battery>();

            const QString source = QStringLiteral("Battery%1").arg(index++);

            batterySources << source;
            m_batterySources[deviceBattery.udi()] = source;

            connect(battery, &Solid::Battery::chargeStateChanged, this, &PowermanagementEngine::updateBatteryChargeState);
            connect(battery, &Solid::Battery::chargePercentChanged, this, &PowermanagementEngine::updateBatteryChargePercent);
            connect(battery, &Solid::Battery::energyChanged, this, &PowermanagementEngine::updateBatteryEnergy);
            connect(battery, &Solid::Battery::presentStateChanged, this, &PowermanagementEngine::updateBatteryPresentState);

            // Set initial values
            updateBatteryChargeState(battery->chargeState(), deviceBattery.udi());
            updateBatteryChargePercent(battery->chargePercent(), deviceBattery.udi());
            updateBatteryEnergy(battery->energy(), deviceBattery.udi());
            updateBatteryPresentState(battery->isPresent(), deviceBattery.udi());
            updateBatteryPowerSupplyState(battery->isPowerSupply(), deviceBattery.udi());

            setData(source, QStringLiteral("Vendor"), deviceBattery.vendor());
            setData(source, QStringLiteral("Product"), deviceBattery.product());
            setData(source, QStringLiteral("Capacity"), battery->capacity());
            setData(source, QStringLiteral("Type"), batteryTypeToString(battery));
        }

        updateBatteryNames();
        updateOverallBattery();

        setData(QStringLiteral("Battery"), QStringLiteral("Sources"), batterySources);
        setData(QStringLiteral("Battery"), QStringLiteral("Has Battery"), !batterySources.isEmpty());
        if (!batterySources.isEmpty()) {
            createPowerManagementDBusMethodCallAndNotifyChanged<qulonglong>(
                QStringLiteral("batteryRemainingTime"),
                std::bind(&PowermanagementEngine::batteryRemainingTimeChanged, this, std::placeholders::_1));
            createPowerManagementDBusMethodCallAndNotifyChanged<qulonglong>(
                QStringLiteral("smoothedBatteryRemainingTime"),
                std::bind(&PowermanagementEngine::smoothedBatteryRemainingTimeChanged, this, std::placeholders::_1));
        }

        createPowerManagementDBusMethodCallAndNotifyChanged<int>(QStringLiteral("chargeStopThreshold"),
                                                                 std::bind(&PowermanagementEngine::chargeStopThresholdChanged, this, std::placeholders::_1));

        m_sources = basicSourceNames() + batterySources;
    } else if (name == QLatin1String("AC Adapter")) {
        QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.PowerManagement"),
                                              QStringLiteral("/org/freedesktop/PowerManagement"),
                                              QStringLiteral("org.freedesktop.PowerManagement"),
                                              QStringLiteral("PowerSaveStatusChanged"),
                                              this,
                                              SLOT(updateAcPlugState(bool)));

        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement"),
                                                          QStringLiteral("/org/freedesktop/PowerManagement"),
                                                          QStringLiteral("org.freedesktop.PowerManagement"),
                                                          QStringLiteral("GetPowerSaveStatus"));
        QDBusReply<bool> reply = QDBusConnection::sessionBus().call(msg);
        updateAcPlugState(reply.isValid() ? reply.value() : false);
    } else if (name == QLatin1String("Sleep States")) {
        setData(QStringLiteral("Sleep States"), QStringLiteral("Standby"), m_session->canSuspend());
        setData(QStringLiteral("Sleep States"), QStringLiteral("Suspend"), m_session->canSuspend());
        setData(QStringLiteral("Sleep States"), QStringLiteral("Hibernate"), m_session->canHibernate());
        setData(QStringLiteral("Sleep States"), QStringLiteral("HybridSuspend"), m_session->canHybridSuspend());
        setData(QStringLiteral("Sleep States"), QStringLiteral("LockScreen"), m_session->canLock());
        setData(QStringLiteral("Sleep States"), QStringLiteral("Logout"), m_session->canLogout());
    } else if (name == QLatin1String("PowerDevil")) {
        createAsyncDBusMethodCallAndCallback<int>(this,
                                                  SOLID_POWERMANAGEMENT_SERVICE,
                                                  QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                  QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                  QStringLiteral("brightness"),
                                                  std::bind(&PowermanagementEngine::screenBrightnessChanged, this, std::placeholders::_1));

        createAsyncDBusMethodCallAndCallback<int>(this,
                                                  SOLID_POWERMANAGEMENT_SERVICE,
                                                  QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                  QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                  QStringLiteral("brightnessMax"),
                                                  std::bind(&PowermanagementEngine::maximumScreenBrightnessChanged, this, std::placeholders::_1));

        createAsyncDBusMethodCallAndCallback<int>(this,
                                                  SOLID_POWERMANAGEMENT_SERVICE,
                                                  QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                  QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                  QStringLiteral("keyboardBrightness"),
                                                  std::bind(&PowermanagementEngine::keyboardBrightnessChanged, this, std::placeholders::_1));

        createAsyncDBusMethodCallAndCallback<int>(this,
                                                  SOLID_POWERMANAGEMENT_SERVICE,
                                                  QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                  QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                  QStringLiteral("keyboardBrightnessMax"),
                                                  std::bind(&PowermanagementEngine::maximumKeyboardBrightnessChanged, this, std::placeholders::_1));

        createPowerManagementDBusMethodCallAndNotifyChanged<bool>(QStringLiteral("isLidPresent"), [this](bool replyValue) {
            setData(QStringLiteral("PowerDevil"), QStringLiteral("Is Lid Present"), replyValue);
        });

        createAsyncDBusMethodCallAndCallback<bool>(this,
                                                   SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/HandleButtonEvents"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.HandleButtonEvents"),
                                                   QStringLiteral("triggersLidAction"),
                                                   std::bind(&PowermanagementEngine::triggersLidActionChanged, this, std::placeholders::_1));
    } else if (name == QLatin1String("PowerManagement")) {
        createAsyncDBusMethodCallAndCallback<bool>(this,
                                                   QStringLiteral("org.freedesktop.PowerManagement"),
                                                   QStringLiteral("/org/freedesktop/PowerManagement"),
                                                   QStringLiteral("org.freedesktop.PowerManagement"),
                                                   QStringLiteral("HasInhibit"),
                                                   [this](const bool &replyValue) {
                                                       hasInhibitionChanged(replyValue);
                                                   });

    } else if (name == QLatin1String("Inhibitions")) {
        createAsyncDBusMethodCallAndCallback<QList<InhibitionInfo>>(this,
                                                                    SOLID_POWERMANAGEMENT_SERVICE,
                                                                    QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent"),
                                                                    QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent"),
                                                                    QStringLiteral("ListInhibitions"),
                                                                    [this](const QList<InhibitionInfo> &replyValue) {
                                                                        removeAllData(QStringLiteral("Inhibitions"));
                                                                        inhibitionsChanged(replyValue, QStringList());
                                                                    });
        // any info concerning lock screen/screensaver goes here
    } else if (name == QLatin1String("UserActivity")) {
        setData(QStringLiteral("UserActivity"), QStringLiteral("IdleTime"), KIdleTime::instance()->idleTime());
    } else if (name == QLatin1String("Power Profiles")) {
        createPowerProfileDBusMethodCallAndNotifyChanged<QString>(
            QStringLiteral("currentProfile"),
            std::bind(&PowermanagementEngine::updatePowerProfileCurrentProfile, this, std::placeholders::_1));
        createPowerProfileDBusMethodCallAndNotifyChanged<QStringList>(
            QStringLiteral("profileChoices"),
            std::bind(&PowermanagementEngine::updatePowerProfileChoices, this, std::placeholders::_1));
        createPowerProfileDBusMethodCallAndNotifyChanged<QString>(
            QStringLiteral("performanceInhibitedReason"),
            std::bind(&PowermanagementEngine::updatePowerProfilePerformanceInhibitedReason, this, std::placeholders::_1));
        createPowerProfileDBusMethodCallAndNotifyChanged<QString>(
            QStringLiteral("performanceDegradedReason"),
            std::bind(&PowermanagementEngine::updatePowerProfilePerformanceDegradedReason, this, std::placeholders::_1));
        createPowerProfileDBusMethodCallAndNotifyChanged<QList<QVariantMap>>(
            QStringLiteral("profileHolds"),
            std::bind(&PowermanagementEngine::updatePowerProfileHolds, this, std::placeholders::_1));
    } else {
        qDebug() << "Data for" << name << "not found";
        return false;
    }
    return true;
}

QString PowermanagementEngine::batteryTypeToString(const Solid::Battery *battery) const
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
    default:
        return QStringLiteral("Unknown");
    }
}

bool PowermanagementEngine::updateSourceEvent(const QString &source)
{
    if (source == QLatin1String("UserActivity")) {
        setData(QStringLiteral("UserActivity"), QStringLiteral("IdleTime"), KIdleTime::instance()->idleTime());
        return true;
    }
    return Plasma5Support::DataEngine::updateSourceEvent(source);
}

Plasma5Support::Service *PowermanagementEngine::serviceForSource(const QString &source)
{
    if (source == QLatin1String("PowerDevil")) {
        return new PowerManagementService(this);
    }

    return nullptr;
}

QString PowermanagementEngine::batteryStateToString(int newState) const
{
    QString state(QStringLiteral("Unknown"));
    if (newState == Solid::Battery::NoCharge) {
        state = QLatin1String("NoCharge");
    } else if (newState == Solid::Battery::Charging) {
        state = QLatin1String("Charging");
    } else if (newState == Solid::Battery::Discharging) {
        state = QLatin1String("Discharging");
    } else if (newState == Solid::Battery::FullyCharged) {
        state = QLatin1String("FullyCharged");
    }

    return state;
}

void PowermanagementEngine::updateBatteryChargeState(int newState, const QString &udi)
{
    const QString source = m_batterySources[udi];
    setData(source, QStringLiteral("State"), batteryStateToString(newState));
    updateOverallBattery();
}

void PowermanagementEngine::updateBatteryPresentState(bool newState, const QString &udi)
{
    const QString source = m_batterySources[udi];
    setData(source, QStringLiteral("Plugged in"), newState); // FIXME This needs to be renamed and Battery Monitor adjusted
}

void PowermanagementEngine::updateBatteryChargePercent(int newValue, const QString &udi)
{
    const QString source = m_batterySources[udi];
    setData(source, QStringLiteral("Percent"), newValue);
    updateOverallBattery();
}

void PowermanagementEngine::updateBatteryEnergy(double newValue, const QString &udi)
{
    const QString source = m_batterySources[udi];
    setData(source, QStringLiteral("Energy"), newValue);
}

void PowermanagementEngine::updateBatteryPowerSupplyState(bool newState, const QString &udi)
{
    const QString source = m_batterySources[udi];
    setData(source, QStringLiteral("Is Power Supply"), newState);
}

void PowermanagementEngine::updateBatteryNames()
{
    uint unnamedBatteries = 0;
    for (const QString &source : std::as_const(m_batterySources)) {
        DataContainer *batteryDataContainer = containerForSource(source);
        if (batteryDataContainer) {
            const QString batteryVendor = batteryDataContainer->data()[QStringLiteral("Vendor")].toString();
            const QString batteryProduct = batteryDataContainer->data()[QStringLiteral("Product")].toString();

            // Don't show battery name for primary power supply batteries. They usually have cryptic serial number names.
            const bool showBatteryName = batteryDataContainer->data()[QStringLiteral("Type")].toString() != QLatin1String("Battery")
                || !batteryDataContainer->data()[QStringLiteral("Is Power Supply")].toBool();

            if (!batteryProduct.isEmpty() && batteryProduct != QLatin1String("Unknown Battery") && showBatteryName) {
                if (!batteryVendor.isEmpty()) {
                    setData(source, QStringLiteral("Pretty Name"), QString(batteryVendor + ' ' + batteryProduct));
                } else {
                    setData(source, QStringLiteral("Pretty Name"), batteryProduct);
                }
            } else {
                ++unnamedBatteries;
                if (unnamedBatteries > 1) {
                    setData(source, QStringLiteral("Pretty Name"), i18nc("Placeholder is the battery number", "Battery %1", unnamedBatteries));
                } else {
                    setData(source, QStringLiteral("Pretty Name"), i18n("Battery"));
                }
            }
        }
    }
}

void PowermanagementEngine::updateOverallBattery()
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
        setData(QStringLiteral("Battery"), QStringLiteral("Percent"), qRound(totalPercentage));
    } else if (totalEnergy > 0) {
        setData(QStringLiteral("Battery"), QStringLiteral("Percent"), qRound(energy / totalEnergy * 100));
    } else if (count > 0) { // UPS don't have energy, see Bug 348588
        setData(QStringLiteral("Battery"), QStringLiteral("Percent"), qRound(totalPercentage / static_cast<qreal>(count)));
    } else {
        setData(QStringLiteral("Battery"), QStringLiteral("Percent"), int(0));
    }

    if (hasCumulative) {
        if (allFullyCharged) {
            setData(QStringLiteral("Battery"), QStringLiteral("State"), "FullyCharged");
        } else if (charging) {
            setData(QStringLiteral("Battery"), QStringLiteral("State"), "Charging");
        } else if (noCharge) {
            setData(QStringLiteral("Battery"), QStringLiteral("State"), "NoCharge");
        } else {
            setData(QStringLiteral("Battery"), QStringLiteral("State"), "Discharging");
        }
    } else {
        setData(QStringLiteral("Battery"), QStringLiteral("State"), "Unknown");
    }

    setData(QStringLiteral("Battery"), QStringLiteral("Has Cumulative"), hasCumulative);
}

void PowermanagementEngine::updateAcPlugState(bool onBattery)
{
    setData(QStringLiteral("AC Adapter"), QStringLiteral("Plugged in"), !onBattery);
}

void PowermanagementEngine::updatePowerProfileCurrentProfile(const QString &activeProfile)
{
    setData(QStringLiteral("Power Profiles"), QStringLiteral("Current Profile"), activeProfile);
}

void PowermanagementEngine::updatePowerProfileChoices(const QStringList &choices)
{
    setData(QStringLiteral("Power Profiles"), QStringLiteral("Profiles"), choices);
}

void PowermanagementEngine::updatePowerProfilePerformanceInhibitedReason(const QString &reason)
{
    setData(QStringLiteral("Power Profiles"), QStringLiteral("Performance Inhibited Reason"), reason);
}

void PowermanagementEngine::updatePowerProfilePerformanceDegradedReason(const QString &reason)
{
    setData(QStringLiteral("Power Profiles"), QStringLiteral("Performance Degraded Reason"), reason);
}

void PowermanagementEngine::updatePowerProfileHolds(const QList<QVariantMap> &holds)
{
    QList<QVariantMap> out;
    std::transform(holds.cbegin(), holds.cend(), std::back_inserter(out), [this](const QVariantMap &hold) {
        QString prettyName;
        QString icon;
        populateApplicationData(hold[QStringLiteral("ApplicationId")].toString(), &prettyName, &icon);
        return QVariantMap{
            {QStringLiteral("Name"), prettyName},
            {QStringLiteral("Icon"), icon},
            {QStringLiteral("Reason"), hold[QStringLiteral("Reason")]},
            {QStringLiteral("Profile"), hold[QStringLiteral("Profile")]},
        };
    });
    setData(QStringLiteral("Power Profiles"), QStringLiteral("Profile Holds"), QVariant::fromValue(out));
}

void PowermanagementEngine::deviceRemoved(const QString &udi)
{
    if (m_batterySources.contains(udi)) {
        Solid::Device device(udi);
        Solid::Battery *battery = device.as<Solid::Battery>();
        if (battery) {
            battery->disconnect(this);
        }

        const QString source = m_batterySources[udi];
        m_batterySources.remove(udi);
        removeSource(source);

        QStringList sourceNames(m_batterySources.values());
        sourceNames.removeAll(source);
        setData(QStringLiteral("Battery"), QStringLiteral("Sources"), sourceNames);
        setData(QStringLiteral("Battery"), QStringLiteral("Has Battery"), !sourceNames.isEmpty());

        updateOverallBattery();
    }
}

void PowermanagementEngine::deviceAdded(const QString &udi)
{
    Solid::Device device(udi);
    if (device.isValid()) {
        const Solid::Battery *battery = device.as<Solid::Battery>();

        if (battery) {
            int index = 0;
            QStringList sourceNames(m_batterySources.values());
            while (sourceNames.contains(QStringLiteral("Battery%1").arg(index))) {
                index++;
            }

            const QString source = QStringLiteral("Battery%1").arg(index);
            sourceNames << source;
            m_batterySources[device.udi()] = source;

            connect(battery, &Solid::Battery::chargeStateChanged, this, &PowermanagementEngine::updateBatteryChargeState);
            connect(battery, &Solid::Battery::chargePercentChanged, this, &PowermanagementEngine::updateBatteryChargePercent);
            connect(battery, &Solid::Battery::energyChanged, this, &PowermanagementEngine::updateBatteryEnergy);
            connect(battery, &Solid::Battery::presentStateChanged, this, &PowermanagementEngine::updateBatteryPresentState);
            connect(battery, &Solid::Battery::powerSupplyStateChanged, this, &PowermanagementEngine::updateBatteryPowerSupplyState);

            // Set initial values
            updateBatteryChargeState(battery->chargeState(), device.udi());
            updateBatteryChargePercent(battery->chargePercent(), device.udi());
            updateBatteryEnergy(battery->energy(), device.udi());
            updateBatteryPresentState(battery->isPresent(), device.udi());
            updateBatteryPowerSupplyState(battery->isPowerSupply(), device.udi());

            setData(source, QStringLiteral("Vendor"), device.vendor());
            setData(source, QStringLiteral("Product"), device.product());
            setData(source, QStringLiteral("Capacity"), battery->capacity());
            setData(source, QStringLiteral("Type"), batteryTypeToString(battery));

            setData(QStringLiteral("Battery"), QStringLiteral("Sources"), sourceNames);
            setData(QStringLiteral("Battery"), QStringLiteral("Has Battery"), !sourceNames.isEmpty());

            updateBatteryNames();
            updateOverallBattery();
        }
    }
}

void PowermanagementEngine::batteryRemainingTimeChanged(qulonglong time)
{
    // qDebug() << "Remaining time 2:" << time;
    setData(QStringLiteral("Battery"), QStringLiteral("Remaining msec"), time);
}

void PowermanagementEngine::smoothedBatteryRemainingTimeChanged(qulonglong time)
{
    setData(QStringLiteral("Battery"), QStringLiteral("Smoothed Remaining msec"), time);
}

void PowermanagementEngine::screenBrightnessChanged(int brightness)
{
    setData(QStringLiteral("PowerDevil"), QStringLiteral("Screen Brightness"), brightness);
}

void PowermanagementEngine::maximumScreenBrightnessChanged(int maximumBrightness)
{
    setData(QStringLiteral("PowerDevil"), QStringLiteral("Maximum Screen Brightness"), maximumBrightness);
    setData(QStringLiteral("PowerDevil"), QStringLiteral("Screen Brightness Available"), maximumBrightness > 0);
}

void PowermanagementEngine::keyboardBrightnessChanged(int brightness)
{
    setData(QStringLiteral("PowerDevil"), QStringLiteral("Keyboard Brightness"), brightness);
}

void PowermanagementEngine::maximumKeyboardBrightnessChanged(int maximumBrightness)
{
    setData(QStringLiteral("PowerDevil"), QStringLiteral("Maximum Keyboard Brightness"), maximumBrightness);
    setData(QStringLiteral("PowerDevil"), QStringLiteral("Keyboard Brightness Available"), maximumBrightness > 0);
}

void PowermanagementEngine::triggersLidActionChanged(bool triggers)
{
    setData(QStringLiteral("PowerDevil"), QStringLiteral("Triggers Lid Action"), triggers);
}

void PowermanagementEngine::hasInhibitionChanged(bool inhibited)
{
    setData(QStringLiteral("PowerManagement"), QStringLiteral("Has Inhibition"), inhibited);
}

void PowermanagementEngine::inhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed)
{
    for (auto it = removed.constBegin(); it != removed.constEnd(); ++it) {
        removeData(QStringLiteral("Inhibitions"), (*it));
    }

    for (auto it = added.constBegin(); it != added.constEnd(); ++it) {
        const QString &name = (*it).first;
        QString prettyName;
        QString icon;
        const QString &reason = (*it).second;

        populateApplicationData(name, &prettyName, &icon);

        setData(QStringLiteral("Inhibitions"),
                name,
                QVariantMap{{QStringLiteral("Name"), prettyName}, {QStringLiteral("Icon"), icon}, {QStringLiteral("Reason"), reason}});
    }
}

template<typename ReplyType>
inline void PowermanagementEngine::createPowerManagementDBusMethodCallAndNotifyChanged(const QString &method, std::function<void(ReplyType)> &&callback)
{
    createAsyncDBusMethodCallAndCallback<ReplyType>(this,
                                                    SOLID_POWERMANAGEMENT_SERVICE,
                                                    QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                    SOLID_POWERMANAGEMENT_SERVICE,
                                                    method,
                                                    std::move(callback));
}

template<typename ReplyType>
inline void PowermanagementEngine::createPowerProfileDBusMethodCallAndNotifyChanged(const QString &method, std::function<void(ReplyType)> &&callback)
{
    createAsyncDBusMethodCallAndCallback<ReplyType>(this,
                                                    SOLID_POWERMANAGEMENT_SERVICE,
                                                    QStringLiteral("/org/kde/Solid/PowerManagement/Actions/PowerProfile"),
                                                    QStringLiteral("org.kde.Solid.PowerManagement.Actions.PowerProfile"),
                                                    method,
                                                    std::move(callback));
}

void PowermanagementEngine::populateApplicationData(const QString &name, QString *prettyName, QString *icon)
{
    if (m_applicationInfo.contains(name)) {
        const auto &info = m_applicationInfo.value(name);
        *prettyName = info.first;
        *icon = info.second;
    } else {
        KService::Ptr service = KService::serviceByStorageId(name + ".desktop");
        if (service) {
            *prettyName = service->name(); // cannot be null
            *icon = service->icon();

            m_applicationInfo.insert(name, qMakePair(*prettyName, *icon));
        } else {
            *prettyName = name;
            *icon = name.section(QLatin1Char('/'), -1).toLower();
            if (!QIcon::hasThemeIcon(*icon)) {
                icon->clear();
            }
        }
    }
}

void PowermanagementEngine::chargeStopThresholdChanged(int threshold)
{
    setData(QStringLiteral("Battery"), QStringLiteral("Charge Stop Threshold"), threshold);
}

K_PLUGIN_CLASS_WITH_JSON(PowermanagementEngine, "plasma-dataengine-powermanagement.json")

#include "powermanagementengine.moc"
