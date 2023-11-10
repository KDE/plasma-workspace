#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

import os
import pathlib
import subprocess
import unittest
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy

KDE_INSTALL_FULL_LIBEXECDIR: Final = os.environ.get("KDE_INSTALL_FULL_LIBEXECDIR", os.path.join(pathlib.Path.home(), "kde", "usr", "lib64", "libexec"))


class LogoutGreeterTests(unittest.TestCase):

    def setUp(self):
        self.proc = subprocess.Popen([f"{KDE_INSTALL_FULL_LIBEXECDIR}/ksmserver-logout-greeter", "--windowed"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        options = AppiumOptions()
        options.set_capability("app", str(self.proc.pid))
        options.set_capability("timeouts", {'implicit': 10000})
        self.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def tearDown(self):
        self.driver.quit()
        self.assertEqual(self.proc.returncode != None, True)
        try:
            self.proc.terminate(timeout=5)
        except subprocess.TimeoutExpired:
            self.proc.kill()
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_logoutgreetertest_{self.id()}.png")

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
    unittest.main()
