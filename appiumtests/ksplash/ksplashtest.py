#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

# pylint: disable=too-many-arguments

import os
import subprocess
import sys
import time
import unittest

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gio, GLib


class KSplashTest(unittest.TestCase):

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        options = AppiumOptions()
        options.set_capability("app", "ksplashqml --window")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false;org.kde.plasma.ksplashqml.debug=true",
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor=f'http://127.0.0.1:{os.getenv("FLASK_PORT", "4723")}', options=options)

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_ksplash_#{self.id()}.png")

    def test_1_bug494840_setStage(self) -> None:
        """
        Checks if the setStage method is ever called after starting plasma-ksplash-ready.service.
        """
        if os.getenv("TEST_WITH_KWIN_WAYLAND", "1") == "0":
            stages = ("wm", "kcminit", "ksmserver", "startPlasma", "desktop")
        else:
            stages = ("kcminit", "ksmserver", "startPlasma", "desktop")

        session_bus = Gio.bus_get_sync(Gio.BusType.SESSION)
        for stage in stages:
            message: Gio.DBusMessage = Gio.DBusMessage.new_method_call("org.kde.KSplash", "/KSplash", "org.kde.KSplash", "setStage")
            message.set_body(GLib.Variant("(s)", [stage]))
            session_bus.send_message_with_reply_sync(message, Gio.DBusSendMessageFlags.NONE, 3000)

        self.driver.find_element(AppiumBy.NAME, "Plasma made by KDE")

        with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "startkde", "systemd", "plasma-ksplash-ready.service.in"), encoding="utf-8") as handler:
            for line in handler:
                if line.startswith("ExecStart="):
                    command = line.removeprefix("ExecStart=").strip().split(" ")
                    subprocess.check_call(command, stdout=sys.stderr, stderr=sys.stderr)
                    break

        success = False
        for _ in range(10):
            try:
                subprocess.check_call(["pidof", "ksplashqml"])
            except subprocess.CalledProcessError:
                success = True
                break
            time.sleep(1)
        self.assertTrue(success)


if __name__ == '__main__':
    unittest.main()
