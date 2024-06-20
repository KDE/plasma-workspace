#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import logging
import os
import subprocess
import sys
import time
import unittest
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from appium.webdriver.webdriver import ExtensionBase
from appium.webdriver.webelement import WebElement
from gi.repository import Gio, GLib
from selenium.webdriver.support.ui import WebDriverWait

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "utils"))
from GLibMainLoopThread import GLibMainLoopThread
from NetHadessPowerProfiles import NetHadessPowerProfiles
from OrgFreedesktopUPower import OrgFreedesktopUPower

WIDGET_ID: Final = "org.kde.plasma.battery"
POWERDEVIL_PATH: Final = os.environ.get("POWERDEVIL_PATH", "/usr/libexec/org_kde_powerdevil")
POWERDEVIL_SERVICE_NAME: Final = "org.kde.Solid.PowerManagement"
ENABLE_DISPLAY_DEVICE: Final = int(os.environ["ENABLE_DISPLAY_DEVICE"]) != 0


def name_has_owner(session_bus: Gio.DBusConnection, name: str) -> bool:
    """
    Whether the given name is available on session bus
    """
    message: Gio.DBusMessage = Gio.DBusMessage.new_method_call("org.freedesktop.DBus", "/", "org.freedesktop.DBus", "NameHasOwner")
    message.set_body(GLib.Variant("(s)", [name]))
    reply, _ = session_bus.send_message_with_reply_sync(message, Gio.DBusSendMessageFlags.NONE, 1000)
    return reply and reply.get_signature() == 'b' and reply.get_body().get_child_value(0).get_boolean()


class SetValueCommand(ExtensionBase):

    def method_name(self):
        return "set_value"

    def set_value(self, element: WebElement, value: str):
        """
        Set the value on this element in the application
        Args:
            value: The value to be set
        """
        data = {
            "id": element.id,
            "text": value,
        }
        return self.execute(data)["value"]

    def add_command(self):
        return "post", "/session/$sessionId/appium/element/$id/value"


