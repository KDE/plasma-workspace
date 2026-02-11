#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
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
from gi.repository import Gio, GLib
from selenium.webdriver.support.ui import WebDriverWait

WIDGET_ID: Final = "org.kde.plasma.appmenu"
KDE_VERSION: Final = 6
CMAKE_RUNTIME_OUTPUT_DIRECTORY: Final = os.getenv("CMAKE_RUNTIME_OUTPUT_DIRECTORY", os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "build", "bin"))


def name_has_owner(session_bus: Gio.DBusConnection, name: str) -> bool:
    """
    Whether the given name is available on session bus
    """
    message: Gio.DBusMessage = Gio.DBusMessage.new_method_call("org.freedesktop.DBus", "/", "org.freedesktop.DBus", "NameHasOwner")
    message.set_body(GLib.Variant("(s)", [name]))
    reply, _ = session_bus.send_message_with_reply_sync(message, Gio.DBusSendMessageFlags.NONE, 1000)
    return reply and reply.get_signature() == 'b' and reply.get_body().get_child_value(0).get_boolean()


class AppMenuTest(unittest.TestCase):
    """
    Tests for the appmenu widget
    """

    driver: webdriver.Remote
    kded: subprocess.Popen | None = None
    gmenudbusmenuproxy: subprocess.Popen | None = None

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
        if not name_has_owner(session_bus, f"org.kde.kded{KDE_VERSION}"):
            cls.kded = subprocess.Popen([f"kded{KDE_VERSION}"])
            kded_started: bool = False
            for _ in range(10):
                if name_has_owner(session_bus, f"org.kde.kded{KDE_VERSION}"):
                    kded_started = True
                    break
                print(f"waiting for kded{KDE_VERSION} to appear on the dbus session")
                time.sleep(1)
            assert kded_started, "kded is not started"
            kded_reply: GLib.Variant = session_bus.call_sync(f"org.kde.kded{KDE_VERSION}", "/kded", f"org.kde.kded{KDE_VERSION}", "loadModule", GLib.Variant("(s)", ["appmenu"]), GLib.VariantType("(b)"), Gio.DBusSendMessageFlags.NONE, 1000)
            assert kded_reply.get_child_value(0).get_boolean(), "appmenu module is not loaded"

        if not "gmenudbusmenuproxy" in subprocess.check_output(["ps", "-ef"]).decode():
            cls.gmenudbusmenuproxy = subprocess.Popen([os.path.join(CMAKE_RUNTIME_OUTPUT_DIRECTORY, "gmenudbusmenuproxy")])

        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false",
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            if os.getenv("TEST_WITH_KWIN_WAYLAND", "1") == "0":
                subprocess.check_call(["import", "-window", "root", f"failed_test_shot_{WIDGET_ID}_#{self.id()}.png"])
            else:
                self.driver.get_screenshot_as_file(f"failed_test_shot_{WIDGET_ID}_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        if cls.gmenudbusmenuproxy is not None:
            subprocess.check_call([f"kquitapp{KDE_VERSION}", "gmenudbusmenuproxy"])
            cls.gmenudbusmenuproxy.wait(5)
        if cls.kded is not None:
            subprocess.check_call([f"kquitapp{KDE_VERSION}", f"kded{KDE_VERSION}"])
            cls.kded.wait(5)
        cls.driver.quit()

    def test_0_open(self) -> None:
        """
        Tests the widget can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "Global Menu")

    def test_1_gtk_application_menu(self) -> None:
        """
        Activate a window with two hidden submenus and match strings in the widget
        """
        gtk_app = subprocess.Popen(["python3", os.path.join(os.path.dirname(os.path.abspath(__file__)), "appmenutest_window.py")], stdout=sys.stderr, stderr=sys.stderr)
        self.addCleanup(gtk_app.terminate)

        menu1 = self.driver.find_element(AppiumBy.NAME, "foo")
        menu2 = self.driver.find_element(AppiumBy.NAME, "bar")

        gtk_app.terminate()
        gtk_app.wait()
        wait = WebDriverWait(self.driver, 5)
        wait.until_not(lambda _: menu1.is_displayed())
        wait.until_not(lambda _: menu2.is_displayed())


if __name__ == '__main__':
    unittest.main()
