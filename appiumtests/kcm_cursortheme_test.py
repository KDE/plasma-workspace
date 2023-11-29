#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import os
import shutil
import tempfile
import time
import unittest
from typing import Final

import cv2 as cv
import numpy as np
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import GLib

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

            cv_first_image = cv.imread(saved_image_path, cv.IMREAD_COLOR)
            first_image = base64.b64encode(cv.imencode('.png', cv_first_image)[1].tobytes()).decode()

        # Red
        cv_second_image = np.zeros((16, 16, 3), dtype=np.uint8)
        cv_second_image[:, :] = [0, 0, 255]
        second_image = base64.b64encode(cv.imencode('.png', cv_second_image)[1].tobytes()).decode()
        self.driver.find_image_occurrence(first_image, second_image)
        # Green
        cv_second_image[:, :] = [0, 255, 0]
        second_image = base64.b64encode(cv.imencode('.png', cv_second_image)[1].tobytes()).decode()
        self.driver.find_image_occurrence(first_image, second_image)
        # Blue
        cv_second_image[:, :] = [255, 0, 0]
        second_image = base64.b64encode(cv.imencode('.png', cv_second_image)[1].tobytes()).decode()
        self.driver.find_image_occurrence(first_image, second_image)
        # Yellow
        cv_second_image[:, :] = [0, 255, 255]
        second_image = base64.b64encode(cv.imencode('.png', cv_second_image)[1].tobytes()).decode()
        self.driver.find_image_occurrence(first_image, second_image)


if __name__ == '__main__':
    unittest.main()