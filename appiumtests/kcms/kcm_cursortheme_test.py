#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import os
import shutil
import subprocess
import tempfile
import time
import unittest
from typing import Final

import gi
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

gi.require_version('Gdk', '4.0')
gi.require_version('GdkPixbuf', '2.0')
from gi.repository import Gdk, GdkPixbuf, GLib

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
        test_data_folder: Final = os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, "kcms", "cursortheme", "autotests", "data", "testcursortheme")
        assert os.path.exists(test_data_folder)
        shutil.copytree(test_data_folder, icon_theme_folder)
        cls.addClassCleanup(lambda: shutil.rmtree(icon_theme_folder))

        options = AppiumOptions()
        options.set_capability("app", f"kcmshell{KDE_VERSION} {KCM_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
        # From XCURSORPATH
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
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
        cls.driver.find_element(AppiumBy.XPATH, "//*[@name='Close' and contains(@accessibility-id, 'Button')]").click()
        for _ in range(10):
            try:
                subprocess.check_call(["pidof", f"kcmshell{KDE_VERSION}"])
            except subprocess.CalledProcessError:
                break
            time.sleep(1)
        cls.driver.quit()

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
        WebDriverWait(self.driver, 10).until_not(lambda _: close_button.is_displayed())

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
