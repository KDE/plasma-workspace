#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import sys
import time
import unittest
from typing import ClassVar

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils.constants import APPIUM_SERVER_URL, DEFAULT_IMPLICIT_TIMEOUT, KDE_VERSION, PROCESS_WAIT_INTERVAL, PROCESS_WAIT_ITERATIONS


class KFontInstTest(unittest.TestCase):
    """
    Tests for the font management application
    """

    driver: ClassVar[webdriver.Remote]

    @classmethod
    def setUpClass(cls) -> None:
        options = AppiumOptions()
        options.set_capability("app", "kfontinst")
        options.set_capability("timeouts", {'implicit': DEFAULT_IMPLICIT_TIMEOUT})
        options.set_capability("environ", {"LC_ALL": "en_US.UTF-8"})
        cls.driver = webdriver.Remote(command_executor=APPIUM_SERVER_URL, options=options)

    def tearDown(self) -> None:
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_kfontinst_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        subprocess.check_call([f"kquitapp{KDE_VERSION}", "kfontinst"])
        for _ in range(PROCESS_WAIT_ITERATIONS):
            try:
                subprocess.check_call(["pidof", "kfontinst"])
            except subprocess.CalledProcessError:
                break
            time.sleep(PROCESS_WAIT_INTERVAL)
        cls.driver.quit()

    def test_0_open(self) -> None:
        """
        Tests the application can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "Find Duplicates…")


if __name__ == '__main__':
    unittest.main()
