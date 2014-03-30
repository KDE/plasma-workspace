/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007-2008 Sebastian Kuegler <sebas@kde.org>
 *   CopyRight 2007 Maor Vanmak <mvanmak1@gmail.com>
 *   Copyright 2008 Dario Freddi <drf54321@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "powermanagementengine.h"

//solid specific includes
#include <solid/devicenotifier.h>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/battery.h>
#include <solid/powermanagement.h>

#include <klocalizedstring.h>
#include <KIdleTime>

#include <QDebug>

#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusPendingCallWatcher>

#include <Plasma/DataContainer>
#include "powermanagementservice.h"

typedef QMap< QString, QString > StringStringMap;
Q_DECLARE_METATYPE(StringStringMap)

PowermanagementEngine::PowermanagementEngine(QObject* parent, const QVariantList& args)
        : Plasma::DataEngine(parent, args)
        , m_sources(basicSourceNames())
{
    Q_UNUSED(args)
    qDBusRegisterMetaType< StringStringMap >();
    init();
}

PowermanagementEngine::~PowermanagementEngine()
{}

void PowermanagementEngine::init()
{
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)),
            this,                              SLOT(deviceAdded(QString)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)),
            this,                              SLOT(deviceRemoved(QString)));

    // FIXME This check doesn't work, connect seems to always return true, hence the hack below
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.Solid.PowerManagement")) {
        if (!QDBusConnection::sessionBus().connect("org.kde.Solid.PowerManagement",
                                                   "/org/kde/Solid/PowerManagement/Actions/BrightnessControl",
                                                   "org.kde.Solid.PowerManagement.Actions.BrightnessControl",
                                                   "brightnessChanged", this,
                                                   SLOT(screenBrightnessChanged(int)))) {
            qDebug() << "error connecting to Brightness changes via dbus";
            brightnessControlsAvailableChanged(false);
        } else {
            brightnessControlsAvailableChanged(true);
        }

        if (!QDBusConnection::sessionBus().connect("org.kde.Solid.PowerManagement",
                                                   "/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl",
                                                   "org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl",
                                                   "keyboardBrightnessChanged", this,
                                                   SLOT(keyboardBrightnessChanged(int)))) {
            qDebug() << "error connecting to Keyboard Brightness changes via dbus";
            keyboardBrightnessControlsAvailableChanged(false);
        } else {
            keyboardBrightnessControlsAvailableChanged(true);
        }

        sourceRequestEvent("PowerDevil");

        if (!QDBusConnection::sessionBus().connect("org.kde.Solid.PowerManagement",
                                                   "/org/kde/Solid/PowerManagement",
                                                   "org.kde.Solid.PowerManagement",
                                                   "batteryRemainingTimeChanged", this,
                                                   SLOT(batteryRemainingTimeChanged(qulonglong)))) {
            qDebug() << "error connecting to remaining time changes";
        }
    }
}

QStringList PowermanagementEngine::basicSourceNames() const
{
    QStringList sources;
    sources << "Battery" << "AC Adapter" << "Sleep States" << "PowerDevil";
    return sources;
}

QStringList PowermanagementEngine::sources() const
{
    return m_sources;
}

