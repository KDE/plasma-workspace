#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import logging
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
from gi.repository import Gdk, GdkPixbuf, Gio, GLib, Gtk

WIDGET_ID: Final = "org.kde.plasma.clipboard"
KDE_VERSION: Final = 6


class SpinThread(threading.Thread):

    def __init__(self) -> None:
        self.quit_event = threading.Event()
        # Create the thread
        super().__init__()

    @classmethod
    def spin(cls) -> None:
        context = GLib.MainContext.default()
        count = 0
        while context.pending() or count < 10:
            count += 1
            if not context.pending():
                time.sleep(0.1)
                continue
            context.iteration(may_block=True)

    def run(self) -> None:
        while not self.quit_event.is_set():
            self.spin()


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
    gtk_thread: GtkApplicationThread
    spin_thread: SpinThread
    klipper_proxy: Gio.DBusProxy
    klipper_updated_event: threading.Event
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
        options.set_capability(
            "environ",
            {
                "LC_ALL": "en_US.UTF-8",
                "QT_FATAL_WARNINGS": "0",  # DataControlOffer: timeout reading from pipe
                "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;qt.qml.typeresolution.cycle.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false;org.kde.klipper.debug=true",
            })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.appium_options = options
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

        cls.gtk_thread = GtkApplicationThread()
        cls.gtk_thread.start()
        assert cls.gtk_thread.activate_event.wait()

        cls.spin_thread = SpinThread()
        cls.spin_thread.start()

        cls.klipper_proxy = Gio.DBusProxy.new_for_bus_sync(Gio.BusType.SESSION, 0, None, "org.kde.klipper", "/klipper", "org.kde.klipper.klipper")
        cls.klipper_updated_event = threading.Event()

    def setUp(self) -> None:
        self.klipper_updated_event.clear()

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
        subprocess.check_call([f"kquitapp{KDE_VERSION}", "plasmawindowed"])
        for _ in range(10):
            try:
                subprocess.check_call(["pidof", "plasmawindowed"])
            except subprocess.CalledProcessError:
                break
            time.sleep(1)
        cls.gtk_thread.quit()
        cls.spin_thread.quit_event.set()
        cls.driver.quit()

    def klipper_signal_handler(self, d_bus_proxy: Gio.DBusProxy, sender_name: str, signal_name: str, parameters: GLib.Variant):
        logging.info(f"received signal {signal_name}")
        if signal_name == "clipboardHistoryUpdated":
            self.klipper_updated_event.set()

    def test_0_open(self) -> None:
        """
        Tests the widget can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")
        self.driver.find_element(AppiumBy.NAME, "clipboard")

        self.assertEqual(self.klipper_proxy.getClipboardContents(), "Fushan Wen")
        self.klipper_proxy.connect("g-signal", self.klipper_signal_handler)

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
        self.assertTrue(self.klipper_updated_event.wait(5))
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
        self.assertTrue(self.klipper_updated_event.wait(5))

        self.klipper_updated_event.clear()
        item = self.driver.find_element(AppiumBy.NAME, "Fushan Wen")
        self.driver.find_element(AppiumBy.NAME, "Clear History").click()
        self.driver.find_element(AppiumBy.NAME, "Delete").click()
        WebDriverWait(self.driver, 5).until_not(lambda _: item.is_displayed())
        self.assertTrue(self.klipper_updated_event.wait(5))

    def test_3_dbus_interface(self) -> None:
        """
        D-Bus interface for Klipper
        """
        # setClipboardContents with a valid string
        clipboard_content = "setFromTest"
        self.klipper_proxy.setClipboardContents("(s)", clipboard_content)
        element = self.driver.find_element(AppiumBy.NAME, clipboard_content)
        self.assertTrue(self.klipper_updated_event.wait(5))

        # setClipboardContents with an empty string
        self.klipper_updated_event.clear()
        self.klipper_proxy.setClipboardContents("(s)", "")
        self.assertFalse(self.klipper_updated_event.wait(5))

        # clearClipboardHistory
        last_modified = os.stat(self.klipper_data_file).st_mtime
        self.klipper_updated_event.clear()
        self.klipper_proxy.clearClipboardHistory()
        WebDriverWait(self.driver, 5).until_not(lambda _: element.is_displayed())
        self.assertTrue(self.klipper_updated_event.wait(5))
        self.assertNotEqual(last_modified, os.stat(self.klipper_data_file).st_mtime)

        # saveClipboardHistory
        last_modified = os.stat(self.klipper_data_file).st_mtime
        self.klipper_proxy.setClipboardContents("(s)", clipboard_content)
        self.klipper_proxy.saveClipboardHistory()
        self.assertNotEqual(last_modified, os.stat(self.klipper_data_file).st_mtime)

        # History item
        self.assertEqual(self.klipper_proxy.getClipboardHistoryMenu(), [clipboard_content])
        self.assertEqual(self.klipper_proxy.getClipboardHistoryItem("(i)", 0), clipboard_content)
        self.assertEqual(self.klipper_proxy.getClipboardHistoryItem("(i)", 123), "")  # Invalid index

        self.spin_thread.quit_event.set()

    def update_config_and_restart_clipboard(self, group: str | list[str], key: str | list[str], new_value: str | list[str], reset_history: bool = False) -> None:
        subprocess.check_call([f"kquitapp{KDE_VERSION}", "plasmawindowed"])
        for _ in range(10):
            try:
                subprocess.check_call(["pidof", "plasmawindowed"])
            except subprocess.CalledProcessError:
                break
            time.sleep(1)
        self.driver.quit()

        if reset_history:
            shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "klipper/autotests/data/onetextentry.lst"), self.klipper_data_file)

        if isinstance(group, str):
            subprocess.check_call([f"kwriteconfig{KDE_VERSION}", "--file", "klipperrc", "--group", group, "--key", key, new_value])
        else:
            for g, k, v in zip(group, key, new_value):
                subprocess.check_call([f"kwriteconfig{KDE_VERSION}", "--file", "klipperrc", "--group", g, "--key", k, v])

        # Restart the widget to take effect
        self.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=self.appium_options)
        if reset_history:
            self.driver.find_element(AppiumBy.NAME, "Fushan Wen")

    def gtk_copy(self, content: Gdk.ContentProvider, clipboard_mode: int = 0) -> None:
        """
        0 is clipboard, 1 is selection.
        """
        window = Gtk.Window()
        window.set_default_size(20, 20)
        button = Gtk.Button(label="Copy Content")
        window.set_child(button)
        window.set_visible(True)
        SpinThread.spin()

        # Click the button to update the latest serial. See also:
        # https://invent.kde.org/plasma/kwin/-/commit/31018c000bbad5dc3b263b7f452b0795dd153ceb
        # https://github.com/GNOME/gtk/blob/7da4844dcc2fb2a35457fc4e251c504c8f3d0206/gdk/wayland/gdkseat-wayland.c#L4390
        ActionChains(self.driver).send_keys(Keys.SPACE).perform()
        SpinThread.spin()
        if clipboard_mode == 0:
            clipboard = window.get_display().get_clipboard()  # Clipboard
        else:
            clipboard = window.get_display().get_primary_clipboard()  # Selection
        self.assertTrue(clipboard.set_content(content))
        SpinThread.spin()

        window.set_visible(False)
        SpinThread.spin()

    def take_screenshot(self) -> str:
        with tempfile.TemporaryDirectory() as temp_dir:
            saved_image_path = os.path.join(temp_dir, "tray.png")
            self.driver.get_screenshot_as_file(saved_image_path)
            return base64.b64encode(Gdk.Texture.new_from_filename(saved_image_path).save_to_png_bytes().get_data()).decode()

    def test_4_bug487843_bug466414_empty_clip_crash(self) -> None:
        """
        When "Text selection - Always save in history" is enabled, a clip with empty text can crash klipper.
        @see https://bugs.kde.org/show_bug.cgi?id=487843
        @see https://bugs.kde.org/show_bug.cgi?id=466414
        """
        # Enable "Text selection - Always save in history" to test the two bugs
        self.update_config_and_restart_clipboard(["General"] * 2, ["IgnoreSelection", "SyncClipboards"], ["false", "true"], True)

        content_text = Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes("", "utf-8")))
        # Clip data from Firefox have additional mime types, which cause the crash
        content_application = Gdk.ContentProvider.new_for_bytes("application/whatever", GLib.Bytes.new(bytes("abc", "utf-8")))
        content_union = Gdk.ContentProvider.new_union([content_text, content_application])
        self.gtk_copy(content_union)
        self.driver.find_element(AppiumBy.NAME, "Fushan Wen")  # Still alive

        new_text = "Hello World"
        self.gtk_copy(Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes(new_text, "utf-8"))))
        # self.assertEqual(self.driver.get_clipboard_text(), new_text) Broken in CI
        self.driver.find_element(AppiumBy.NAME, new_text)  # Still alive

    def test_5_ignore_image(self) -> None:
        """
        When `IgnoreImages` is set to false, the clipboard should save images.
        """
        # Enable "Only when explicitly copied" to test the two bugs
        self.update_config_and_restart_clipboard(["General"] * 3, ["IgnoreImages", "IgnoreSelection", "SyncClipboards"], ["false", "true", "false"], True)

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

    def test_6_url_preview(self) -> None:
        """
        The PreviewImageProvider registers a custom image provider to load previews for URLs.
        """
        self.update_config_and_restart_clipboard(["General"] * 3, ["IgnoreImages", "IgnoreSelection", "SyncClipboards"], ["true", "true", "false"], True)
        new_text = "clip thin"
        pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 256, 256)
        with tempfile.TemporaryDirectory() as temp_dir:
            colors = (0xff0000ff, 0x00ff00ff, 0x0000ffff)
            for color in colors:
                pixbuf.fill(color)
                pixbuf.savev(os.path.join(temp_dir, f"{str(color)}.png"), "png")

            uri_list = '\r\n'.join([f"file://{os.path.join(temp_dir, path)}" for path in os.listdir(temp_dir)]) + '\r\n'
            content_urls = Gdk.ContentProvider.new_for_bytes("text/uri-list", GLib.Bytes.new(bytes(uri_list, "utf-8")))
            content_text = Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes(new_text, "utf-8")))
            content_union = Gdk.ContentProvider.new_union([content_text, content_urls])
            self.gtk_copy(content_union)  # URL has the highest priority

            partial_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 16, 16)
            for color in colors:
                partial_pixbuf.fill(color)
                partial_image = base64.b64encode(Gdk.Texture.new_for_pixbuf(partial_pixbuf).save_to_png_bytes().get_data()).decode()
                self.driver.find_image_occurrence(self.take_screenshot(), partial_image)

            self.assertRaises(NoSuchElementException, self.driver.find_element, AppiumBy.NAME, new_text)

    @unittest.expectedFailure  # https://invent.kde.org/plasma/kwin/-/commit/31018c000bbad5dc3b263b7f452b0795dd153ceb#note_1013530
    def test_7_sync_selection_with_ignore_selection(self) -> None:
        """
        When `SyncClipboards` is true but `IgnoreSelection` is true, the clipboard should still sync clipboard and selection.
        """
        self.update_config_and_restart_clipboard(["General"] * 2, ["IgnoreSelection", "SyncClipboards"], ["true", "true"], True)
        selected_text = "appiumtest123"
        content_text = Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes(selected_text, "utf-8")))
        self.gtk_copy(content_text, 1)
        self.assertEqual(self.driver.get_clipboard_text(), selected_text)
        self.assertRaises(NoSuchElementException, self.driver.find_element, AppiumBy.NAME, selected_text)

    def test_8_edit_page(self) -> None:
        """
        In edit mode, the text area should be focused by default.
        """
        self.update_config_and_restart_clipboard(["General"] * 2, ["IgnoreSelection", "SyncClipboards", "IgnoreImages"], ["true", "false", "true"], True)
        ActionChains(self.driver).send_keys(Keys.DOWN).send_keys(Keys.DOWN).perform()
        self.driver.find_element(AppiumBy.NAME, "Edit contents").click()
        self.driver.find_element(AppiumBy.NAME, "Text edit area")
        time.sleep(1)

        # By default the text area is focused, so typing anything will appear in the text area.
        new_text = "clip bold"
        ActionChains(self.driver).key_down(Keys.CONTROL).send_keys("a").key_up(Keys.CONTROL).perform()  # Select all
        ActionChains(self.driver).key_down(Keys.CONTROL).send_keys("a").key_up(Keys.CONTROL).perform()  # Perform twice to make it less flaky
        ActionChains(self.driver).send_keys(new_text).pause(1).perform()
        ActionChains(self.driver).key_down(Keys.CONTROL).send_keys("s").key_up(Keys.CONTROL).perform()  # Save
        self.driver.find_element(AppiumBy.NAME, new_text)
        # self.assertEqual(self.driver.get_clipboard_text(), new_text)

    def test_9_bug491488_copy_cells(self) -> None:
        """
        A cell has both image data and text data, which should not be ignored when images are ignored.
        """
        self.update_config_and_restart_clipboard(["General"] * 3, ["IgnoreSelection", "SyncClipboards", "IgnoreImages"], ["true", "false", "true"], True)
        new_text = "clip thin"
        pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 256, 256)
        pixbuf.fill(0xff0000ff)
        content_image = Gdk.ContentProvider.new_for_bytes("image/png", Gdk.Texture.new_for_pixbuf(pixbuf).save_to_png_bytes())
        content_text = Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes(new_text, "utf-8")))
        content_union = Gdk.ContentProvider.new_union([content_text, content_image])
        self.gtk_copy(content_union)
        self.driver.find_element(AppiumBy.NAME, new_text)

    def test_9_bug492170_disable_history_across_session(self) -> None:
        """
        Clips should not be saved across desktop sessions when "Save history across desktop sessions" is disabled.
        """
        self.update_config_and_restart_clipboard("General", "KeepClipboardContents", "false", False)
        self.assertRaises(NoSuchElementException, self.driver.find_element, AppiumBy.NAME, "Fushan Wen")


if __name__ == '__main__':
    assert subprocess.call(["pidof", "plasmashell"]) != 0, "The test requires plasmashell to quit"
    logging.getLogger().setLevel(logging.INFO)
    unittest.main()
