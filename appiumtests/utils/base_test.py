# SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

"""Base test class for Appium tests."""

from __future__ import annotations

import os
import subprocess
import time
import unittest
from typing import ClassVar

from appium import webdriver
from appium.options.common.base import AppiumOptions

from .constants import APPIUM_SERVER_URL, DEFAULT_IMPLICIT_TIMEOUT, KDE_VERSION, PROCESS_WAIT_INTERVAL, PROCESS_WAIT_ITERATIONS


def _filter_fatal_warnings(environ: dict[str, str]) -> dict[str, str]:
    """Only enable QT_FATAL_WARNINGS when KDECI_BUILD is set."""
    if "QT_FATAL_WARNINGS" in environ and "KDECI_BUILD" not in os.environ:
        environ = dict(environ)
        del environ["QT_FATAL_WARNINGS"]
    return environ


class PlasmaAppletTest(unittest.TestCase):
    """Base test class for Plasma applet tests using plasmawindowed."""

    driver: ClassVar[webdriver.Remote]
    widget_id: ClassVar[str]
    extra_environ: ClassVar[dict[str, str]] = {}

    @classmethod
    def setUpClass(cls) -> None:
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {cls.widget_id}")
        options.set_capability("timeouts", {'implicit': DEFAULT_IMPLICIT_TIMEOUT})
        environ: dict[str, str] = {"LC_ALL": "en_US.UTF-8"}
        environ.update(_filter_fatal_warnings(cls.extra_environ))
        options.set_capability("environ", environ)
        cls.driver = webdriver.Remote(command_executor=APPIUM_SERVER_URL, options=options)

    def tearDown(self) -> None:
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_{self.widget_id}_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        subprocess.check_call([f"kquitapp{KDE_VERSION}", "plasmawindowed"])
        for _ in range(PROCESS_WAIT_ITERATIONS):
            try:
                subprocess.check_call(["pidof", "plasmawindowed"])
            except subprocess.CalledProcessError:
                break
            time.sleep(PROCESS_WAIT_INTERVAL)
        cls.driver.quit()


class KCMTest(unittest.TestCase):
    """Base test class for KCM tests using kcmshell."""

    driver: ClassVar[webdriver.Remote]
    kcm_id: ClassVar[str]
    extra_environ: ClassVar[dict[str, str]] = {}

    @classmethod
    def setUpClass(cls) -> None:
        options = AppiumOptions()
        options.set_capability("app", f"kcmshell{KDE_VERSION} {cls.kcm_id}")
        options.set_capability("timeouts", {'implicit': DEFAULT_IMPLICIT_TIMEOUT})
        environ: dict[str, str] = {"LC_ALL": "en_US.UTF-8"}
        environ.update(_filter_fatal_warnings(cls.extra_environ))
        options.set_capability("environ", environ)
        cls.driver = webdriver.Remote(command_executor=APPIUM_SERVER_URL, options=options)

    def tearDown(self) -> None:
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_{self.kcm_id}_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        from appium.webdriver.common.appiumby import AppiumBy
        cls.driver.find_element(AppiumBy.XPATH, "//*[@name='Close' and contains(@accessibility-id, 'Button')]").click()
        for _ in range(PROCESS_WAIT_ITERATIONS):
            try:
                subprocess.check_call(["pidof", f"kcmshell{KDE_VERSION}"])
            except subprocess.CalledProcessError:
                break
            time.sleep(PROCESS_WAIT_INTERVAL)
        cls.driver.quit()