bool PowermanagementEngine::sourceRequestEvent(const QString &name)
{
    if (name == "Battery") {
        const QList<Solid::Device> listBattery = Solid::Device::listFromType(Solid::DeviceInterface::Battery);
        m_batterySources.clear();

        if (listBattery.isEmpty()) {
            setData("Battery", "Has Battery", false);
            return true;
        }

        uint index = 0;
        QStringList batterySources;

        foreach (const Solid::Device &deviceBattery, listBattery) {
            const Solid::Battery* battery = deviceBattery.as<Solid::Battery>();

            const QString source = QString("Battery%1").arg(index++);

            batterySources << source;
            m_batterySources[deviceBattery.udi()] = source;

            connect(battery, SIGNAL(chargeStateChanged(int,QString)), this,
                    SLOT(updateBatteryChargeState(int,QString)));
            connect(battery, SIGNAL(chargePercentChanged(int,QString)), this,
                    SLOT(updateBatteryChargePercent(int,QString)));
            connect(battery, SIGNAL(plugStateChanged(bool,QString)), this,
                    SLOT(updateBatteryPlugState(bool,QString)));

            // Set initial values
            updateBatteryChargeState(battery->chargeState(), deviceBattery.udi());
            updateBatteryChargePercent(battery->chargePercent(), deviceBattery.udi());
            updateBatteryPlugState(battery->isPlugged(), deviceBattery.udi());
            updateBatteryPowerSupplyState(battery->isPowerSupply(), deviceBattery.udi());

            setData(source, "Vendor", deviceBattery.vendor());
            setData(source, "Product", deviceBattery.product());
            setData(source, "Capacity", battery->capacity());
            setData(source, "Type", batteryType(battery));
        }

        updateBatteryNames();

        setData("Battery", "Has Battery", !batterySources.isEmpty());
        if (!batterySources.isEmpty()) {
            setData("Battery", "Sources", batterySources);
            QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.Solid.PowerManagement",
                                                              "/org/kde/Solid/PowerManagement",
                                                              "org.kde.Solid.PowerManagement",
                                                              "batteryRemainingTime");
            QDBusPendingReply<qulonglong> reply = QDBusConnection::sessionBus().asyncCall(msg);
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
            QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                             this, SLOT(batteryRemainingTimeReply(QDBusPendingCallWatcher*)));
        }

        m_sources = basicSourceNames() + batterySources;
    } else if (name == "AC Adapter") {
        bool isPlugged = false;

        const QList<Solid::Device> list_ac = Solid::Device::listFromType(Solid::DeviceInterface::AcAdapter);
        foreach (const Solid::Device & device_ac, list_ac) {
            const Solid::AcAdapter* acadapter = device_ac.as<Solid::AcAdapter>();
            isPlugged |= acadapter->isPlugged();
            connect(acadapter, SIGNAL(plugStateChanged(bool,QString)), this,
                    SLOT(updateAcPlugState(bool)), Qt::UniqueConnection);
        }

        updateAcPlugState(isPlugged);
    } else if (name == "Sleep States") {
        const QSet<Solid::PowerManagement::SleepState> sleepstates =
                                Solid::PowerManagement::supportedSleepStates();
        // We first set all possible sleepstates to false, then enable the ones that are available
        setData("Sleep States", "Standby", false);
        setData("Sleep States", "Suspend", false);
        setData("Sleep States", "Hibernate", false);

        foreach (const Solid::PowerManagement::SleepState &sleepstate, sleepstates) {
            if (sleepstate == Solid::PowerManagement::StandbyState) {
                setData("Sleep States", "Standby", true);
            } else if (sleepstate == Solid::PowerManagement::SuspendState) {
                setData("Sleep States", "Suspend", true);
            } else if (sleepstate == Solid::PowerManagement::HibernateState) {
                setData("Sleep States", "Hibernate", true);
            }
            //qDebug() << "Sleepstate \"" << sleepstate << "\" supported.";
        }
    } else if (name == "PowerDevil") {
        if (m_brightnessControlsAvailable) {
          QDBusMessage screenMsg = QDBusMessage::createMethodCall("org.kde.Solid.PowerManagement",
                                                                  "/org/kde/Solid/PowerManagement/Actions/BrightnessControl",
                                                                  "org.kde.Solid.PowerManagement.Actions.BrightnessControl",
                                                                  "brightness");
          QDBusPendingReply<int> screenReply = QDBusConnection::sessionBus().asyncCall(screenMsg);
          QDBusPendingCallWatcher *screenWatcher = new QDBusPendingCallWatcher(screenReply, this);
          QObject::connect(screenWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                              this, SLOT(screenBrightnessReply(QDBusPendingCallWatcher*)));
        }

        if (m_keyboardBrightnessControlsAvailable) {
          QDBusMessage keyboardMsg = QDBusMessage::createMethodCall("org.kde.Solid.PowerManagement",
                                                                    "/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl",
                                                                    "org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl",
                                                                    "keyboardBrightness");
          QDBusPendingReply<int> keyboardReply = QDBusConnection::sessionBus().asyncCall(keyboardMsg);
          QDBusPendingCallWatcher *keyboardWatcher = new QDBusPendingCallWatcher(keyboardReply, this);
          QObject::connect(keyboardWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                              this, SLOT(keyboardBrightnessReply(QDBusPendingCallWatcher*)));
        }
    //any info concerning lock screen/screensaver goes here
    } else if (name == "UserActivity") {
        setData("UserActivity", "IdleTime", KIdleTime::instance()->idleTime());
    } else {
        qDebug() << "Data for '" << name << "' not found";
        return false;
    }
    return true;
}

