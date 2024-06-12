#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import os
import shutil
import subprocess
import tempfile
import threading
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

gi.require_version('Gdk', '4.0')
gi.require_version('GdkPixbuf', '2.0')
gi.require_version('Gtk', '4.0')
from gi.repository import Gdk, GdkPixbuf, GLib, Gtk

WIDGET_ID: Final = "org.kde.plasma.clipboard"
KDE_VERSION: Final = 6


def spin_glib_main_loop() -> None:
    """
    Processes some pending events in the main loop
    """
    context = GLib.MainContext.default()
    count = 0
    while context.pending() or count < 10:
        if not context.pending():
            time.sleep(0.1)
        context.iteration(may_block=False)
        count += 1


class GtkApplicationThread(threading.Thread):

    def __init__(self) -> None:
        # Set up D-Bus loop
        self.app = Gtk.Application(application_id="org.kde.plasma.clipboardtest")
        self.activate_event = threading.Event()
        self.failSafeTimer = threading.Timer(120, self.app.quit)

        # Create the thread
        super().__init__()

    def on_activate(self, _app: Gtk.Application) -> None:
        self.activate_event.set()

    def run(self) -> None:
        self.failSafeTimer.start()
        self.app.connect("activate", self.on_activate)
        self.app.run(None)

    def quit(self) -> None:
        self.failSafeTimer.cancel()
        self.app.quit()


class ClipboardTest(unittest.TestCase):
    """
    Tests for the clipboard widget
    """

    driver: webdriver.Remote
    loop_thread: GtkApplicationThread
    appium_options: AppiumOptions
    klipper_data_file: str = ""

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        # Create history file to suppress warnings
        data_dir: str = GLib.get_user_data_dir()
        klipper_folder: Final = os.path.join(data_dir, "klipper")
        cls.klipper_data_file = os.path.join(klipper_folder, "history2.lst")
        assert not os.path.exists(cls.klipper_data_file)
        os.mkdir(klipper_folder)
        cls.addClassCleanup(lambda: shutil.rmtree(klipper_folder))

        shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "klipper/autotests/data/onetextentry.lst"), cls.klipper_data_file)

        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
            "QT_FATAL_WARNINGS": "1" if "KDECI_BUILD" in os.environ else "0",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;qt.qml.typeresolution.cycle.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false;org.kde.klipper.debug=true",
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.appium_options = options
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

        cls.loop_thread = GtkApplicationThread()
        cls.loop_thread.start()
        assert cls.loop_thread.activate_event.wait()

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

    def update_config_and_restart_clipboard(self, group: str | list[str], key: str | list[str], new_value: str | list[str]) -> None:
        self.driver.quit()
        time.sleep(1)  # Avoid races

        if isinstance(group, str):
            subprocess.check_call([f"kwriteconfig{KDE_VERSION}", "--file", "klipperrc", "--group", group, "--key", key, new_value])
        else:
            for g, k, v in zip(group, key, new_value):
                subprocess.check_call([f"kwriteconfig{KDE_VERSION}", "--file", "klipperrc", "--group", g, "--key", k, v])

        # Restart the widget to take effect
        self.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=self.appium_options)

    def gtk_copy(self, content: Gdk.ContentProvider, clipboard_mode: int = 0) -> None:
        """
        0 is clipboard, 1 is selection.
        """
        window = Gtk.Window()
        window.set_default_size(20, 20)
        window.set_visible(True)
        spin_glib_main_loop()

        clipboard: Gdk.Clipboard
        if clipboard_mode == 0:
            clipboard = window.get_display().get_clipboard()  # Clipboard
        else:
            clipboard = window.get_display().get_primary_clipboard()  # Selection
        self.assertTrue(clipboard.set_content(content))
        spin_glib_main_loop()

        window.set_visible(False)
        spin_glib_main_loop()

    def take_screenshot(self) -> str:
        with tempfile.TemporaryDirectory() as temp_dir:
            saved_image_path = os.path.join(temp_dir, "tray.png")
            self.driver.get_screenshot_as_file(saved_image_path)
            return base64.b64encode(Gdk.Texture.new_from_filename(saved_image_path).save_to_png_bytes().get_data()).decode()

    def test_3_bug487843_bug466414_empty_clip_crash(self) -> None:
        """
        When "Text selection - Always save in history" is enabled, a clip with empty text can crash klipper.
        @see https://bugs.kde.org/show_bug.cgi?id=487843
        @see https://bugs.kde.org/show_bug.cgi?id=466414
        """
        # Enable "Text selection - Always save in history" to test the two bugs
        self.update_config_and_restart_clipboard(["General"] * 2, ["IgnoreSelection", "SyncClipboards"], ["false", "true"])
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")

        content_text = Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes("", "utf-8")))
        # Clip data from Firefox have additional mime types, which cause the crash
        content_application = Gdk.ContentProvider.new_for_bytes("application/whatever", GLib.Bytes.new(bytes("abc", "utf-8")))
        content_union = Gdk.ContentProvider.new_union([content_text, content_application])
        self.gtk_copy(content_union)
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")  # Still alive
        self.gtk_copy(Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes("Fushan Wen", "utf-8"))))

    def test_4_ignore_image(self) -> None:
        """
        When `IgnoreImages` is set to false, the clipboard should save images.
        """
        # Enable "Only when explicitly copied" to test the two bugs
        shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "klipper/autotests/data/onetextentry.lst"), self.klipper_data_file)
        self.update_config_and_restart_clipboard(["General"] * 3, ["IgnoreImages", "IgnoreSelection", "SyncClipboards"], ["false", "true", "false"])
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")

        # Copy 3 color blocks to clipboard
        for color in (0xff0000ff, 0x00ff00ff, 0x0000ffff):
            pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 256, 256)
            pixbuf.fill(color)
            content_image = Gdk.ContentProvider.new_for_bytes("image/png", Gdk.Texture.new_for_pixbuf(pixbuf).save_to_png_bytes())
            self.gtk_copy(content_image)
            time.sleep(1)
            # Match the color block in the history
            partial_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 16, 16)
            partial_pixbuf.fill(color)
            partial_image = base64.b64encode(Gdk.Texture.new_for_pixbuf(partial_pixbuf).save_to_png_bytes().get_data()).decode()
            self.driver.find_image_occurrence(self.take_screenshot(), partial_image)


if __name__ == '__main__':
    unittest.main()