class BatteryMonitorTests(unittest.TestCase):
    """
    Tests for the system tray widget
    """

    dbus_daemon_pid: str
    session_bus: Gio.DBusConnection
    driver: webdriver.Remote
    loop_thread: GLibMainLoopThread
    upower_interface: OrgFreedesktopUPower
    ppd_interface: NetHadessPowerProfiles
    powerdevil: subprocess.Popen[bytes]

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        cls.addClassCleanup(lambda: subprocess.Popen(["kill", "-15", cls.dbus_daemon_pid]).wait())
        lines: list[str] = subprocess.check_output(['dbus-daemon', '--fork', '--print-address=1', '--print-pid=1', '--session'], universal_newlines=True).strip().splitlines()
        assert len(lines) == 2, "Expected exactly 2 lines of output from dbus-daemon"
        cls.dbus_daemon_pid = lines[1]
        assert int(cls.dbus_daemon_pid) > 0, "Failed to start dbus-daemon"
        os.environ["DBUS_SYSTEM_BUS_ADDRESS"] = lines[0]
        os.environ["DBUS_SESSION_BUS_ADDRESS"] = lines[0]

        # Start the mocked upower backend
        cls.loop_thread = GLibMainLoopThread()
        cls.loop_thread.start()
        cls.upower_interface = OrgFreedesktopUPower(None, ENABLE_DISPLAY_DEVICE)
        # Wait until the mocked upower interface is online
        assert cls.upower_interface.registered_event.wait(10), "upower interface is not ready"
        cls.ppd_interface = NetHadessPowerProfiles()
        # Wait until the mocked upower interface is online
        assert cls.ppd_interface.registered_event.wait(10), "ppd interface is not ready"

        # Start PowerDevil which is used by the dataengine
        debug_env: dict[str, str] = os.environ.copy()
        debug_env["QT_LOGGING_RULES"] = "org.kde.powerdevil.debug=true"
        cls.session_bus = Gio.bus_get_sync(Gio.BusType.SESSION)
        assert not name_has_owner(cls.session_bus, POWERDEVIL_SERVICE_NAME), "PowerDevil is already running"
        cls.powerdevil = subprocess.Popen([POWERDEVIL_PATH], env=debug_env, stdout=sys.stderr, stderr=sys.stderr)
        powerdevil_started: bool = False
        for _ in range(10):
            if name_has_owner(cls.session_bus, POWERDEVIL_SERVICE_NAME):
                powerdevil_started = True
                break
            print("waiting for PowerDevil to appear on the dbus session")
            time.sleep(1)
        assert powerdevil_started, "PowerDevil is not running"

        # Now start the appium test
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("environ", {
            "DBUS_SYSTEM_BUS_ADDRESS": os.environ["DBUS_SYSTEM_BUS_ADDRESS"],
            "DBUS_SESSION_BUS_ADDRESS": os.environ["DBUS_SESSION_BUS_ADDRESS"],
            "LC_ALL": "en_US.UTF-8",
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;qt.dbus.integration.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false",
        })
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', extensions=[SetValueCommand], options=options)

    def setUp(self) -> None:
        pass

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_batterymonitor_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.quit()
        cls.powerdevil.terminate()
        cls.upower_interface.quit()
        cls.ppd_interface.quit()
        cls.loop_thread.quit()

    def test_01_batteries_are_listed(self) -> None:
        """
        Tests the widget can list all available batteries
        """
        # Don't show battery name for primary power supply batteries. They usually have cryptic serial number names.
        self.driver.find_element(by=AppiumBy.NAME, value="Battery")
        self.driver.find_element(by=AppiumBy.NAME, value="50%")
        self.driver.find_element(by=AppiumBy.NAME, value="Charging")
        self.driver.find_element(by=AppiumBy.NAME, value="1:00")  # Remaining time (h) = (40 * 0.5) / 20 = 1

        # The second battery
        self.driver.find_element(by=AppiumBy.NAME, value="Battery 2")
        self.driver.find_element(by=AppiumBy.NAME, value="100%")
        self.driver.find_element(by=AppiumBy.NAME, value="Fully Charged")

        # Wireless Mouse
        self.driver.find_element(by=AppiumBy.NAME, value="KDE Gaming Mouse")

    def test_10_ac_line_unplugged(self) -> None:
        """
        Tests the battery state changes from Charging to Discharging
        """
        self.upower_interface.set_ac_unplugged()
        self.driver.find_element(by=AppiumBy.NAME, value="Battery")
        self.driver.find_element(by=AppiumBy.NAME, value="80%")
        self.driver.find_element(by=AppiumBy.NAME, value="3:36")  # Remaining time (h) = (40 * 0.8 + 40) / 20 = 3.6
        self.driver.find_element(by=AppiumBy.NAME, value="Discharging")

        # The second battery is untouched
        self.driver.find_element(by=AppiumBy.NAME, value="Battery 2")
        self.driver.find_element(by=AppiumBy.NAME, value="100%")
        self.driver.find_element(by=AppiumBy.NAME, value="Fully Charged")

    def test_11_discharging_rate(self) -> None:
        """
        The remaining time should be updated accordingly when the discharging rate changes
        """
        self.driver.find_element(by=AppiumBy.NAME, value="3:36")
        self.upower_interface.set_energy_props(OrgFreedesktopUPower.BATTERY0_OBJECT_PATH, 40.0)
        # double weight = 0.005 * std::min<qulonglong>(60, timestamp - m_lastRateTimestamp) = 0.3;
        # double current = last * (1 - weight) + update * weight = 20 * 0.7 + 40 * 0.3 = 26;
        # Remaining time (h) = (40 * 0.8 + 40) / 26 = 2.77
        self.driver.find_element(by=AppiumBy.NAME, value="2:46")

    def test_12_estimating_discharging_rate(self) -> None:
        """
        When the discharging rate is 0, the remaining time label should show "Estimating…"
        """
        self.upower_interface.set_energy_props(OrgFreedesktopUPower.BATTERY0_OBJECT_PATH, 0)
        self.driver.find_element(by=AppiumBy.NAME, value="Estimating…")

    def test_13_hotplug_battery_when_discharging(self) -> None:
        """
        After the secondary battery is not present, the remaining time should also be updated accordingly.
        In upower, "is-present" is not necessarily bound to "power-supply", so this also tests https://invent.kde.org/plasma/powerdevil/-/merge_requests/247
        """
        self.upower_interface.set_energy_props(OrgFreedesktopUPower.BATTERY0_OBJECT_PATH, 20)
        # double weight = 0.005 * std::min<qulonglong>(60, timestamp - m_lastRateTimestamp) = 0.3;
        # double current = last * (1 - weight) + update * weight = 26 * 0.7 + 20 * 0.3 = 24.2;
        # Remaining time (h) = (40 * 0.8 + 40) / 24.2 = 2.98
        self.driver.find_element(by=AppiumBy.NAME, value="2:59")
        # Unplug
        self.upower_interface.set_device_property(OrgFreedesktopUPower.BATTERY1_OBJECT_PATH, "IsPresent", GLib.Variant("b", False))
        self.driver.find_element(by=AppiumBy.NAME, value="1:19")  # Remaining time (h) = (40 * 0.8) / 24.2 = 1.32
        state_element = self.driver.find_element(by=AppiumBy.NAME, value="Not present")
        # Plug in
        self.upower_interface.set_device_property(OrgFreedesktopUPower.BATTERY1_OBJECT_PATH, "IsPresent", GLib.Variant("b", True))
        self.driver.find_element(by=AppiumBy.NAME, value="2:59")
        new_state_element = self.driver.find_element(by=AppiumBy.NAME, value="Fully Charged")
        self.assertTrue(new_state_element == state_element)

    def test_20_ac_line_plugged_in(self) -> None:
        """
        Tests the battery state changes from Discharging to Charging
        """
        self.upower_interface.set_ac_plugged()
        self.driver.find_element(by=AppiumBy.NAME, value="Battery")
        self.driver.find_element(by=AppiumBy.NAME, value="90%")
        self.driver.find_element(by=AppiumBy.NAME, value="Charging")
        self.driver.find_element(by=AppiumBy.NAME, value="0:12")  # Remaining time (h) = 40 * 0.1 / 20

        # The second battery is untouched
        self.driver.find_element(by=AppiumBy.NAME, value="Battery 2")
        self.driver.find_element(by=AppiumBy.NAME, value="100%")
        self.driver.find_element(by=AppiumBy.NAME, value="Fully Charged")

    def test_21_charging_rate(self) -> None:
        """
        The remaining time should be updated accordingly when the charging rate changes
        """
        self.driver.find_element(by=AppiumBy.NAME, value="0:12")
        self.upower_interface.set_energy_props(OrgFreedesktopUPower.BATTERY0_OBJECT_PATH, -10.0)
        # Remaining time (h) = 40 * 0.1 / 10
        self.driver.find_element(by=AppiumBy.NAME, value="0:24")

    def test_22_estimating_charging_rate(self) -> None:
        """
        When the charging rate is 0, the remaining time label should be hidden
        """
        time_element = self.driver.find_element(by=AppiumBy.NAME, value="0:24")
        self.assertTrue(time_element.is_displayed())
        self.upower_interface.set_energy_props(OrgFreedesktopUPower.BATTERY0_OBJECT_PATH, 0.0)
        WebDriverWait(self.driver, 5).until(lambda _: not time_element.is_displayed())

    def test_23_hotplug_battery_when_charging(self) -> None:
        """
        After the secondary battery is not present, the remaining time should also be updated accordingly.
        In upower, "is-present" is not necessarily bound to "power-supply", so this also tests https://invent.kde.org/plasma/powerdevil/-/merge_requests/247
        """
        self.upower_interface.set_energy_props(OrgFreedesktopUPower.BATTERY0_OBJECT_PATH, -20.0)
        self.upower_interface.set_energy_props(OrgFreedesktopUPower.BATTERY1_OBJECT_PATH, 0, 80.0)
        self.driver.find_element(by=AppiumBy.NAME, value="0:36")  # Remaining time (h) = (40 * 0.1 + 40 * 0.2) / 20
        # Unplug
        self.upower_interface.set_device_property(OrgFreedesktopUPower.BATTERY1_OBJECT_PATH, "IsPresent", GLib.Variant("b", False))
        self.driver.find_element(by=AppiumBy.NAME, value="0:12")  # Remaining time (h) = (40 * 0.1) / 20
        self.driver.find_element(by=AppiumBy.NAME, value="Not present")
        # Plug in
        self.upower_interface.set_device_property(OrgFreedesktopUPower.BATTERY1_OBJECT_PATH, "IsPresent", GLib.Variant("b", True))
        self.driver.find_element(by=AppiumBy.NAME, value="0:36")  # Remaining time (h) = (40 * 0.1 + 40 * 0.2) / 20
        self.driver.find_element(by=AppiumBy.NAME, value="Charging")

    def test_30_device_added(self) -> None:
        """
        Add a external device like a wireless keyboard
        """
        properties = {
            "NativePath": GLib.Variant("s", "hidpp_battery_1"),
            "Vendor": GLib.Variant("s", "KDE"),
            "Model": GLib.Variant("s", "Wireless Keyboard"),
            "Serial": GLib.Variant("s", "1234-5678-91"),
            "UpdateTime": GLib.Variant('t', int(time.time())),
            "Type": GLib.Variant("u", 6),  # Keyboard
            "PowerSupply": GLib.Variant("b", False),
            "HasHistory": GLib.Variant("b", False),
            "HasStatistics": GLib.Variant("b", False),
            "Online": GLib.Variant("b", False),  # only valid for AC
            "Energy": GLib.Variant("d", 0.0),  # only valid for batteries
            "EnergyEmpty": GLib.Variant("d", 0.0),  # only valid for batteries
            "EnergyFull": GLib.Variant("d", 0.0),  # only valid for batteries
            "EnergyFullDesign": GLib.Variant("d", 0.0),  # only valid for batteries
            "EnergyRate": GLib.Variant("d", 0.0),  # only valid for batteries
            "Voltage": GLib.Variant("d", 0.0),
            "ChargeCycles": GLib.Variant('i', -1),  # only valid for batteries
            "Luminosity": GLib.Variant("d", 0.0),
            "TimeToEmpty": GLib.Variant("x", 0),  # only valid for batteries
            "TimeToFull": GLib.Variant("x", 0),  # only valid for batteries
            "Percentage": GLib.Variant("d", 100.0),  # only valid for batteries
            "Temperature": GLib.Variant("d", 0.0),  # only valid for batteries
            "IsPresent": GLib.Variant("b", False),  # only valid for batteries
            "State": GLib.Variant("u", 0),  # Unknown, only valid for batteries
            "IsRechargeable": GLib.Variant("b", False),  # only valid for batteries
            "Capacity": GLib.Variant("d", 0.0),  # only valid for batteries
            "Technology": GLib.Variant("u", 0),  # Unknown, only valid for batteries
            "WarningLevel": GLib.Variant("u", 1),  # None
            "BatteryLevel": GLib.Variant("u", 3),  # Low
            "IconName": GLib.Variant("s", ""),
        }
        self.upower_interface.add_device(OrgFreedesktopUPower.WIRELESS_KEYBOARD_OBJECT_PATH, properties)
        # Wireless Keyboard
        self.driver.find_element(by=AppiumBy.NAME, value="KDE Wireless Keyboard")

    def test_31_device_removed(self) -> None:
        """
        Unplug a external device like a wireless keyboard
        """
        keyboard_element = self.driver.find_element(by=AppiumBy.NAME, value="KDE Wireless Keyboard")
        self.assertTrue(keyboard_element.is_displayed())
        self.upower_interface.remove_device(OrgFreedesktopUPower.WIRELESS_KEYBOARD_OBJECT_PATH)
        WebDriverWait(self.driver, 5).until(lambda _: not keyboard_element.is_displayed())

    def test_32_device_added_removed_race_condition(self) -> None:
        """
        Detects if there is any potential race condition when a device is plugged in and unplugged in a flash
        """
        for i in range(50):
            print(f"i={str(i)}", file=sys.stderr, flush=True)
            self.test_30_device_added()
            self.test_31_device_removed()

        self.driver.find_element(by=AppiumBy.NAME, value="KDE Gaming Mouse")

    def test_41_set_power_profile(self) -> None:
        """
        Sets the power profile
        """
        # Remote changes
        self.ppd_interface.set_profile("power-saver")
        self.driver.find_element(AppiumBy.NAME, "Power Save")
        self.ppd_interface.set_profile("performance")
        self.driver.find_element(AppiumBy.NAME, "Performance")
        self.ppd_interface.set_profile("balanced")
        self.driver.find_element(AppiumBy.NAME, "Balanced")

        # Local changes
        slider_element = self.driver.find_element(AppiumBy.NAME, "Power Profile")
        self.driver.set_value(slider_element, str(0))
        slider_element.click()
        self.driver.find_element(AppiumBy.NAME, "Power Save")
        self.assertTrue(self.ppd_interface.active_profile_set_event.wait(5))
        self.ppd_interface.active_profile_set_event.clear()
        self.driver.set_value(slider_element, str(1))
        slider_element.click()
        self.driver.find_element(AppiumBy.NAME, "Balanced")
        self.assertTrue(self.ppd_interface.active_profile_set_event.wait(5))
        self.ppd_interface.active_profile_set_event.clear()
        self.driver.set_value(slider_element, str(2))
        slider_element.click()
        self.driver.find_element(AppiumBy.NAME, "Performance")
        self.assertTrue(self.ppd_interface.active_profile_set_event.wait(5))
        self.ppd_interface.active_profile_set_event.clear()

    def test_42_active_profile_holds(self) -> None:
        """
        Active profile holds are listed in the widget
        """
        cookie1 = self.ppd_interface.hold_profile("performance", "Building modules", "kde-builder")
        cookie2 = self.ppd_interface.hold_profile("performance", "Running tests", "appiumtest")
        label1 = self.driver.find_element(AppiumBy.NAME, "kde-builder: Building modules")
        label2 = self.driver.find_element(AppiumBy.NAME, "appiumtest: Running tests")

        self.ppd_interface.release_profile(cookie1)
        self.ppd_interface.release_profile(cookie2)
        wait = WebDriverWait(self.driver, 5)
        wait.until_not(lambda _: label1.is_displayed())
        wait.until_not(lambda _: label2.is_displayed())

    def test_43_performance_degraded_reason(self) -> None:
        """
        The widget can show the reason of why the performance is degraded
        """
        self.ppd_interface.set_profile("performance")
        self.ppd_interface.set_performance_degraded_reason("other")
        self.driver.find_element(AppiumBy.NAME, "Performance may be reduced.")
        self.ppd_interface.set_performance_degraded_reason("lap-detected")
        self.driver.find_element(AppiumBy.NAME, "Performance may be lowered to reduce heat generation because the computer has detected that it may be sitting on your lap.")
        self.ppd_interface.set_performance_degraded_reason("high-operating-temperature")
        reason_label = self.driver.find_element(AppiumBy.NAME, "Performance may be reduced because the computer is running too hot.")
        self.ppd_interface.set_performance_degraded_reason("")
        wait = WebDriverWait(self.driver, 5)
        wait.until_not(lambda _: reason_label.is_displayed())

    def has_inhibition(self, driver) -> bool:
        message: Gio.DBusMessage = Gio.DBusMessage.new_method_call("org.kde.Solid.PowerManagement.PolicyAgent", "/org/kde/Solid/PowerManagement/PolicyAgent", "org.kde.Solid.PowerManagement.PolicyAgent", "ListInhibitions")
        reply, _ = self.session_bus.send_message_with_reply_sync(message, Gio.DBusSendMessageFlags.NONE, 1000)
        return reply and reply.get_signature() == 'a(ss)' and reply.get_body().get_child_value(0).n_children() > 0

    def inhibit(self, application: str, reason: str) -> int:
        message: Gio.DBusMessage = Gio.DBusMessage.new_method_call("org.freedesktop.PowerManagement.Inhibit", "/org/freedesktop/PowerManagement/Inhibit", "org.freedesktop.PowerManagement.Inhibit", "Inhibit")
        message.set_body(GLib.Variant("(ss)", [application, reason]))
        reply, _ = self.session_bus.send_message_with_reply_sync(message, Gio.DBusSendMessageFlags.NONE, 1000)
        return reply and reply.get_signature() == 'u' and reply.get_body().get_child_value(0).get_uint32()

    def uninhibit(self, cookie: int) -> bool:
        message: Gio.DBusMessage = Gio.DBusMessage.new_method_call("org.freedesktop.PowerManagement.Inhibit", "/org/freedesktop/PowerManagement/Inhibit", "org.freedesktop.PowerManagement.Inhibit", "UnInhibit")
        message.set_body(GLib.Variant("(u)", [cookie]))
        reply, _ = self.session_bus.send_message_with_reply_sync(message, Gio.DBusSendMessageFlags.NONE, 1000)
        return reply and reply.get_error_name() is None

    def test_5_inhibition(self) -> None:
        """
        Manual inhibition or inhibition from application
        """
        switch_element = self.driver.find_element(AppiumBy.NAME, "Manually block")
        switch_element.click()
        wait = WebDriverWait(self.driver, 10)
        wait.until(self.has_inhibition)
        switch_element.click()
        wait.until_not(self.has_inhibition)

        application: str = "appiumtest"
        reason: str = "正在测试电池小部件"
        cookie: int = self.inhibit(application, reason)
        inhibition_hint = self.driver.find_element(AppiumBy.NAME, f"{application} is currently blocking sleep and screen locking ({reason})")
        self.uninhibit(cookie)
        wait.until_not(lambda _: inhibition_hint.is_displayed())


if __name__ == '__main__':
    assert os.path.exists(POWERDEVIL_PATH), f"{POWERDEVIL_PATH} does not exist"
    assert "ENABLE_DISPLAY_DEVICE" in os.environ, "Missing ENABLE_DISPLAY_DEVICE"
    logging.getLogger().setLevel(logging.INFO)
    unittest.main()
