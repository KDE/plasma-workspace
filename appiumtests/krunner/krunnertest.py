#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import subprocess
import time
import unittest
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

KDE_VERSION: Final = 6


class KRunnerTest(unittest.TestCase):
    """
    Tests for the main ui
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the KCM and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", "krunner")
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
            self.driver.get_screenshot_as_file(f"failed_test_shot_krunnertest_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        subprocess.check_call([f"kquitapp{KDE_VERSION}", "krunner"])
        for _ in range(10):
            try:
                subprocess.check_call(["pidof", "krunner"])
            except subprocess.CalledProcessError:
                break
            time.sleep(1)
        cls.driver.quit()

    def test_0_open(self) -> None:
        """
        Tests the main ui can be opened
        """
        button_element = self.driver.find_element(AppiumBy.NAME, "Configure")  # RunCommand.qml
        searchfield_element = self.driver.find_element(AppiumBy.NAME, "Search")

        wait = WebDriverWait(self.driver, 5)
        searchfield_element.send_keys("99999+99999")
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "199998")))

        actions = ActionChains(self.driver)
        actions.send_keys(Keys.ESCAPE).perform()
        wait.until_not(lambda _: button_element.is_displayed())


if __name__ == '__main__':
    unittest.main()