QString PowermanagementEngine::batteryType(const Solid::Battery* battery)
{
  switch(battery->type()) {
      case Solid::Battery::PrimaryBattery:
          return QLatin1String("Battery");
          break;
      case Solid::Battery::UpsBattery:
          return QLatin1String("Ups");
          break;
      case Solid::Battery::MonitorBattery:
          return QLatin1String("Monitor");
          break;
      case Solid::Battery::MouseBattery:
          return QLatin1String("Mouse");
          break;
      case Solid::Battery::KeyboardBattery:
          return QLatin1String("Keyboad");
          break;
      case Solid::Battery::PdaBattery:
          return QLatin1String("Pda");
          break;
      case Solid::Battery::PhoneBattery:
          return QLatin1String("Phone");
          break;
      default:
          return QLatin1String("Unknown");
  }

  return QLatin1String("Unknown");
}

bool PowermanagementEngine::updateSourceEvent(const QString &source)
{
    if (source == "UserActivity") {
        setData("UserActivity", "IdleTime", KIdleTime::instance()->idleTime());
        return true;
    }
    return Plasma::DataEngine::updateSourceEvent(source);
}

Plasma::Service* PowermanagementEngine::serviceForSource(const QString &source)
{
    if (source == "PowerDevil") {
        return new PowerManagementService(this);
    }

    return 0;
}

void PowermanagementEngine::updateBatteryChargeState(int newState, const QString& udi)
{
    QString state("Unknown");
    if (newState == Solid::Battery::NoCharge) {
        state = "NoCharge";
    } else if (newState == Solid::Battery::Charging) {
        state = "Charging";
    } else if (newState == Solid::Battery::Discharging) {
        state = "Discharging";
    }

    const QString source = m_batterySources[udi];
    setData(source, "State", state);
}

void PowermanagementEngine::updateBatteryPlugState(bool newState, const QString& udi)
{
    const QString source = m_batterySources[udi];
    setData(source, "Plugged in", newState);
}

void PowermanagementEngine::updateBatteryChargePercent(int newValue, const QString& udi)
{
    const QString source = m_batterySources[udi];
    setData(source, "Percent", newValue);
}

void PowermanagementEngine::updateBatteryPowerSupplyState(bool newState, const QString& udi)
{
    const QString source = m_batterySources[udi];
    setData(source, "Is Power Supply", newState);
}

void PowermanagementEngine::updateBatteryNames()
{
    uint unnamedBatteries = 0;
    foreach (QString source, m_batterySources) {
        DataContainer *batteryDataContainer = containerForSource(source);
        if (batteryDataContainer) {
            const QString batteryVendor = batteryDataContainer->data()["Vendor"].toString();
            const QString batteryProduct = batteryDataContainer->data()["Product"].toString();

            // Don't show battery name for primary power supply batteries. They usually have cryptic serial number names.
            const bool showBatteryName = batteryDataContainer->data()["Type"].toString() != QLatin1String("Battery") ||
                                         !batteryDataContainer->data()["Is Power Supply"].toBool();

            if (!batteryProduct.isEmpty() && batteryProduct != "Unknown Battery" && showBatteryName) {
                if (!batteryVendor.isEmpty()) {
                    setData(source, "Pretty Name", QString(batteryVendor + ' ' + batteryProduct));
                } else {
                    setData(source, "Pretty Name", batteryProduct);
                }
            } else {
                ++unnamedBatteries;
                if (unnamedBatteries > 1) {
                    setData(source, "Pretty Name", i18nc("Placeholder is the battery number", "Battery %1", unnamedBatteries));
                } else {
                    setData(source, "Pretty Name", i18n("Battery"));
                }
            }
        }
    }
}

void PowermanagementEngine::updateAcPlugState(bool newState)
{
    setData("AC Adapter", "Plugged in", newState);
}

