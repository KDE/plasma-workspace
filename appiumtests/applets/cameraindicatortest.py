#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import subprocess
import sys
import unittest
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

WIDGET_ID: Final = "org.kde.plasma.cameraindicator"


class CameraIndicatorTest(unittest.TestCase):
    """
    Tests for the camera indicator widget
    """

    driver: webdriver.Remote
    pipewire_already_running_before_test: bool = False

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false;kpipewire_logging.warning=false",
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

        cls.pipewire_already_running_before_test = subprocess.Popen(["pidof", "pipewire"]).wait() == 0

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_cameraindicatortest_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.quit()

    def test_0_open(self) -> None:
        """
        Tests the widget can be opened
        """
        if self.pipewire_already_running_before_test:
            self.driver.find_element(AppiumBy.NAME, "No camera is in use")
        else:
            self.driver.find_element(AppiumBy.NAME, "Camera indicator is unavailable")

    def test_10_connect_to_pipewire(self) -> None:
        """
        Tests the widget can connect to pipewire
        """
        if self.pipewire_already_running_before_test:
            self.skipTest("Pipewire is already running.")

        pipewire = subprocess.Popen(["pipewire"], stdout=sys.stderr, stderr=sys.stderr)
        self.addCleanup(pipewire.terminate)

        # Reconnecting takes at least 5s
        WebDriverWait(self.driver, 20).until(EC.presence_of_element_located((AppiumBy.NAME, "No camera is in use")))

        pipewire.terminate()
        self.driver.find_element(AppiumBy.NAME, "Camera indicator is unavailable")


if __name__ == '__main__':
    unittest.main()
