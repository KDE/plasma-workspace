#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import shutil
import unittest
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import GLib

WIDGET_ID: Final = "org.kde.plasma.clipboard"


class ClipboardTest(unittest.TestCase):
    """
    Tests for the clipboard widget
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        # Create history file to suppress warnings
        data_dir: str = GLib.get_user_data_dir()
        klipper_folder: Final = os.path.join(data_dir, "klipper")
        klipper_data_file: Final = os.path.join(klipper_folder, "history2.lst")
        if os.path.exists(klipper_data_file):
            shutil.move(klipper_data_file, f"{klipper_data_file}.bak")
            cls.addClassCleanup(lambda: shutil.move(f"{klipper_data_file}.bak", klipper_data_file))
        elif os.path.exists(klipper_folder):
            cls.addClassCleanup(lambda: os.remove(klipper_data_file))
        else:
            os.mkdir(klipper_folder)
            cls.addClassCleanup(lambda: shutil.rmtree(klipper_folder))

        shutil.copy("../klipper/autotests/data/onetextentry.lst", klipper_data_file)

        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_{WIDGET_ID}_#{self.id()}.png")

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
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")


if __name__ == '__main__':
    unittest.main()
