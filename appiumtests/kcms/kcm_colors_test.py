#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import time
import unittest
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy

KDE_VERSION: Final = 6
KCM_ID: Final = "kcm_colors"


class KCMColorsTest(unittest.TestCase):
    """
    Tests for kcm_colors
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the KCM and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", f"kcmshell{KDE_VERSION} {KCM_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
        })
        cls.driver = webdriver.Remote(command_executor=f'http://127.0.0.1:{os.getenv("FLASK_PORT", "4723")}', options=options)

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_{KCM_ID}_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.find_element(AppiumBy.XPATH, "//*[@name='Close' and contains(@accessibility-id, 'Button')]").click()
        cls.driver.quit()

    def test_0_open(self) -> None:
        """
        Tests the KCM can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "All schemes")
        self.driver.find_element(AppiumBy.NAME, "Breeze Dark")


if __name__ == '__main__':
    unittest.main()
