#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

import unittest
from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait
from datetime import date
from dateutil.relativedelta import relativedelta
import subprocess
import time
import sys

class LogoutGreeterTests(unittest.TestCase):
    def setUp(self):
        self.proc = subprocess.Popen(["{}/ksmserver-logout-greeter".format(sys.argv[1]), "--windowed"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        desired_caps = {}
        desired_caps["app"] = str(self.proc.pid)
        desired_caps["timeouts"] = {'implicit': 10000}
        self.driver = webdriver.Remote(
            command_executor='http://127.0.0.1:4723',
            desired_capabilities=desired_caps)
        self.driver.implicitly_wait = 10

    def tearDown(self):
        self.driver.quit()
        self.assertEqual(self.proc.returncode != None, True)
        try:
            self.proc.terminate(timeout=5)
        except TimeoutExpired:
            self.proc.kill()
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file("failed_test_shot_{}.png".format(self.id()))

    def assertStdErrLine(self, expected):
        out, err = self.proc.communicate()
        errLines = err.splitlines()
        self.assertEqual(expected, errLines[(len(errLines) - 1)].decode('UTF-8'))

    def test_sleep(self):
        self.driver.find_element(by=AppiumBy.NAME, value="Sleep").click()
        self.assertStdErrLine("suspend")

    def test_hibernate(self):
        self.driver.find_element(by=AppiumBy.NAME, value="Hibernate").click()
        self.assertStdErrLine("hibernate")

    def test_restart(self):
        self.driver.find_element(by=AppiumBy.NAME, value="Restart").click()
        self.assertStdErrLine("reboot")

    def test_restart(self):
        self.driver.find_element(by=AppiumBy.NAME, value="Shut Down").click()
        self.assertStdErrLine("shutdown")

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(LogoutGreeterTests)
    unittest.TextTestRunner(verbosity=2).run(suite)
