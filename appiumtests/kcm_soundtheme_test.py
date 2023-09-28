#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import shutil
import unittest
from typing import Any, Final

from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import GLib

KDE_VERSION: Final = 6
KCM_ID: Final = "kcm_soundtheme"


class KCMSoundThemeTest(unittest.TestCase):
    """
    Tests for kcm_soundtheme
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the KCM and initialize the webdriver
        """
        # In CI there is no sound theme by default, so manually install one
        system_data_dirs: Final[list[str]] = GLib.get_system_data_dirs()
        freedesktop_sound_theme_installed: bool = False
        for system_data_dir in system_data_dirs:
            if os.path.exists(os.path.join(system_data_dir, "sounds", "freedesktop", "index.theme")):
                freedesktop_sound_theme_installed = True
                break

        if not freedesktop_sound_theme_installed:
            user_data_dir: Final[str] = GLib.get_user_data_dir()
            sounds_folder: Final = os.path.join(user_data_dir, "sounds")
            if not os.path.exists(sounds_folder):
                os.mkdir(sounds_folder)
                cls.addClassCleanup(lambda: shutil.rmtree(sounds_folder))

            fdo_sound_theme_folder: Final = os.path.join(user_data_dir, "sounds", "freedesktop")
            if not os.path.exists(fdo_sound_theme_folder):
                shutil.copytree("../kcms/soundtheme/autotests/data/freedesktop", fdo_sound_theme_folder)
                cls.addClassCleanup(lambda: shutil.rmtree(fdo_sound_theme_folder))

        desired_caps: dict[str, Any] = {}
        desired_caps["app"] = f"kcmshell{KDE_VERSION} {KCM_ID}"
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', desired_capabilities=desired_caps)
        cls.driver.implicitly_wait = 10

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_{KCM_ID}_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.quit()

    def test_0_open(self) -> None:
        """
        Tests the KCM can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "FreeDesktop")
        self.driver.find_element(AppiumBy.NAME, "Fallback sound theme from freedesktop.org")
        self.driver.find_element(AppiumBy.NAME, "Preview sounds:")


if __name__ == '__main__':
    unittest.main()
