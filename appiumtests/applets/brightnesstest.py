#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

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
from OrgFreedesktopUPower import OrgFreedesktopUPower

WIDGET_ID: Final = "org.kde.plasma.brightness"
POWERDEVIL_PATH: Final = os.environ.get("POWERDEVIL_PATH", "/usr/libexec/org_kde_powerdevil")
POWERDEVIL_SERVICE_NAME: Final = "org.kde.Solid.PowerManagement"


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


class BrightnessTests(unittest.TestCase):
    """
    Tests for the brightness widget
    """

    dbus_daemon_pid: str
    driver: webdriver.Remote
    loop_thread: GLibMainLoopThread
    upower_interface: OrgFreedesktopUPower
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
        cls.upower_interface = OrgFreedesktopUPower(None, True)
        # Wait until the mocked upower interface is online
        assert cls.upower_interface.registered_event.wait(10), "upower interface is not ready"

        # Start PowerDevil which is used by the dataengine
        debug_env: dict[str, str] = os.environ.copy()
        debug_env["QT_LOGGING_RULES"] = "org.kde.powerdevil.debug=true"
        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
        assert not name_has_owner(session_bus, POWERDEVIL_SERVICE_NAME), "PowerDevil is already running"
        cls.powerdevil = subprocess.Popen([POWERDEVIL_PATH], env=debug_env, stdout=sys.stdout, stderr=sys.stderr)
        powerdevil_started: bool = False
        for _ in range(10):
            if name_has_owner(session_bus, POWERDEVIL_SERVICE_NAME):
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
            "QT_FATAL_WARNINGS": "0",
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
            self.driver.get_screenshot_as_file(f"failed_test_shot_{WIDGET_ID}_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.quit()
        cls.powerdevil.terminate()
        cls.upower_interface.quit()
        cls.loop_thread.quit()

    def test_1_bug487743_set_keyboard_brightness(self) -> None:
        """
        Can change the keyboard brightness
        """
        slider_element = self.driver.find_element(AppiumBy.NAME, "Keyboard Brightness")
        wait = WebDriverWait(self.driver, 5)
        for target_brightness in range(0, 3 + 1):
            self.driver.set_value(slider_element, str(target_brightness))
            slider_element.click()
            wait.until(lambda _: self.upower_interface.current_keyboard_brightness == target_brightness)


if __name__ == '__main__':
    assert os.path.exists(POWERDEVIL_PATH), f"{POWERDEVIL_PATH} does not exist"
    unittest.main()
