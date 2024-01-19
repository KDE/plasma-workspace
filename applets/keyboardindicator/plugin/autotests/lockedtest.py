#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import time
import unittest

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy


class LockedTest(unittest.TestCase):

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        options = AppiumOptions()
        options.set_capability("app", f"{os.getenv('QML_EXEC')} {os.path.dirname(os.path.realpath(__file__))}/lockedtest.qml")
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)
        options.set_capability("timeouts", {'implicit': 10000})

    @classmethod
    def tearDownClass(cls) -> None:
        # Make sure to terminate the driver again, lest it dangles.
        cls.driver.quit()

    def test_caps_lock(self) -> None:
        self.driver.find_element(AppiumBy.NAME, "unlocked")
        subprocess.check_call(["xdotool", "key", "Caps_Lock"])
        self.driver.find_element(AppiumBy.NAME, "locked")
        subprocess.check_call(["xdotool", "key", "Caps_Lock"])
        self.driver.find_element(AppiumBy.NAME, "unlocked")


if __name__ == '__main__':
    unittest.main()
