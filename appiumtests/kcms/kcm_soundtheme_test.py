#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import shutil
import unittest
from typing import Final

from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import GLib

import sys
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils.base_test import KCMTest

KCM_ID: Final = "kcm_soundtheme"


class KCMSoundThemeTest(KCMTest):
    """
    Tests for kcm_soundtheme
    """

    kcm_id = KCM_ID

    @classmethod
    def setUpClass(cls) -> None:
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
                shutil.copytree("../../kcms/soundtheme/autotests/data/freedesktop", fdo_sound_theme_folder)
                cls.addClassCleanup(lambda: shutil.rmtree(fdo_sound_theme_folder))

        super().setUpClass()

    def test_0_open(self) -> None:
        """
        Tests the KCM can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "FreeDesktop")
        self.driver.find_element(AppiumBy.NAME, "Fallback sound theme from freedesktop.org")
        self.driver.find_element(AppiumBy.NAME, "Preview:")


if __name__ == '__main__':
    unittest.main()
