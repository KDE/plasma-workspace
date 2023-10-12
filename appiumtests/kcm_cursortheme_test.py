#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import shutil
import tempfile
import time
import unittest
from typing import Final

import gi
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy

gi.require_version("GdkPixbuf", "2.0")

from gi.repository import GdkPixbuf, GLib

KDE_VERSION: Final = 6
KCM_ID: Final = "kcm_cursortheme"


class KCMCursorThemeTest(unittest.TestCase):
    """
    Tests for kcm_cursortheme
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the KCM and initialize the webdriver
        """
        # Install the custom cursor theme
        user_data_dir: Final[str] = GLib.get_user_data_dir()
        icons_folder: Final = os.path.join(user_data_dir, "icons")
        if not os.path.exists(icons_folder):
            os.mkdir(icons_folder)
            cls.addClassCleanup(lambda: shutil.rmtree(icons_folder))

        icon_theme_folder: Final = os.path.join(user_data_dir, "icons", "testcursortheme")
        assert not os.path.exists(icon_theme_folder)
        assert os.path.exists("../kcms/cursortheme/autotests/data/testcursortheme")
        shutil.copytree("../kcms/cursortheme/autotests/data/testcursortheme", icon_theme_folder)
        cls.addClassCleanup(lambda: shutil.rmtree(icon_theme_folder))

        options = AppiumOptions()
        options.set_capability("app", f"kcmshell{KDE_VERSION} {KCM_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
        # From XCURSORPATH
        options.set_capability("environ", {
            "XCURSOR_PATH": icons_folder,
            "QT_LOGGING_RULES": "kcm_cursortheme.debug=true",
        })
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

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
        self.driver.find_element(AppiumBy.NAME, "&Configure Launch Feedback…")
        self.driver.find_element(AppiumBy.NAME, "Test Cursor Theme (DO NOT TRANSLATE)")

    def test_1_cursor_theme_preview(self) -> None:
        """
        Tests if the cursor preview is loaded
        """
        time.sleep(3)  # Wait until the window appears

        with tempfile.TemporaryDirectory() as temp_dir:
            saved_image_path: str = os.path.join(temp_dir, "kcm_window.png")
            self.assertTrue(self.driver.get_screenshot_as_file(saved_image_path))

            pixbuf: GdkPixbuf.Pixbuf | None = GdkPixbuf.Pixbuf.new_from_file(saved_image_path)
            self.assertIsNotNone(pixbuf)
            self.assertGreater(pixbuf.get_width(), 0)
            self.assertGreater(pixbuf.get_height(), 0)
            self.assertEqual(pixbuf.get_n_channels(), 4)  # R, G, B, A

            pixel_data: bytes = pixbuf.get_pixels()  # R, G, B, A, R, G, B, A, ...

        # Now scan each pixels to match the expected colors
        i = 0
        count = [
            0,  # Red
            0,  # Green
            0,  # Blue
            0,  # Yellow
        ]
        while i < len(pixel_data):
            color = (pixel_data[i], pixel_data[i + 1], pixel_data[i + 2])  # 0~255
            i += 4

            match color:
                case (255, 0, 0):  # Red
                    count[0] += 1
                case (0, 255, 0):  # Green
                    count[1] += 1
                case (0, 0, 255):  # Blue
                    count[2] += 1
                case (255, 255, 0):  # Yellow
                    count[3] += 1

            if count >= [32] * 4:
                return

        self.fail(f"Cursor preview is not loaded {count} {pixel_data}")


if __name__ == '__main__':
    unittest.main()
