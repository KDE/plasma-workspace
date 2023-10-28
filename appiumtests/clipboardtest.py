#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from enum import Enum
from typing import Final

import gi
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.common.exceptions import NoSuchElementException
from selenium.webdriver.support.ui import WebDriverWait

gi.require_version('Gdk', '3.0')
from gi.repository import Gdk, GLib

WIDGET_ID: Final = "org.kde.plasma.clipboard"


class XKeyCode(Enum):
    """
    @see https://www.cl.cam.ac.uk/~mgk25/ucs/keysymdef.h
    """
    Tab = 0xff09
    Space = 0x0020
    Return = 0xff0d
    Enter = 0xff8d
    Escape = 0xff1b
    Up = 0xff52
    Right = 0xff53
    Down = 0xff54


def keyval_to_keycode(key_val: XKeyCode):
    keymap = Gdk.Keymap.get_default()
    ret, keys = keymap.get_entries_for_keyval(key_val.value)
    if not ret:
        raise RuntimeError("Failed to map key!")
    return keys[0]


def press_key(key_val: XKeyCode, repeat_num: int = 1) -> None:
    with tempfile.NamedTemporaryFile() as handler:
        keymap = keyval_to_keycode(key_val)
        handler.write(json.dumps([{'type': 'keyboard', 'level': keymap.level, 'keycode': keymap.keycode}]).encode())
        handler.flush()
        for _ in range(repeat_num):
            assert subprocess.Popen(["selenium-webdriver-at-spi-inputsynth", handler.name], stdout=sys.stderr, stderr=sys.stderr).wait() == 0
            time.sleep(1)


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
        options.set_capability("environ", {
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.warning=false;org.kde.klipper.debug=true",
        })
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
        self.driver.find_element(AppiumBy.NAME, "clipboard")

    def test_1_barcode_1_open_barcode_page(self) -> None:
        """
        Tests the barcode page can be opened
        """
        press_key(XKeyCode.Down)
        # Wait until the first item is selected
        try:
            self.driver.find_element(AppiumBy.NAME, "Show QR code")
        except NoSuchElementException:
            press_key(XKeyCode.Down)  # Try pressing down key again
            self.driver.find_element(AppiumBy.NAME, "Show QR code")
        press_key(XKeyCode.Right, 2)
        press_key(XKeyCode.Space)

        self.driver.find_element(AppiumBy.NAME, "QR Code")
        self.driver.find_element(AppiumBy.NAME, "Return to Clipboard")
        self.driver.find_element(AppiumBy.NAME, "Change the QR code type")

    def test_1_barcode_2_open_barcode_type_menu(self) -> None:
        """
        Open the barcode type menu
        """
        press_key(XKeyCode.Tab, 2)
        press_key(XKeyCode.Space)
        self.driver.find_element(AppiumBy.NAME, "QR Code")
        self.driver.find_element(AppiumBy.NAME, "Data Matrix")
        self.driver.find_element(AppiumBy.NAME, "Aztec")
        self.driver.find_element(AppiumBy.NAME, "Code 39")
        self.driver.find_element(AppiumBy.NAME, "Code 93")
        self.driver.find_element(AppiumBy.NAME, "Code 128")

    def test_1_barcode_3_change_barcode_type_Aztec(self) -> None:
        """
        Switch the current barcode type to Aztec
        """
        menu_item = self.driver.find_element(AppiumBy.NAME, "Aztec")

        # Switch to Aztec
        press_key(XKeyCode.Down, 3)
        press_key(XKeyCode.Space)
        WebDriverWait(self.driver, 5).until(lambda _: not menu_item.is_displayed())
        self.driver.find_element(AppiumBy.NAME, "Aztec")  # This is from barcodeItem

    def test_1_barcode_4_change_barcode_type_Code39(self) -> None:
        """
        Switch the current barcode type to Code 39
        """
        press_key(XKeyCode.Tab, 2)
        press_key(XKeyCode.Space)
        menu_item = self.driver.find_element(AppiumBy.NAME, "Code 39")
        press_key(XKeyCode.Down, 4)
        press_key(XKeyCode.Space)
        WebDriverWait(self.driver, 5).until(lambda _: not menu_item.is_displayed())
        self.driver.find_element(AppiumBy.NAME, "Code 39")  # This is from barcodeItem

    def test_1_barcode_5_change_barcode_type_Code93(self) -> None:
        """
        Switch the current barcode type to Code 93
        """
        press_key(XKeyCode.Tab, 2)
        press_key(XKeyCode.Space)
        menu_item = self.driver.find_element(AppiumBy.NAME, "Code 93")
        press_key(XKeyCode.Down, 5)
        press_key(XKeyCode.Space)
        WebDriverWait(self.driver, 5).until(lambda _: not menu_item.is_displayed())
        self.driver.find_element(AppiumBy.NAME, "Code 93")  # This is from barcodeItem

    def test_1_barcode_6_change_barcode_type_Code128(self) -> None:
        """
        Switch the current barcode type to Code 128
        """
        press_key(XKeyCode.Tab, 2)
        press_key(XKeyCode.Space)
        menu_item = self.driver.find_element(AppiumBy.NAME, "Code 128")
        press_key(XKeyCode.Down, 6)
        press_key(XKeyCode.Space)
        WebDriverWait(self.driver, 5).until(lambda _: not menu_item.is_displayed())
        self.driver.find_element(AppiumBy.NAME, "Code 128")  # This is from barcodeItem

    def test_1_barcode_7_change_barcode_type_QRCode(self) -> None:
        """
        Switch the current barcode type to QRCode
        """
        press_key(XKeyCode.Tab, 2)
        press_key(XKeyCode.Space)
        menu_item = self.driver.find_element(AppiumBy.NAME, "QR Code")
        press_key(XKeyCode.Down, 1)
        press_key(XKeyCode.Space)
        WebDriverWait(self.driver, 5).until(lambda _: not menu_item.is_displayed())
        self.driver.find_element(AppiumBy.NAME, "QR Code")  # This is from barcodeItem

    def test_1_barcode_8_go_back_to_list_from_barcode_page(self) -> None:
        """
        Go back to the list from the barcode page
        """
        button_item = self.driver.find_element(AppiumBy.NAME, "Return to Clipboard")
        self.assertTrue(button_item.is_displayed())
        press_key(XKeyCode.Tab)
        press_key(XKeyCode.Space)
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")
        self.assertFalse(button_item.is_displayed())

    def test_2_list_1_bug475696(self) -> None:
        """
        Pressing Return on an item should trigger the copy action
        @see https://bugs.kde.org/show_bug.cgi?id=475696
        """
        press_key(XKeyCode.Tab)
        press_key(XKeyCode.Down)
        self.driver.find_element(AppiumBy.NAME, "Show QR code")
        press_key(XKeyCode.Return)
        self.assertEqual(self.driver.get_clipboard_text(), "clipboard")
        press_key(XKeyCode.Enter)
        self.assertEqual(self.driver.get_clipboard_text(), "Fushan Wen")
        press_key(XKeyCode.Space)
        self.assertEqual(self.driver.get_clipboard_text(), "clipboard")

    def test_2_list_2_delete(self) -> None:
        """
        Deletes the top item and tests if the current clipboard changes
        @see https://bugs.kde.org/show_bug.cgi?id=475696
        """
        # Now "clipboard" is the first item
        press_key(XKeyCode.Up)
        self.driver.find_element(AppiumBy.NAME, "Remove from history").click()
        # The first item becomes the current clipboard item
        self.assertEqual(self.driver.get_clipboard_text(), "Fushan Wen")


if __name__ == '__main__':
    unittest.main()
