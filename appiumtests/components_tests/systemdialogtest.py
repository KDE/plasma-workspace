#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import unittest

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait


class SystemDialogTest(unittest.TestCase):

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        if len(subprocess.check_output(["kreadconfig6", "--group", "KDE", "--key", "LookAndFeelPackage"]).decode(encoding="utf-8").strip()) == 0 or "KDECI_BUILD" in os.environ:
            subprocess.check_call(["kwriteconfig6", "--group", "KDE", "--key", "LookAndFeelPackage", "org.kde.breeze.desktop"])

        options = AppiumOptions()
        options.set_capability("app", f"{os.getenv('QML_EXEC')} {os.path.join(os.path.dirname(os.path.realpath(__file__)), 'systemdialogtest.qml')}")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false",
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_systemdialogtest_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        # Make sure to terminate the driver again, lest it dangles.
        cls.driver.quit()

    def test_1_simple_dialog_desktop(self) -> None:
        self.driver.find_element(AppiumBy.NAME, "Simple dialog (Desktop)").click()
        self.driver.find_element(AppiumBy.NAME, "Reset Data")
        ok_element = self.driver.find_element(AppiumBy.NAME, "Yes")
        ok_element.click()
        wait = WebDriverWait(self.driver, 5)
        wait.until_not(lambda _: ok_element.is_displayed())

    def test_2_simple_list(self) -> None:
        self.driver.find_element(AppiumBy.NAME, "Simple List").click()
        self.driver.find_element(AppiumBy.NAME, "Reset Data")
        self.driver.find_element(AppiumBy.NAME, "banana3")
        cancel_element = self.driver.find_element(AppiumBy.NAME, "Cancel")
        cancel_element.click()
        wait = WebDriverWait(self.driver, 5)
        wait.until_not(lambda _: cancel_element.is_displayed())

    def test_3_polkit_dialog_desktop(self) -> None:
        self.driver.find_element(AppiumBy.NAME, "Polkit dialog (Desktop)").click()
        title_element = self.driver.find_element(AppiumBy.NAME, "Authentication Required")
        self.driver.find_element(AppiumBy.NAME, "Show Password")
        self.assertFalse(self.driver.find_element(AppiumBy.NAME, "OK").is_enabled())
        self.driver.find_element(AppiumBy.NAME, "Details").click()
        wait = WebDriverWait(self.driver, 5)
        wait.until_not(lambda _: title_element.is_displayed())


if __name__ == '__main__':
    unittest.main()
