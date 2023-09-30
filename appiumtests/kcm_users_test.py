#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import json
import subprocess
import sys
import tempfile
import time
import unittest
from typing import Final

import gi
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait

gi.require_version('Gdk', '3.0')
from gi.repository import Gdk

KDE_VERSION: Final = 6
KCM_ID: Final = "kcm_users"


def keyval_to_keycode(key_val: int):
    """
    @param key_val see https://www.cl.cam.ac.uk/~mgk25/ucs/keysymdef.h
    """
    keymap = Gdk.Keymap.get_default()
    ret, keys = keymap.get_entries_for_keyval(key_val)
    if not ret:
        raise RuntimeError("Failed to map key!")
    return keys[0]


def press_key(key_val: int, repeat_num: int = 1) -> None:
    with tempfile.NamedTemporaryFile() as handler:
        keymap = keyval_to_keycode(key_val)
        handler.write(json.dumps([{'type': 'keyboard', 'level': keymap.level, 'keycode': keymap.keycode}]).encode())
        handler.flush()
        for _ in range(repeat_num):
            assert subprocess.Popen(["selenium-webdriver-at-spi-inputsynth", handler.name], stdout=sys.stderr, stderr=sys.stderr).wait() == 0
            time.sleep(1)


def press_tab_key(repeat_num: int = 1) -> None:
    press_key(0xff09, repeat_num)


def press_space_key() -> None:
    press_key(0x0020)


def press_escape_key() -> None:
    press_key(0xff1b)


class KCMUsersTest(unittest.TestCase):
    """
    Tests for kcm_users
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the KCM and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", f"kcmshell{KDE_VERSION} {KCM_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
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
        self.driver.find_element(AppiumBy.NAME, "Add New")
        self.driver.find_element(AppiumBy.NAME, "Name:")
        self.driver.find_element(AppiumBy.NAME, "Change Password")
        self.driver.find_element(AppiumBy.NAME, "Delete User…")

    def test_1_pictures_sheet_1_open(self) -> None:
        """
        Only use keyboard to open the avatar sheet
        """
        self.driver.find_element(AppiumBy.NAME, "Change avatar")  # Press Space to activate the button
        press_tab_key(3)
        press_space_key()
        self.driver.find_element(AppiumBy.NAME, "Choose File…")
        #  Doesn't work without account service
        # self.driver.find_element(AppiumBy.NAME, "Initials")
        # self.driver.find_element(AppiumBy.NAME, "Placeholder Icon")


if __name__ == '__main__':
    unittest.main()
