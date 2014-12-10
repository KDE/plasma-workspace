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

static const char SOLID_POWERMANAGEMENT_SERVICE[] = "org.kde.Solid.PowerManagement";

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

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(SOLID_POWERMANAGEMENT_SERVICE)) {
        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                   QStringLiteral("brightnessValueChanged"), this,
                                                   SLOT(screenBrightnessChanged(int)))) {
            qDebug() << "error connecting to Brightness changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                   QStringLiteral("brightnessValueMaxChanged"), this,
                                                   SLOT(maximumScreenBrightnessChanged(int)))) {
            qDebug() << "error connecting to max brightness changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                   QStringLiteral("keyboardBrightnessValueChanged"), this,
                                                   SLOT(keyboardBrightnessChanged(int)))) {
            qDebug() << "error connecting to Keyboard Brightness changes via dbus";
        }

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                   QStringLiteral("keyboardBrightnessValueMaxChanged"), this,
                                                   SLOT(maximumKeyboardBrightnessChanged(int)))) {
            qDebug() << "error connecting to max keyboard Brightness changes via dbus";
        }

        sourceRequestEvent("PowerDevil");

        if (!QDBusConnection::sessionBus().connect(SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("/org/kde/Solid/PowerManagement"),
                                                   SOLID_POWERMANAGEMENT_SERVICE,
                                                   QStringLiteral("batteryRemainingTimeChanged"), this,
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
            connect(battery, SIGNAL(presentStateChanged(bool,QString)), this,
                    SLOT(updateBatteryPresentState(bool,QString)));

            // Set initial values
            updateBatteryChargeState(battery->chargeState(), deviceBattery.udi());
            updateBatteryChargePercent(battery->chargePercent(), deviceBattery.udi());
            updateBatteryPresentState(battery->isPresent(), deviceBattery.udi());
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
            QDBusMessage msg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                              "/org/kde/Solid/PowerManagement",
                                                              SOLID_POWERMANAGEMENT_SERVICE,
                                                              "batteryRemainingTime");
            QDBusPendingReply<qulonglong> reply = QDBusConnection::sessionBus().asyncCall(msg);
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
            QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
                QDBusPendingReply<qulonglong> reply = *watcher;
                if (!reply.isError()) {
                    batteryRemainingTimeChanged(reply.value());
                }
                watcher->deleteLater();
            });
        }

        m_sources = basicSourceNames() + batterySources;
    } else if (name == "AC Adapter") {
        connect(Solid::PowerManagement::notifier(), SIGNAL(appShouldConserveResourcesChanged(bool)),
                this, SLOT(updateAcPlugState(bool)));
        updateAcPlugState(Solid::PowerManagement::appShouldConserveResources());
    } else if (name == "Sleep States") {
        const QSet<Solid::PowerManagement::SleepState> sleepstates = Solid::PowerManagement::supportedSleepStates();
        setData("Sleep States", "Standby", sleepstates.contains(Solid::PowerManagement::StandbyState));
        setData("Sleep States", "Suspend", sleepstates.contains(Solid::PowerManagement::SuspendState));
        setData("Sleep States", "Hibernate", sleepstates.contains(Solid::PowerManagement::HibernateState));
        setData("Sleep States", "HybridSuspend", sleepstates.contains(Solid::PowerManagement::HybridSuspendState));
    } else if (name == "PowerDevil") {
        QDBusMessage screenMsg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                              QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                              QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                              QStringLiteral("brightnessValue"));
        QDBusPendingReply<int> screenReply = QDBusConnection::sessionBus().asyncCall(screenMsg);
        QDBusPendingCallWatcher *screenWatcher = new QDBusPendingCallWatcher(screenReply, this);
        QObject::connect(screenWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<int> reply = *watcher;
            if (!reply.isError()) {
                screenBrightnessChanged(reply.value());
            }
            watcher->deleteLater();
        });

        QDBusMessage maxScreenMsg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                  QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                                                  QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"),
                                                                  QStringLiteral("brightnessValueMax"));
        QDBusPendingReply<int> maxScreenReply = QDBusConnection::sessionBus().asyncCall(maxScreenMsg);
        QDBusPendingCallWatcher *maxScreenWatcher = new QDBusPendingCallWatcher(maxScreenReply, this);
        QObject::connect(maxScreenWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<int> reply = *watcher;
            if (!reply.isError()) {
                maximumScreenBrightnessChanged(reply.value());
            }
            watcher->deleteLater();
        });

        QDBusMessage keyboardMsg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                                QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                                QStringLiteral("keyboardBrightnessValue"));
        QDBusPendingReply<int> keyboardReply = QDBusConnection::sessionBus().asyncCall(keyboardMsg);
        QDBusPendingCallWatcher *keyboardWatcher = new QDBusPendingCallWatcher(keyboardReply, this);
        QObject::connect(keyboardWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<int> reply = *watcher;
            if (!reply.isError()) {
                keyboardBrightnessChanged(reply.value());
            }
            watcher->deleteLater();
        });

        QDBusMessage maxKeyboardMsg = QDBusMessage::createMethodCall(SOLID_POWERMANAGEMENT_SERVICE,
                                                                     QStringLiteral("/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl"),
                                                                     QStringLiteral("org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl"),
                                                                     QStringLiteral("keyboardBrightnessValueMax"));
        QDBusPendingReply<int> maxKeyboardReply = QDBusConnection::sessionBus().asyncCall(maxKeyboardMsg);
        QDBusPendingCallWatcher *maxKeyboardWatcher = new QDBusPendingCallWatcher(maxKeyboardReply, this);
        QObject::connect(maxKeyboardWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<int> reply = *watcher;
            if (!reply.isError()) {
                maximumKeyboardBrightnessChanged(reply.value());
            }
            watcher->deleteLater();
        });

    //any info concerning lock screen/screensaver goes here
    } else if (name == "UserActivity") {
        setData("UserActivity", "IdleTime", KIdleTime::instance()->idleTime());
    } else {
        qDebug() << "Data for '" << name << "' not found";
        return false;
    }
    return true;
}

