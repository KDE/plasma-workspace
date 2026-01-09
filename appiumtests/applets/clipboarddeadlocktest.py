#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import sys
import unittest

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait


class ClipboardDeadlockTest(unittest.TestCase):
    """
    A test for the clipboard deadlock issue
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        options = AppiumOptions()
        options.set_capability("app", f"python3 {os.path.join(os.path.dirname(os.path.abspath(__file__)), 'clipboardtest', 'deadlocktestwindow.py')}")
        options.set_capability("environ", {
            "QT_QPA_PLATFORM": "wayland" if os.getenv("TEST_WITH_KWIN_WAYLAND", "1") != "0" else "xcb",
            "LD_PRELOAD": subprocess.check_output(["gcc", "-print-file-name=libasan.so"]).strip().decode(encoding="utf-8"),
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_deadlocktestwindow_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.quit()

    def test_0_set_regular_clipboard(self) -> None:
        text = "From Normal Clipboard"
        self.driver.find_element(AppiumBy.NAME, "Normal Text Edit").send_keys(text)
        self.driver.find_element(AppiumBy.NAME, "Submit1").click()
        WebDriverWait(self.driver, 10).until(lambda d: d.find_element(AppiumBy.NAME, "Normal Clipboard View").text == text)
        WebDriverWait(self.driver, 10).until(lambda d: d.find_element(AppiumBy.NAME, "System Clipboard View").text == text)

    def test_1_set_system_clipboard(self) -> None:
        text = "From System Clipboard"
        self.driver.find_element(AppiumBy.NAME, "System Text Edit").send_keys(text)
        self.driver.find_element(AppiumBy.NAME, "Submit2").click()
        WebDriverWait(self.driver, 10).until(lambda d: d.find_element(AppiumBy.NAME, "Normal Clipboard View").text == text)
        WebDriverWait(self.driver, 10).until(lambda d: d.find_element(AppiumBy.NAME, "System Clipboard View").text == text)


if __name__ == '__main__':
    try:
        from PySide6.QtCore import QObject
    except ModuleNotFoundError:
        sys.exit(0)
    unittest.main()
