/*
 * SPDX-FileCopyrightText: 2026 Ramil Nurmanov <ramil2004nur@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusVirtualObject>
#include <QMutex>
#include <QSet>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

#include <algorithm>
#include <chrono>
#include <tuple>
#include <utility>

#include "batterycontrol.h"
#include "upowertypes_p.h"

using namespace Qt::StringLiterals;

namespace
{
constexpr QLatin1StringView UPOWER_SERVICE("org.freedesktop.UPower");
constexpr QLatin1StringView UPOWER_PATH("/org/freedesktop/UPower");
constexpr QLatin1StringView UPOWER_DEVICE_IFACE("org.freedesktop.UPower.Device");
constexpr QLatin1StringView PROPERTIES_IFACE("org.freedesktop.DBus.Properties");
constexpr QLatin1StringView MOCK_CONNECTION("batterycontroltest-mock");
constexpr QLatin1StringView LOGIN1_CONNECTION("batterycontroltest-login1");
constexpr QLatin1StringView LOGIN1_SERVICE("org.freedesktop.login1");

constexpr int SETTLE_TIME_MSEC = 100;

QString devicePath(const QString &name)
{
    return u"/org/freedesktop/UPower/devices/"_s + name;
}

QVariantMap battery(UpDeviceKind kind = UP_DEVICE_KIND_BATTERY)
{
    return {
        {u"Type"_s, kind},
        {u"NativePath"_s, u"BAT"_s},
        {u"Vendor"_s, QString()},
        {u"Model"_s, QString()},
        {u"Technology"_s, UP_DEVICE_TECHNOLOGY_UNKNOWN},
        {u"IsPresent"_s, true},
        {u"PowerSupply"_s, true},
        {u"Percentage"_s, 50.0},
        {u"BatteryLevel"_s, UP_DEVICE_LEVEL_NONE},
        {u"Capacity"_s, 90.0},
        {u"Energy"_s, 25.0},
        {u"EnergyFull"_s, 50.0},
        {u"State"_s, UP_DEVICE_STATE_DISCHARGING},
    };
}

class MockUPower : public QDBusVirtualObject
{
public:
    QString introspect(const QString &) const override
    {
        return {};
    }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        QDBusConnection bus(connection);
        const QMutexLocker locker(&m_mutex);

        if (message.path() == UPOWER_PATH) {
            if (message.member() == "EnumerateDevices"_L1) {
                QList<QDBusObjectPath> paths;
                for (const QString &path : std::as_const(m_registrationOrder)) {
                    paths.append(QDBusObjectPath(path));
                }
                ++m_enumerations;
                bus.send(message.createReply(QVariant::fromValue(paths)));
                return true;
            }
            return false;
        }

        if (message.interface() != PROPERTIES_IFACE || message.member() != "GetAll"_L1) {
            return false;
        }

        ++m_getAllCalls;
        const auto it = m_devices.constFind(message.path());
        if (m_failGetAll || it == m_devices.constEnd()) {
            bus.send(message.createErrorReply(QDBusError::Failed, message.path()));
            return true;
        }

        const QDBusMessage reply = message.createReply(QVariant::fromValue(*it));
        if (m_holdGetAll || m_holdPaths.contains(message.path())) {
            m_heldReplies.append({message.path(), reply});
            return true;
        }
        bus.send(reply);

        if (const QVariantMap changes = m_changesAfterGetAll.take(message.path()); !changes.isEmpty()) {
            m_devices[message.path()].insert(changes);
            bus.send(propertiesChanged(message.path(), changes, {}));
        }
        return true;
    }

    void addDevice(const QString &path, const QVariantMap &properties, bool notify)
    {
        {
            const QMutexLocker locker(&m_mutex);
            m_devices.insert(path, properties);
            if (!m_registrationOrder.contains(path)) {
                m_registrationOrder.append(path);
            }
        }
        if (notify) {
            sendManagerSignal(u"DeviceAdded"_s, path);
        }
    }

    void removeDevice(const QString &path, bool notify = true)
    {
        {
            const QMutexLocker locker(&m_mutex);
            m_devices.remove(path);
            m_registrationOrder.removeOne(path);
        }
        if (notify) {
            sendManagerSignal(u"DeviceRemoved"_s, path);
        }
    }

    void changeProperties(const QString &path, const QVariantMap &changes, const QStringList &invalidated = {})
    {
        changeSilently(path, changes);
        QDBusConnection(MOCK_CONNECTION).send(propertiesChanged(path, invalidated.isEmpty() ? changes : QVariantMap(), invalidated));
    }

    void changeSilently(const QString &path, const QVariantMap &changes)
    {
        const QMutexLocker locker(&m_mutex);
        m_devices[path].insert(changes);
    }

    void changeAfterNextGetAll(const QString &path, const QVariantMap &changes)
    {
        const QMutexLocker locker(&m_mutex);
        m_changesAfterGetAll.insert(path, changes);
    }

    void setFailGetAll(bool fail)
    {
        const QMutexLocker locker(&m_mutex);
        m_failGetAll = fail;
    }

    void setHoldGetAll(bool hold)
    {
        const QMutexLocker locker(&m_mutex);
        m_holdGetAll = hold;
    }

    void holdGetAllFor(const QString &path)
    {
        const QMutexLocker locker(&m_mutex);
        m_holdPaths.insert(path);
    }

    int heldReplies()
    {
        const QMutexLocker locker(&m_mutex);
        return m_heldReplies.size();
    }

    void releaseHeldReply(const QString &path)
    {
        const QMutexLocker locker(&m_mutex);
        const auto it = std::ranges::find(m_heldReplies, path, &std::pair<QString, QDBusMessage>::first);
        QVERIFY(it != m_heldReplies.end());
        QDBusConnection(MOCK_CONNECTION).send(it->second);
        m_heldReplies.erase(it);
    }

    void releaseHeldRepliesInReverse()
    {
        const QMutexLocker locker(&m_mutex);
        std::ranges::reverse(m_heldReplies);
        for (const auto &[path, reply] : std::as_const(m_heldReplies)) {
            QDBusConnection(MOCK_CONNECTION).send(reply);
        }
        m_heldReplies.clear();
        m_holdPaths.clear();
        m_holdGetAll = false;
    }

    int enumerations()
    {
        const QMutexLocker locker(&m_mutex);
        return m_enumerations;
    }

    int getAllCalls()
    {
        const QMutexLocker locker(&m_mutex);
        return m_getAllCalls;
    }

    void resume()
    {
        QDBusMessage signal = QDBusMessage::createSignal(u"/org/freedesktop/login1"_s, u"org.freedesktop.login1.Manager"_s, u"PrepareForSleep"_s);
        signal.setArguments({false});
        QDBusConnection(LOGIN1_CONNECTION).send(signal);
    }

private:
    static QDBusMessage propertiesChanged(const QString &path, const QVariantMap &changes, const QStringList &invalidated)
    {
        QDBusMessage signal = QDBusMessage::createSignal(path, PROPERTIES_IFACE, u"PropertiesChanged"_s);
        signal.setArguments({QString(UPOWER_DEVICE_IFACE), QVariant::fromValue(changes), invalidated});
        return signal;
    }

    void sendManagerSignal(const QString &name, const QString &path)
    {
        QDBusMessage signal = QDBusMessage::createSignal(UPOWER_PATH, UPOWER_SERVICE, name);
        signal.setArguments({QVariant::fromValue(QDBusObjectPath(path))});
        QDBusConnection(MOCK_CONNECTION).send(signal);
    }

    QMutex m_mutex;
    QMap<QString, QVariantMap> m_devices;
    QStringList m_registrationOrder;
    QMap<QString, QVariantMap> m_changesAfterGetAll;
    QSet<QString> m_holdPaths;
    QList<std::pair<QString, QDBusMessage>> m_heldReplies;
    bool m_failGetAll = false;
    bool m_holdGetAll = false;
    int m_enumerations = 0;
    int m_getAllCalls = 0;
};
}

class BatteryControlTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testNoBatteries();
    void testDeviceFiltering();
    void testStartupKeepsEnumerationOrder();
    void testSlowDeviceDoesNotBlockOthers();
    void testAdding();
    void testReplacedDeviceKeepsConnectionOrder();
    void testRemoving();
    void testRemovingWhileLoading();
    void testRoles();
    void testNames();
    void testPropertyChanges();
    void testChangeRightAfterInitialRead();
    void testChangeDuringInitialRead();
    void testChangeDuringReload();
    void testInvalidatedProperties();
    void testResume();
    void testStaleReadIsIgnored();
    void testFailedReadKeepsCache();
    void testFailedInitialReadIsRetried();
    void testFailedInitialReadIsRetriedOnResume();
    void testOverallBattery();
    void testServiceRestart();
    void testServiceRestartWhileLoading();

private:
    QVariant roleValue(const BatteryControlModel &model, int row, BatteryControlModel::BatteryRoles role) const
    {
        return model.data(model.index(row), role);
    }

    QThread m_thread;
    MockUPower *m_mock = nullptr;
};

void BatteryControlTest::initTestCase()
{
    qDBusRegisterMetaType<QList<QDBusObjectPath>>();

    const QString address = QString::fromLocal8Bit(qgetenv("DBUS_SESSION_BUS_ADDRESS"));
    QDBusConnection login1 = QDBusConnection::connectToBus(address, LOGIN1_CONNECTION);
    QVERIFY(login1.registerService(LOGIN1_SERVICE));
    QVERIFY(QDBusConnection::connectToBus(address, MOCK_CONNECTION).isConnected());
    m_thread.start();
}

void BatteryControlTest::cleanupTestCase()
{
    m_thread.quit();
    QVERIFY(m_thread.wait());
}

void BatteryControlTest::init()
{
    m_mock = new MockUPower;
    m_mock->moveToThread(&m_thread);

    QDBusConnection bus(MOCK_CONNECTION);
    QVERIFY(bus.registerVirtualObject(UPOWER_PATH, m_mock, QDBusConnection::SubPath));
    QVERIFY(bus.registerService(UPOWER_SERVICE));
}

void BatteryControlTest::cleanup()
{
    QDBusConnection bus(MOCK_CONNECTION);
    bus.unregisterObject(UPOWER_PATH, QDBusConnection::UnregisterTree);
    QVERIFY(bus.unregisterService(UPOWER_SERVICE));
    QMetaObject::invokeMethod(
        m_mock,
        [mock = m_mock] {
            delete mock;
        },
        Qt::BlockingQueuedConnection);
    m_mock = nullptr;
}

void BatteryControlTest::testNoBatteries()
{
    BatteryControlModel model;

    QTRY_COMPARE(m_mock->enumerations(), 1);
    QTest::qWait(SETTLE_TIME_MSEC);

    QCOMPARE(model.rowCount({}), 0);
    QCOMPARE(model.property("hasBatteries").toBool(), false);
    QCOMPARE(model.property("hasCumulative").toBool(), false);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), false);
}

void BatteryControlTest::testDeviceFiltering()
{
    QVariantMap bluezDevice = battery(UP_DEVICE_KIND_BLUETOOTH_GENERIC);
    bluezDevice[u"NativePath"_s] = u"/org/bluez/hci0/dev_00"_s;

    m_mock->addDevice(devicePath(u"line_power"_s), battery(UP_DEVICE_KIND_LINE_POWER), false);
    m_mock->addDevice(devicePath(u"unknown"_s), battery(UP_DEVICE_KIND_UNKNOWN), false);
    m_mock->addDevice(devicePath(u"bluez"_s), bluezDevice, false);
    m_mock->addDevice(devicePath(u"speakers"_s), battery(UP_DEVICE_KIND_SPEAKERS), false);

    BatteryControlModel model;

    QTRY_COMPARE(model.rowCount({}), 2);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Type).toString(), u"Bluetooth"_s);
    QCOMPARE(roleValue(model, 1, BatteryControlModel::Type).toString(), u"Speakers"_s);
    QCOMPARE(model.property("hasBatteries").toBool(), true);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), false);
    QCOMPARE(model.property("pluggedIn").toBool(), true);
}

void BatteryControlTest::testStartupKeepsEnumerationOrder()
{
    QVariantMap bat1 = battery();
    bat1[u"Percentage"_s] = 11.0;
    QVariantMap bat0 = battery();
    bat0[u"Percentage"_s] = 10.0;

    m_mock->addDevice(devicePath(u"mouse"_s), battery(UP_DEVICE_KIND_MOUSE), false);
    m_mock->addDevice(devicePath(u"battery_BAT1"_s), bat1, false);
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), bat0, false);
    m_mock->setHoldGetAll(true);

    BatteryControlModel model;

    QCOMPARE(model.rowCount({}), 0);
    QCOMPARE(model.property("hasBatteries").toBool(), false);

    QTRY_COMPARE(m_mock->heldReplies(), 3);
    QCOMPARE(model.rowCount({}), 0);

    m_mock->releaseHeldReply(devicePath(u"battery_BAT1"_s));
    QTRY_COMPARE(model.rowCount({}), 1);
    m_mock->releaseHeldReply(devicePath(u"mouse"_s));
    QTRY_COMPARE(model.rowCount({}), 2);
    m_mock->releaseHeldReply(devicePath(u"battery_BAT0"_s));
    QTRY_COMPARE(model.rowCount({}), 3);
    m_mock->setHoldGetAll(false);

    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 11);
    QCOMPARE(roleValue(model, 1, BatteryControlModel::Percent).toInt(), 10);
    QCOMPARE(roleValue(model, 2, BatteryControlModel::Type).toString(), u"Mouse"_s);
    QCOMPARE(model.property("hasBatteries").toBool(), true);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), true);
}

void BatteryControlTest::testSlowDeviceDoesNotBlockOthers()
{
    QVariantMap bluezDevice = battery(UP_DEVICE_KIND_BLUETOOTH_GENERIC);
    bluezDevice[u"NativePath"_s] = u"/org/bluez/hci0/dev_00"_s;

    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);
    m_mock->addDevice(devicePath(u"bluez"_s), bluezDevice, false);
    m_mock->holdGetAllFor(devicePath(u"bluez"_s));

    BatteryControlModel model;

    QTRY_COMPARE(model.rowCount({}), 1);
    QTRY_COMPARE(m_mock->heldReplies(), 1);
    QCOMPARE(model.rowCount({}), 1);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Type).toString(), u"Battery"_s);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), true);
    QCOMPARE(model.property("hasCumulative").toBool(), true);
    QCOMPARE(model.property("percent").toInt(), 50);

    m_mock->releaseHeldReply(devicePath(u"bluez"_s));
    QTRY_COMPARE(model.rowCount({}), 2);
    QCOMPARE(roleValue(model, 1, BatteryControlModel::Type).toString(), u"Bluetooth"_s);
}

void BatteryControlTest::testAdding()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);
    m_mock->addDevice(devicePath(u"mouse"_s), battery(UP_DEVICE_KIND_MOUSE), false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 2);

    QSignalSpy rowsInserted(&model, &QAbstractItemModel::rowsInserted);

    m_mock->addDevice(devicePath(u"ups"_s), battery(UP_DEVICE_KIND_UPS), true);
    QVERIFY(rowsInserted.wait());
    QCOMPARE(rowsInserted.takeFirst().at(1).toInt(), 2);
    QCOMPARE(roleValue(model, 2, BatteryControlModel::Type).toString(), u"Ups"_s);

    QVariantMap bat1 = battery();
    bat1[u"Percentage"_s] = 12.0;
    m_mock->addDevice(devicePath(u"battery_BAT1"_s), bat1, true);
    QVERIFY(rowsInserted.wait());
    QCOMPARE(rowsInserted.takeFirst().at(1).toInt(), 1);
    QCOMPARE(roleValue(model, 1, BatteryControlModel::Percent).toInt(), 12);

    m_mock->addDevice(devicePath(u"ups"_s), battery(UP_DEVICE_KIND_UPS), true);
    m_mock->addDevice(devicePath(u"line_power"_s), battery(UP_DEVICE_KIND_LINE_POWER), true);
    QVERIFY(!rowsInserted.wait(SETTLE_TIME_MSEC));
    QCOMPARE(model.rowCount({}), 4);
}

void BatteryControlTest::testReplacedDeviceKeepsConnectionOrder()
{
    QVariantMap headset = battery(UP_DEVICE_KIND_HEADSET);
    headset[u"Model"_s] = u"Earbuds"_s;
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);
    m_mock->addDevice(devicePath(u"headset_dev_00"_s), headset, false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 2);

    QVariantMap bluezKeyboard = battery(UP_DEVICE_KIND_KEYBOARD);
    bluezKeyboard[u"Model"_s] = u"Keyboard"_s;
    m_mock->addDevice(devicePath(u"keyboard_dev_01"_s), bluezKeyboard, true);
    QTRY_COMPARE(model.rowCount({}), 3);
    QCOMPARE(roleValue(model, 2, BatteryControlModel::PrettyName).toString(), u"Keyboard"_s);

    // UPower hides the Bluetooth device once the kernel exposes the same battery under a new object path
    m_mock->removeDevice(devicePath(u"keyboard_dev_01"_s));
    QTRY_COMPARE(model.rowCount({}), 2);

    QVariantMap kernelKeyboard = battery(UP_DEVICE_KIND_KEYBOARD);
    kernelKeyboard[u"Model"_s] = u"Keyboard"_s;
    m_mock->addDevice(devicePath(u"battery_hidpp_battery_0"_s), kernelKeyboard, true);
    QTRY_COMPARE(model.rowCount({}), 3);

    QCOMPARE(roleValue(model, 0, BatteryControlModel::Type).toString(), u"Battery"_s);
    QCOMPARE(roleValue(model, 1, BatteryControlModel::PrettyName).toString(), u"Earbuds"_s);
    QCOMPARE(roleValue(model, 2, BatteryControlModel::PrettyName).toString(), u"Keyboard"_s);
}

void BatteryControlTest::testRemoving()
{
    QVariantMap bat0 = battery();
    bat0[u"Percentage"_s] = 25.0;
    bat0[u"Energy"_s] = 10.0;
    bat0[u"EnergyFull"_s] = 40.0;
    QVariantMap bat1 = battery();
    bat1[u"Percentage"_s] = 75.0;
    bat1[u"Energy"_s] = 30.0;
    bat1[u"EnergyFull"_s] = 40.0;

    m_mock->addDevice(devicePath(u"battery_BAT0"_s), bat0, false);
    m_mock->addDevice(devicePath(u"battery_BAT1"_s), bat1, false);
    m_mock->addDevice(devicePath(u"mouse"_s), battery(UP_DEVICE_KIND_MOUSE), false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 3);
    QCOMPARE(model.property("percent").toInt(), 50);

    QSignalSpy rowsRemoved(&model, &QAbstractItemModel::rowsRemoved);

    m_mock->removeDevice(devicePath(u"battery_BAT1"_s));
    QVERIFY(rowsRemoved.wait());
    QCOMPARE(model.rowCount({}), 2);
    QCOMPARE(model.property("percent").toInt(), 25);
    QCOMPARE(model.property("hasCumulative").toBool(), true);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), true);
    QCOMPARE(model.property("state").value<BatteryControlModel::ChargeStateEnum>(), BatteryControlModel::Discharging);

    m_mock->removeDevice(devicePath(u"battery_BAT0"_s));
    QVERIFY(rowsRemoved.wait());
    QCOMPARE(model.rowCount({}), 1);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Type).toString(), u"Mouse"_s);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), false);
    QCOMPARE(model.property("hasBatteries").toBool(), true);
    QCOMPARE(model.property("hasCumulative").toBool(), false);
    QCOMPARE(model.property("percent").toInt(), 0);
    QCOMPARE(model.property("pluggedIn").toBool(), true);
    QCOMPARE(model.property("state").value<BatteryControlModel::ChargeStateEnum>(), BatteryControlModel::NoCharge);

    m_mock->removeDevice(devicePath(u"mouse"_s));
    QVERIFY(rowsRemoved.wait());
    QCOMPARE(model.rowCount({}), 0);
    QCOMPARE(model.property("hasBatteries").toBool(), false);
}

void BatteryControlTest::testRemovingWhileLoading()
{
    BatteryControlModel model;
    QTRY_COMPARE(m_mock->enumerations(), 1);
    QTest::qWait(SETTLE_TIME_MSEC);

    m_mock->setHoldGetAll(true);
    m_mock->addDevice(devicePath(u"mouse"_s), battery(UP_DEVICE_KIND_MOUSE), true);
    QTRY_COMPARE(m_mock->heldReplies(), 1);

    m_mock->removeDevice(devicePath(u"mouse"_s));
    QTest::qWait(SETTLE_TIME_MSEC);

    m_mock->releaseHeldRepliesInReverse();
    QTest::qWait(SETTLE_TIME_MSEC);

    QCOMPARE(model.rowCount({}), 0);
    QCOMPARE(model.property("hasBatteries").toBool(), false);
}

void BatteryControlTest::testRoles()
{
    QVariantMap device = battery();
    device[u"Percentage"_s] = 49.6;
    device[u"Capacity"_s] = 87.5;
    device[u"PowerSupply"_s] = false;
    m_mock->addDevice(devicePath(u"a"_s), device, false);

    QVariantMap unknownLevel = battery();
    unknownLevel[u"BatteryLevel"_s] = UP_DEVICE_LEVEL_UNKNOWN;
    m_mock->addDevice(devicePath(u"b"_s), unknownLevel, false);

    QVariantMap noLevel = battery();
    noLevel.remove(u"BatteryLevel"_s);
    noLevel[u"Percentage"_s] = 30.0;
    noLevel[u"State"_s] = UP_DEVICE_STATE_EMPTY;
    m_mock->addDevice(devicePath(u"c"_s), noLevel, false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 3);

    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 50);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Capacity).toInt(), 88);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Energy).toDouble(), 25.0);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::PluggedIn).toBool(), true);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::IsPowerSupply).toBool(), false);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::ChargeState).value<BatteryControlModel::ChargeStateEnum>(), BatteryControlModel::Discharging);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Type).toString(), u"Battery"_s);

    QCOMPARE(roleValue(model, 1, BatteryControlModel::Percent).toInt(), -1);

    QCOMPARE(roleValue(model, 2, BatteryControlModel::Percent).toInt(), 30);
    QCOMPARE(roleValue(model, 2, BatteryControlModel::ChargeState).value<BatteryControlModel::ChargeStateEnum>(), BatteryControlModel::NoCharge);
}

void BatteryControlTest::testNames()
{
    QVariantMap primary = battery();
    primary[u"Model"_s] = u"SERIAL123"_s;
    m_mock->addDevice(devicePath(u"a_primary"_s), primary, false);

    QVariantMap mouse = battery(UP_DEVICE_KIND_MOUSE);
    mouse[u"Vendor"_s] = u"Acme"_s;
    mouse[u"Model"_s] = u"Pointer"_s;
    m_mock->addDevice(devicePath(u"b_mouse"_s), mouse, false);

    QVariantMap technology = battery(UP_DEVICE_KIND_MOUSE);
    technology[u"Technology"_s] = UP_DEVICE_TECHNOLOGY_LITHIUM_ION;
    m_mock->addDevice(devicePath(u"c_technology"_s), technology, false);

    QVariantMap vendorOnly = battery(UP_DEVICE_KIND_MOUSE);
    vendorOnly[u"Vendor"_s] = u"Acme"_s;
    m_mock->addDevice(devicePath(u"d_vendor"_s), vendorOnly, false);

    QVariantMap unknownBattery = battery(UP_DEVICE_KIND_MOUSE);
    unknownBattery[u"Model"_s] = u"Unknown Battery"_s;
    m_mock->addDevice(devicePath(u"e_unknown"_s), unknownBattery, false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 5);

    QCOMPARE(roleValue(model, 0, BatteryControlModel::PrettyName).toString(), u"Battery"_s);
    QCOMPARE(roleValue(model, 1, BatteryControlModel::PrettyName).toString(), u"Acme Pointer"_s);
    QCOMPARE(roleValue(model, 2, BatteryControlModel::PrettyName).toString(), u"Lithium Ion Battery"_s);
    QCOMPARE(roleValue(model, 3, BatteryControlModel::PrettyName).toString(), u"Acme Acme"_s);
    QCOMPARE(roleValue(model, 4, BatteryControlModel::PrettyName).toString(), u"Battery 2"_s);
}

void BatteryControlTest::testPropertyChanges()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 1);

    QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);

    const QList<std::tuple<QVariantMap, int>> changes = {
        {{{u"Percentage"_s, 60.0}}, BatteryControlModel::Percent},
        {{{u"Capacity"_s, 80.0}}, BatteryControlModel::Capacity},
        {{{u"Energy"_s, 30.0}}, BatteryControlModel::Energy},
        {{{u"IsPresent"_s, false}}, BatteryControlModel::PluggedIn},
        {{{u"PowerSupply"_s, false}}, BatteryControlModel::IsPowerSupply},
        {{{u"State"_s, UP_DEVICE_STATE_CHARGING}}, BatteryControlModel::ChargeState},
    };

    for (const auto &[properties, role] : changes) {
        m_mock->changeProperties(devicePath(u"battery_BAT0"_s), properties);
        QVERIFY(dataChanged.wait());
        QCOMPARE(dataChanged.takeFirst().at(2).value<QList<int>>(), QList<int>{role});
        QVERIFY(dataChanged.isEmpty());
    }

    m_mock->changeProperties(devicePath(u"battery_BAT0"_s), {{u"Temperature"_s, 30.0}, {u"Percentage"_s, 60.0}});
    QVERIFY(!dataChanged.wait(SETTLE_TIME_MSEC));
}

void BatteryControlTest::testChangeRightAfterInitialRead()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);
    m_mock->changeAfterNextGetAll(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 77.0}});

    BatteryControlModel model;

    QTRY_COMPARE(model.rowCount({}), 1);
    QTRY_COMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 77);
    QCOMPARE(model.property("percent").toInt(), 77);
}

void BatteryControlTest::testChangeDuringInitialRead()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);
    m_mock->setHoldGetAll(true);

    BatteryControlModel model;
    QTRY_COMPARE(m_mock->heldReplies(), 1);

    m_mock->changeProperties(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 77.0}});
    QTest::qWait(SETTLE_TIME_MSEC);
    QCOMPARE(model.rowCount({}), 0);

    m_mock->releaseHeldRepliesInReverse();

    QTRY_COMPARE(model.rowCount({}), 1);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 77);
    QCOMPARE(model.property("percent").toInt(), 77);
}

void BatteryControlTest::testChangeDuringReload()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 1);

    m_mock->changeSilently(devicePath(u"battery_BAT0"_s), {{u"Energy"_s, 35.0}});
    m_mock->setHoldGetAll(true);
    m_mock->resume();
    QTRY_COMPARE(m_mock->heldReplies(), 1);

    m_mock->changeProperties(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 60.0}});
    QTRY_COMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 60);

    m_mock->releaseHeldRepliesInReverse();

    QTRY_COMPARE(roleValue(model, 0, BatteryControlModel::Energy).toDouble(), 35.0);
    QTest::qWait(SETTLE_TIME_MSEC);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 60);
    QCOMPARE(model.property("percent").toInt(), 60);
}

void BatteryControlTest::testInvalidatedProperties()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 1);

    QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);

    m_mock->changeProperties(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 70.0}}, {u"Percentage"_s});
    QVERIFY(dataChanged.wait());
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 70);
}

void BatteryControlTest::testResume()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 1);

    QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);

    m_mock->changeSilently(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 20.0}});
    m_mock->resume();

    QVERIFY(dataChanged.wait());
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 20);
    QCOMPARE(model.property("percent").toInt(), 20);
}

void BatteryControlTest::testStaleReadIsIgnored()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 1);

    m_mock->setHoldGetAll(true);

    m_mock->changeSilently(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 10.0}});
    m_mock->resume();
    QTRY_COMPARE(m_mock->heldReplies(), 1);

    m_mock->changeSilently(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 20.0}});
    m_mock->resume();
    QTRY_COMPARE(m_mock->heldReplies(), 2);

    m_mock->releaseHeldRepliesInReverse();

    QTRY_COMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 20);
    QTest::qWait(SETTLE_TIME_MSEC);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 20);
}

void BatteryControlTest::testFailedReadKeepsCache()
{
    QVariantMap device = battery();
    device[u"Percentage"_s] = 60.0;
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), device, false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 1);

    QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);
    m_mock->setFailGetAll(true);

    int calls = m_mock->getAllCalls();
    m_mock->resume();
    QTRY_COMPARE(m_mock->getAllCalls(), calls + 1);
    QTest::qWait(SETTLE_TIME_MSEC);

    calls = m_mock->getAllCalls();
    m_mock->changeProperties(devicePath(u"battery_BAT0"_s), {}, {u"Percentage"_s});
    QTRY_COMPARE(m_mock->getAllCalls(), calls + 1);
    QTest::qWait(SETTLE_TIME_MSEC);

    QVERIFY(dataChanged.isEmpty());
    QCOMPARE(model.rowCount({}), 1);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 60);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::PluggedIn).toBool(), true);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::IsPowerSupply).toBool(), true);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Type).toString(), u"Battery"_s);
    QCOMPARE(model.property("hasCumulative").toBool(), true);
    QCOMPARE(model.property("percent").toInt(), 60);

    m_mock->setFailGetAll(false);
    m_mock->changeSilently(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 40.0}});
    m_mock->resume();
    QVERIFY(dataChanged.wait());
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 40);
}

void BatteryControlTest::testFailedInitialReadIsRetried()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);
    m_mock->addDevice(devicePath(u"mouse"_s), battery(UP_DEVICE_KIND_MOUSE), false);
    m_mock->setFailGetAll(true);

    BatteryControlModel model;
    QTRY_COMPARE(m_mock->getAllCalls(), 2);
    QTest::qWait(SETTLE_TIME_MSEC);

    QCOMPARE(model.rowCount({}), 0);
    QCOMPARE(model.property("hasBatteries").toBool(), false);

    m_mock->setFailGetAll(false);

    QTRY_COMPARE(model.rowCount({}), 2);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Type).toString(), u"Battery"_s);
    QCOMPARE(roleValue(model, 1, BatteryControlModel::Type).toString(), u"Mouse"_s);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), true);
    QCOMPARE(model.property("percent").toInt(), 50);
}

void BatteryControlTest::testFailedInitialReadIsRetriedOnResume()
{
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), battery(), false);
    m_mock->setFailGetAll(true);

    BatteryControlModel model;
    QTRY_COMPARE(m_mock->getAllCalls(), 1);
    QTest::qWait(SETTLE_TIME_MSEC);
    QCOMPARE(model.rowCount({}), 0);

    m_mock->setFailGetAll(false);
    m_mock->resume();

    QTRY_COMPARE_WITH_TIMEOUT(model.rowCount({}), 1, std::chrono::milliseconds(400));
    QCOMPARE(model.property("hasInternalBatteries").toBool(), true);
}

void BatteryControlTest::testOverallBattery()
{
    QVariantMap bat0 = battery();
    bat0[u"Energy"_s] = 10.0;
    bat0[u"EnergyFull"_s] = 40.0;
    bat0[u"Percentage"_s] = 25.0;
    bat0[u"State"_s] = UP_DEVICE_STATE_FULLY_CHARGED;
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), bat0, false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 1);

    QCOMPARE(model.property("hasCumulative").toBool(), true);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), true);
    QCOMPARE(model.property("percent").toInt(), 25);
    QCOMPARE(model.property("pluggedIn").toBool(), true);
    QCOMPARE(model.property("state").value<BatteryControlModel::ChargeStateEnum>(), BatteryControlModel::FullyCharged);

    QSignalSpy rowsInserted(&model, &QAbstractItemModel::rowsInserted);
    QVariantMap bat1 = battery();
    bat1[u"Energy"_s] = 30.0;
    bat1[u"EnergyFull"_s] = 40.0;
    bat1[u"Percentage"_s] = 75.0;
    bat1[u"State"_s] = UP_DEVICE_STATE_DISCHARGING;
    m_mock->addDevice(devicePath(u"battery_BAT1"_s), bat1, true);
    QVERIFY(rowsInserted.wait());

    QSignalSpy stateChanged(&model, &BatteryControlModel::stateChanged);
    m_mock->changeProperties(devicePath(u"battery_BAT1"_s), {{u"Energy"_s, 20.0}, {u"Percentage"_s, 50.0}});
    QTRY_COMPARE(model.property("percent").toInt(), 38);
    QCOMPARE(model.property("pluggedIn").toBool(), false);
    QCOMPARE(model.property("state").value<BatteryControlModel::ChargeStateEnum>(), BatteryControlModel::Discharging);

    m_mock->changeProperties(devicePath(u"battery_BAT1"_s), {{u"State"_s, UP_DEVICE_STATE_UNKNOWN}});
    QVERIFY(stateChanged.wait());
    QCOMPARE(model.property("pluggedIn").toBool(), true);
}

void BatteryControlTest::testServiceRestart()
{
    QVariantMap bat0 = battery();
    bat0[u"Percentage"_s] = 30.0;
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), bat0, false);
    m_mock->addDevice(devicePath(u"mouse"_s), battery(UP_DEVICE_KIND_MOUSE), false);

    BatteryControlModel model;
    QTRY_COMPARE(model.rowCount({}), 2);
    QCOMPARE(model.property("percent").toInt(), 30);

    QSignalSpy modelReset(&model, &QAbstractItemModel::modelReset);

    QDBusConnection bus(MOCK_CONNECTION);
    QVERIFY(bus.unregisterService(UPOWER_SERVICE));

    QTRY_COMPARE(model.rowCount({}), 0);
    QCOMPARE(modelReset.size(), 1);
    QCOMPARE(model.property("hasBatteries").toBool(), false);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), false);
    QCOMPARE(model.property("hasCumulative").toBool(), false);
    QCOMPARE(model.property("percent").toInt(), 0);
    QCOMPARE(model.property("pluggedIn").toBool(), true);
    QCOMPARE(model.property("state").value<BatteryControlModel::ChargeStateEnum>(), BatteryControlModel::NoCharge);

    m_mock->removeDevice(devicePath(u"mouse"_s), false);
    m_mock->changeSilently(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 80.0}, {u"State"_s, UP_DEVICE_STATE_CHARGING}});
    m_mock->addDevice(devicePath(u"ups"_s), battery(UP_DEVICE_KIND_UPS), false);
    QVERIFY(bus.registerService(UPOWER_SERVICE));

    QTRY_COMPARE(model.rowCount({}), 2);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 80);
    QCOMPARE(roleValue(model, 1, BatteryControlModel::Type).toString(), u"Ups"_s);
    QCOMPARE(model.property("hasBatteries").toBool(), true);
    QCOMPARE(model.property("hasInternalBatteries").toBool(), true);
    QCOMPARE(model.property("hasCumulative").toBool(), true);
    QCOMPARE(model.property("percent").toInt(), 80);
    QCOMPARE(model.property("state").value<BatteryControlModel::ChargeStateEnum>(), BatteryControlModel::Charging);

    m_mock->changeProperties(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 81.0}});
    QTRY_COMPARE(model.property("percent").toInt(), 81);
}

void BatteryControlTest::testServiceRestartWhileLoading()
{
    QVariantMap bat0 = battery();
    bat0[u"Percentage"_s] = 30.0;
    m_mock->addDevice(devicePath(u"battery_BAT0"_s), bat0, false);
    m_mock->setHoldGetAll(true);

    BatteryControlModel model;
    QTRY_COMPARE(m_mock->heldReplies(), 1);

    QDBusConnection bus(MOCK_CONNECTION);
    QVERIFY(bus.unregisterService(UPOWER_SERVICE));
    QTest::qWait(SETTLE_TIME_MSEC);

    m_mock->releaseHeldRepliesInReverse();
    QTest::qWait(SETTLE_TIME_MSEC);
    QCOMPARE(model.rowCount({}), 0);

    m_mock->changeSilently(devicePath(u"battery_BAT0"_s), {{u"Percentage"_s, 90.0}});
    QVERIFY(bus.registerService(UPOWER_SERVICE));

    QTRY_COMPARE(model.rowCount({}), 1);
    QCOMPARE(roleValue(model, 0, BatteryControlModel::Percent).toInt(), 90);
    QCOMPARE(model.property("percent").toInt(), 90);
}

int main(int argc, char *argv[])
{
    qputenv("DBUS_SYSTEM_BUS_ADDRESS", qgetenv("DBUS_SESSION_BUS_ADDRESS"));

    QCoreApplication app(argc, argv);
    BatteryControlTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "batterycontroltest.moc"