QString PowermanagementEngine::batteryType(const Solid::Battery* battery) const
{
  switch(battery->type()) {
      case Solid::Battery::PrimaryBattery:
          return QStringLiteral("Battery");
          break;
      case Solid::Battery::UpsBattery:
          return QStringLiteral("Ups");
          break;
      case Solid::Battery::MonitorBattery:
          return QStringLiteral("Monitor");
          break;
      case Solid::Battery::MouseBattery:
          return QStringLiteral("Mouse");
          break;
      case Solid::Battery::KeyboardBattery:
          return QStringLiteral("Keyboard");
          break;
      case Solid::Battery::PdaBattery:
          return QStringLiteral("Pda");
          break;
      case Solid::Battery::PhoneBattery:
          return QStringLiteral("Phone");
          break;
      default:
          return QStringLiteral("Unknown");
  }

  return QStringLiteral("Unknown");
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
    } else if (newState == Solid::Battery::FullyCharged) {
        state = "FullyCharged";
    }

    const QString source = m_batterySources[udi];
    setData(source, "State", state);
}

void PowermanagementEngine::updateBatteryPresentState(bool newState, const QString& udi)
{
    const QString source = m_batterySources[udi];
    setData(source, "Plugged in", newState); // FIXME This needs to be renamed and Battery Monitor adjusted
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

void PowermanagementEngine::updateAcPlugState(bool onBattery)
{
    setData("AC Adapter", "Plugged in", !onBattery);
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
            while (sourceNames.contains(QStringLiteral("Battery%1").arg(index))) {
                index++;
            }

            const QString source = QStringLiteral("Battery%1").arg(index);
            sourceNames << source;
            m_batterySources[device.udi()] = source;

            connect(battery, SIGNAL(chargeStateChanged(int,QString)), this,
                    SLOT(updateBatteryChargeState(int,QString)));
            connect(battery, SIGNAL(chargePercentChanged(int,QString)), this,
                    SLOT(updateBatteryChargePercent(int,QString)));
            connect(battery, SIGNAL(presentStateChanged(bool,QString)), this,
                    SLOT(updateBatteryPresentState(bool,QString)));
            connect(battery, SIGNAL(powerSupplyStateChanged(bool,QString)), this,
                    SLOT(updateBatteryPowerSupplyState(bool,QString)));

            // Set initial values
            updateBatteryChargeState(battery->chargeState(), device.udi());
            updateBatteryChargePercent(battery->chargePercent(), device.udi());
            updateBatteryPresentState(battery->isPresent(), device.udi());
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

void PowermanagementEngine::screenBrightnessChanged(int brightness)
{
    setData("PowerDevil", "Screen Brightness", brightness);
}

void PowermanagementEngine::maximumScreenBrightnessChanged(int maximumBrightness)
{
    setData("PowerDevil", "Maximum Screen Brightness", maximumBrightness);
    setData("PowerDevil", "Screen Brightness Available", maximumBrightness > 0);
}

void PowermanagementEngine::keyboardBrightnessChanged(int brightness)
{
    setData("PowerDevil", "Keyboard Brightness", brightness);
}

void PowermanagementEngine::maximumKeyboardBrightnessChanged(int maximumBrightness)
{
    setData("PowerDevil", "Maximum Keyboard Brightness", maximumBrightness);
    setData("PowerDevil", "Keyboard Brightness Available", maximumBrightness > 0);
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(powermanagement, PowermanagementEngine, "plasma-dataengine-powermanagement.json")

#include "powermanagementengine.moc"
