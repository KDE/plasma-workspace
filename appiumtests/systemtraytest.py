#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import fcntl
import os
from subprocess import Popen, PIPE
import unittest
from datetime import date
from time import sleep
from typing import Any
import threading
import time

import gi

gi.require_version("Gtk", "3.0")  # StatusIcon is removed in 4
from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gtk
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait


class XEmbedTrayIcon(threading.Thread):
    """
    XEmbed tray icon implementation using Gtk.StatusIcon
    """

    def __init__(self, title: str) -> None:
        super().__init__()

        self.__timer: threading.Timer = threading.Timer(20, Gtk.main_quit)  # Failsafe

        self.__status_icon: Gtk.StatusIcon = Gtk.StatusIcon(title=title)
        self.__status_icon.set_from_icon_name("xorg")
        self.__status_icon.connect("button-press-event", self.__on_button_press_event)
        self.__status_icon.connect("button-release-event", self.__on_button_release_event)
        self.__status_icon.connect("popup-menu", self.__on_popup_menu)
        self.__status_icon.connect("scroll-event", self.__on_scroll_event)

    def run(self) -> None:
        self.__timer.start()
        Gtk.main()

    def quit(self) -> None:
        self.__timer.cancel()
        Gtk.main_quit()

    def __on_button_press_event(self, status_icon: Gtk.StatusIcon, button_event) -> None:
        print("button-press-event", button_event.button)

    def __on_button_release_event(self, status_icon: Gtk.StatusIcon, button_event) -> None:
        print("button-release-event", button_event.button)
        self.quit()

    def __on_popup_menu(self, status_icon: Gtk.StatusIcon, button: int, activate_time: int) -> None:
        print("popup-menu", button, activate_time)

    def __on_scroll_event(self, status_icon, scroll_event) -> None:
        print("scroll-event", scroll_event.delta_x, scroll_event.delta_y, int(scroll_event.direction))


class SystemTrayTests(unittest.TestCase):
    """
    Tests for the system tray widget
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        desired_caps: dict[str, Any] = {}
        desired_caps["app"] = "plasmawindowed -p org.kde.plasma.nano org.kde.plasma.systemtray"
        desired_caps["timeouts"] = {'implicit': 10000}
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', desired_capabilities=desired_caps)
        cls.driver.implicitly_wait = 10

    def setUp(self) -> None:
        self.kded = Popen(['kded5'])
        for _ in range(5):
            proc = Popen(['dbus-send', '--print-reply=literal', '--dest=org.freedesktop.DBus',
                          '/org/freedesktop/DBus', 'org.freedesktop.DBus.NameHasOwner', 'string:org.kde.kded5'], stdout=PIPE)
            out, err = proc.communicate()
            if 'boolean true' in str(out):
                break
            print(f"waiting for kded to appear on the dbus session")
            time.sleep(1)
        Popen(['dbus-send', '--print-reply', '--dest=org.kde.kded5', '/kded', 'org.kde.kded5.loadModule', 'string:statusnotifierwatcher'])

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"systemtraytest_failed_test_shot_#{self.id()}.png")
        self.kded.kill()

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Quits the webdriver
        """
        cls.driver.quit()

    def test_xembed_tray_icon(self) -> None:
        """
        Tests XEmbed tray icons can be listed and clicked in the tray.

        @note GTK doesn't like send_events and double checks the mouse position
        matches where the window is and is top level, so match the debug
        output from xembedsniproxy instead.
        """
        debug_env: dict[str, str] = os.environ.copy()
        debug_env["QT_LOGGING_RULES"] = "kde.xembedsniproxy.debug=true"
        xembedsniproxy: Popen[bytes] = Popen(['xembedsniproxy', '--platform', 'xcb'], env=debug_env, stderr=PIPE)  # For debug output
        print(f"xembedsniproxy PID: {xembedsniproxy.pid}")
        if not xembedsniproxy.stderr or xembedsniproxy.poll() != None:
            self.fail("xembedsniproxy is not available")

        title: str = f"XEmbed Status Icon Test {date.today().strftime('%Y%m%d')}"
        xembed_tray_icon: XEmbedTrayIcon = XEmbedTrayIcon(title)
        xembed_tray_icon.start()

        wait: WebDriverWait = WebDriverWait(self.driver, 10)
        try:
            # FocusScope in StatusNotifierItem.qml
            xembed_icon_item = wait.until(EC.presence_of_element_located((AppiumBy.NAME, title)))
        except TimeoutException:
            xembedsniproxy.terminate()
            xembed_tray_icon.quit()
            self.fail("Cannot find the XEmbed icon in the system tray: {}".format(xembedsniproxy.stderr.readlines()))

        # Now test clickable
        xembed_icon_item.click()

        fd: int = xembedsniproxy.stderr.fileno()
        fl: int = fcntl.fcntl(fd, fcntl.F_GETFL)
        fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

        count: int = 0
        success: bool = False
        while count < 10:
            line: bytes | None = xembedsniproxy.stderr.read()
            if not line:
                xembed_icon_item.click()
                sleep(1)
                count += 1
                continue

            line_str: str = line.decode(encoding="utf-8").strip()
            if "Received click" in line_str:
                success = True
                break

            count += 1

        xembedsniproxy.terminate()
        xembed_tray_icon.quit()
        self.assertTrue(success, "xembedsniproxy did not receive the click event")


if __name__ == '__main__':
    unittest.main()
