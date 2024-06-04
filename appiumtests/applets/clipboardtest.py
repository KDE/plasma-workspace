#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import shutil
import subprocess
import sys
import time
import unittest
from typing import Final

import gi
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.common.exceptions import NoSuchElementException
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support.ui import WebDriverWait

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "utils"))
from GLibMainLoopThread import GLibMainLoopThread

gi.require_version('Gdk', '4.0')
gi.require_version('Gtk', '4.0')
from gi.repository import Gdk, GLib, Gtk

WIDGET_ID: Final = "org.kde.plasma.clipboard"
KDE_VERSION: Final = 6


class ClipboardTest(unittest.TestCase):
    """
    Tests for the clipboard widget
    """

    driver: webdriver.Remote
    loop_thread: GLibMainLoopThread
    appium_options: AppiumOptions

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        # Create history file to suppress warnings
        data_dir: str = GLib.get_user_data_dir()
        klipper_folder: Final = os.path.join(data_dir, "klipper")
        klipper_data_file: Final = os.path.join(klipper_folder, "history2.lst")
        assert not os.path.exists(klipper_data_file)
        os.mkdir(klipper_folder)
        cls.addClassCleanup(lambda: shutil.rmtree(klipper_folder))

        shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "klipper/autotests/data/onetextentry.lst"), klipper_data_file)

        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;qt.qml.typeresolution.cycle.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false;org.kde.klipper.debug=true",
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.appium_options = options
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

        cls.loop_thread = GLibMainLoopThread()
        cls.loop_thread.start()

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
        cls.loop_thread.quit()

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
        actions = ActionChains(self.driver)
        actions.send_keys(Keys.DOWN).perform()
        # Wait until the first item is selected
        try:
            self.driver.find_element(AppiumBy.NAME, "Show QR code").click()
        except NoSuchElementException:
            actions.send_keys(Keys.DOWN).perform()  # Try pressing down key again
            self.driver.find_element(AppiumBy.NAME, "Show QR code").click()

        self.driver.find_element(AppiumBy.NAME, "QR Code")
        self.driver.find_element(AppiumBy.NAME, "Return to Clipboard")
        self.driver.find_element(AppiumBy.NAME, "Change the QR code type")

    def test_1_barcode_2_change_barcode_type(self) -> None:
        """
        Opens the barcode type menu and changes the current barcode type
        """
        self.driver.find_element(AppiumBy.NAME, "Change the QR code type").click()
        menu_item = self.driver.find_element(AppiumBy.NAME, "Aztec")
        # Switch to Aztec
        actions = ActionChains(self.driver)
        for _ in range(3):
            actions.send_keys(Keys.DOWN).perform()
            time.sleep(1)
        actions.send_keys(Keys.SPACE).perform()
        WebDriverWait(self.driver, 5).until_not(lambda _: menu_item.is_displayed())
        self.driver.find_element(AppiumBy.NAME, "Aztec")  # This is from barcodeItem

    def test_1_barcode_3_go_back_to_list_from_barcode_page(self) -> None:
        """
        Go back to the list from the barcode page
        """
        button_item = self.driver.find_element(AppiumBy.NAME, "Return to Clipboard")
        self.assertTrue(button_item.is_displayed())
        button_item.click()
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")
        self.assertFalse(button_item.is_displayed())

    def test_2_list_1_bug475696(self) -> None:
        """
        Pressing Return on an item should trigger the copy action
        @see https://bugs.kde.org/show_bug.cgi?id=475696
        """
        ActionChains(self.driver).send_keys(Keys.TAB).send_keys(Keys.DOWN).perform()
        self.driver.find_element(AppiumBy.NAME, "Show QR code")
        ActionChains(self.driver).send_keys(Keys.RETURN).perform()
        self.assertEqual(self.driver.get_clipboard_text(), "clipboard")
        ActionChains(self.driver).send_keys(Keys.ENTER).perform()
        self.assertEqual(self.driver.get_clipboard_text(), "Fushan Wen")
        ActionChains(self.driver).send_keys(Keys.SPACE).perform()
        self.assertEqual(self.driver.get_clipboard_text(), "clipboard")

    def test_2_list_2_delete(self) -> None:
        """
        Deletes the top item and tests if the current clipboard changes
        @see https://bugs.kde.org/show_bug.cgi?id=475696
        """
        # Now "clipboard" is the first item
        ActionChains(self.driver).send_keys(Keys.UP).perform()
        self.driver.find_element(AppiumBy.NAME, "Remove from history").click()
        # The first item becomes the current clipboard item
        self.assertEqual(self.driver.get_clipboard_text(), "Fushan Wen")

    def test_3_bug487843_bug466414_empty_clip_crash(self) -> None:
        """
        When "Text selection - Always save in history" is enabled, a clip with empty text can crash klipper.
        @see https://bugs.kde.org/show_bug.cgi?id=487843
        @see https://bugs.kde.org/show_bug.cgi?id=466414
        """
        self.driver.quit()

        # Enable "Text selection - Always save in history" to test the two bugs
        subprocess.check_call([f"kwriteconfig{KDE_VERSION}", "--file", "klipperrc", "--group", "General", "--key", "IgnoreSelection", "false"])
        subprocess.check_call([f"kwriteconfig{KDE_VERSION}", "--file", "klipperrc", "--group", "General", "--key", "SyncClipboards", "true"])

        # Restart the widget to take effect
        self.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=self.appium_options)
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")

        window = Gtk.Window()
        clipboard: Gdk.Clipboard = window.get_display().get_clipboard()
        content_text = Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes("", "utf-8")))
        # Clip data from Firefox have additional mime types, which cause the crash
        content_application = Gdk.ContentProvider.new_for_bytes("application/whatever", GLib.Bytes.new(bytes("abc", "utf-8")))
        content_union = Gdk.ContentProvider.new_union([content_text, content_application])
        self.assertTrue(clipboard.set_content(content_union))
        time.sleep(3)  # roundtrip
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")  # Still alive
        self.assertEqual(self.driver.get_clipboard_text(), "Fushan Wen")  # No change to the clipboard


if __name__ == '__main__':
    unittest.main()