void PowermanagementEngine::deviceRemoved(const QString& udi)
{
    if (m_batterySources.contains(udi)) {
        Solid::Device device(udi);
        Solid::Battery* battery = device.as<Solid::Battery>();
        if (battery)
            battery->disconnect();

        const QString source = m_batterySources[udi];
        m_batterySources.remove(udi);
        removeSource(source);

        QStringList sourceNames(m_batterySources.values());
        sourceNames.removeAll(source);
        setData("Battery", "Sources", sourceNames);
        setData("Battery", "Has Battery", !sourceNames.isEmpty());
    }
}

void PowermanagementEngine::deviceAdded(const QString& udi)
{
    Solid::Device device(udi);
    if (device.isValid()) {
        const Solid::Battery* battery = device.as<Solid::Battery>();

        if (battery) {
            int index = 0;
            QStringList sourceNames(m_batterySources.values());
            while (sourceNames.contains(QString("Battery%1").arg(index))) {
                index++;
            }

            const QString source = QString("Battery%1").arg(index);
            sourceNames << source;
            m_batterySources[device.udi()] = source;

            connect(battery, SIGNAL(chargeStateChanged(int,QString)), this,
                    SLOT(updateBatteryChargeState(int,QString)));
            connect(battery, SIGNAL(chargePercentChanged(int,QString)), this,
                    SLOT(updateBatteryChargePercent(int,QString)));
            connect(battery, SIGNAL(plugStateChanged(bool,QString)), this,
                    SLOT(updateBatteryPlugState(bool,QString)));
            connect(battery, SIGNAL(powerSupplyStateChanged(bool,QString)), this,
                    SLOT(updateBatteryPowerSupplyState(bool,QString)));

            // Set initial values
            updateBatteryChargeState(battery->chargeState(), device.udi());
            updateBatteryChargePercent(battery->chargePercent(), device.udi());
            updateBatteryPlugState(battery->isPlugged(), device.udi());
            updateBatteryPowerSupplyState(battery->isPowerSupply(), device.udi());

            setData(source, "Vendor", device.vendor());
            setData(source, "Product", device.product());
            setData(source, "Capacity", battery->capacity());
            setData(source, "Type", batteryType(battery));

            setData("Battery", "Sources", sourceNames);
            setData("Battery", "Has Battery", !sourceNames.isEmpty());

            updateBatteryNames();
        }
    }
}

void PowermanagementEngine::batteryRemainingTimeChanged(qulonglong time)
{
    //qDebug() << "Remaining time 2:" << time;
    setData("Battery", "Remaining msec", time);
}

void PowermanagementEngine::brightnessControlsAvailableChanged(bool available)
{
    setData("PowerDevil", "Screen Brightness Available", available);
    m_brightnessControlsAvailable = available;
}

void PowermanagementEngine::keyboardBrightnessControlsAvailableChanged(bool available)
{
    setData("PowerDevil", "Keyboard Brightness Available", available);
    m_keyboardBrightnessControlsAvailable = available;
}

void PowermanagementEngine::batteryRemainingTimeReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<qulonglong> reply = *watcher;
    if (reply.isError()) {
        qDebug() << "Error getting battery remaining time: " << reply.error().message();
    } else {
        batteryRemainingTimeChanged(reply.value());
    }

    watcher->deleteLater();
}

void PowermanagementEngine::screenBrightnessChanged(int brightness)
{
    setData("PowerDevil", "Screen Brightness", brightness);
}

void PowermanagementEngine::keyboardBrightnessChanged(int brightness)
{
    setData("PowerDevil", "Keyboard Brightness", brightness);
}

void PowermanagementEngine::screenBrightnessReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<int> reply = *watcher;
    if (reply.isError()) {
        qDebug() << "Error getting screen brightness: " << reply.error().message();
        // FIXME Because the above check doesn't work, we unclaim backlight support as soon as it fails
        brightnessControlsAvailableChanged(false);
    } else {
        screenBrightnessChanged(reply.value());
    }

    watcher->deleteLater();
}

void PowermanagementEngine::keyboardBrightnessReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<int> reply = *watcher;
    if (reply.isError()) {
        qDebug() << "Error getting keyboard brightness: " << reply.error().message();
        // FIXME Because the above check doesn't work, we unclaim backlight support as soon as it fails
        keyboardBrightnessControlsAvailableChanged(false);
    } else {
        keyboardBrightnessChanged(reply.value());
    }

    watcher->deleteLater();
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(powermanagement, PowermanagementEngine, "plasma-dataengine-powermanagement.json")

#include "powermanagementengine.moc"
