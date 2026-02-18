#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import unittest

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.keys import Keys


class LockedTest(unittest.TestCase):

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        options = AppiumOptions()
        options.set_capability("app", f"{os.getenv('QML_EXEC')} {os.path.dirname(os.path.realpath(__file__))}/keyboardindicatortest.qml")
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)
        options.set_capability("timeouts", {'implicit': 10000})

    @classmethod
    def tearDownClass(cls) -> None:
        # Make sure to terminate the driver again, lest it dangles.
        cls.driver.quit()

    def test_presse_and_release(self) -> None:
        self.driver.find_element(AppiumBy.NAME, "0-0")
        ActionChains(self.driver).send_keys(Keys.CONTROL).perform()
        self.driver.find_element(AppiumBy.NAME, "1-1")
        ActionChains(self.driver).send_keys(Keys.CONTROL).perform()
        self.driver.find_element(AppiumBy.NAME, "2-2")


if __name__ == '__main__':
    unittest.main(failfast=True)
