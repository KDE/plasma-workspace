#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import subprocess
import sys
import unittest
from typing import Any

from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait


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
        desired_caps: dict[str, Any] = {}
        desired_caps["app"] = "plasmawindowed -p org.kde.plasma.nano org.kde.plasma.cameraindicator"
        desired_caps["timeouts"] = {'implicit': 5000}
        desired_caps["environ"] = {
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.warning=false;kpipewire_logging.warning=false",
        }
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', desired_capabilities=desired_caps)
        cls.driver.implicitly_wait = 10

        cls.pipewire_already_running_before_test = subprocess.Popen(["pidof", "pipewire"]).wait() == 0

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"cameraindicatortest_failed_test_shot_#{self.id()}.png")

    def test_0_open(self) -> None:
        """
        Tests the widget can be opened
        """
        if self.pipewire_already_running_before_test:
            self.skipTest("Pipewire is already running.")
        self.driver.find_element(AppiumBy.NAME, "Camera indicator is unavailable")

    def test_10_connect_to_pipewire(self) -> None:
        """
        Tests the widget can connect to pipewire
        """
        pipewire: subprocess.Popen
        if not self.pipewire_already_running_before_test:
            pipewire = subprocess.Popen(["pipewire"], stdout=sys.stderr, stderr=sys.stderr)
            self.addCleanup(pipewire.terminate)

        # Reconnecting takes at least 5s
        WebDriverWait(self.driver, 20).until(EC.presence_of_element_located((AppiumBy.NAME, "No camera is in use")))

        if not self.pipewire_already_running_before_test:
            pipewire.terminate()
            self.driver.find_element(AppiumBy.NAME, "Camera indicator is unavailable")


if __name__ == '__main__':
    unittest.main()
