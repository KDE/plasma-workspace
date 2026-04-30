#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import os
import shutil
import tempfile
import unittest
from typing import Final

import gi
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

gi.require_version('Gdk', '4.0')
gi.require_version('GdkPixbuf', '2.0')
from gi.repository import Gdk, GdkPixbuf, GLib

import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils.base_test import KCMTest

KCM_ID: Final = "kcm_cursortheme"


class KCMCursorThemeTest(KCMTest):
    """
    Tests for kcm_cursortheme
    """

    kcm_id = KCM_ID
    extra_environ = {
        "QT_LOGGING_RULES": "kcm_cursortheme.debug=true",
    }

    @classmethod
    def setUpClass(cls) -> None:
        user_data_dir: Final[str] = GLib.get_user_data_dir()
        icons_folder: Final = os.path.join(user_data_dir, "icons")
        if not os.path.exists(icons_folder):
            os.mkdir(icons_folder)
            cls.addClassCleanup(lambda: shutil.rmtree(icons_folder))

        icon_theme_folder: Final = os.path.join(user_data_dir, "icons", "testcursortheme")
        assert not os.path.exists(icon_theme_folder)
        test_data_folder: Final = os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, "kcms", "cursortheme", "autotests", "data", "testcursortheme")
        assert os.path.exists(test_data_folder)
        shutil.copytree(test_data_folder, icon_theme_folder)
        cls.addClassCleanup(lambda: shutil.rmtree(icon_theme_folder))

        cls.extra_environ = {
            "QT_LOGGING_RULES": "kcm_cursortheme.debug=true",
            "XCURSOR_PATH": icons_folder,
        }
        super().setUpClass()

    def test_0_open(self) -> None:
        """
        Tests the KCM and the launcher feedback dialog can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "&Configure Launch Feedback…")
        self.driver.find_element(AppiumBy.NAME, "Test Cursor Theme (DO NOT TRANSLATE)")

        self.driver.find_element(AppiumBy.NAME, "&Configure Launch Feedback…").click()
        self.driver.find_element(AppiumBy.NAME, "Blinking")
        close_button = self.driver.find_element(AppiumBy.XPATH, "//button[@name='Close' and contains(@accessibility-id, 'LaunchFeedbackDialog')]")
        close_button.click()
        WebDriverWait(self.driver, 10).until(EC.invisibility_of_element(close_button))

    def test_1_cursor_theme_preview(self) -> None:
        """
        Tests if the cursor preview is loaded
        """
        with tempfile.TemporaryDirectory() as temp_dir:
            saved_image_path: str = os.path.join(temp_dir, "kcm_window.png")
            self.driver.get_screenshot_as_file(saved_image_path)
            full_image = base64.b64encode(Gdk.Texture.new_from_filename(saved_image_path).save_to_png_bytes().get_data()).decode()

        # Red
        partial_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 16, 16)
        partial_pixbuf.fill(0xff0000ff)
        partial_image = base64.b64encode(Gdk.Texture.new_for_pixbuf(partial_pixbuf).save_to_png_bytes().get_data()).decode()
        self.driver.find_image_occurrence(full_image, partial_image)


if __name__ == '__main__':
    unittest.main(failfast=True)
