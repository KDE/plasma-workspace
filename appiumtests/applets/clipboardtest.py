#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import logging
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from typing import Final

import gi
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.common.exceptions import NoSuchElementException, WebDriverException
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.actions.action_builder import ActionBuilder
from selenium.webdriver.common.actions.interaction import POINTER_MOUSE
from selenium.webdriver.common.actions.mouse_button import MouseButton
from selenium.webdriver.common.actions.pointer_input import PointerInput
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support.ui import WebDriverWait

gi.require_version('Gdk', '4.0')
gi.require_version('GdkPixbuf', '2.0')
gi.require_version('Gtk', '4.0')
from gi.repository import Gdk, GdkPixbuf, Gio, GLib, Gtk

WIDGET_ID: Final = "org.kde.plasma.clipboard"
KDE_VERSION: Final = 6


class TestApplication(Gtk.Application):

    def __init__(self) -> None:
        super().__init__(application_id="org.kde.plasma.clipboardtest")

        # Create history file to suppress warnings
        klipper_folder = os.path.join(GLib.get_user_data_dir(), "klipper")
        self.klipper_data_file = os.path.join(klipper_folder, "history3.sqlite")
        assert not os.path.exists(self.klipper_data_file)
        shutil.copytree(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "klipper/autotests/data/onetextentry"), klipper_folder)

        self.driver: webdriver.Remote
        self.driver_ready_event = threading.Event()
        self.connect("activate", self.on_activate)

        self.klipper_proxy = Gio.DBusProxy.new_for_bus_sync(Gio.BusType.SESSION, 0, None, "org.kde.klipper", "/klipper", "org.kde.klipper.klipper")
        self.klipper_updated_event = threading.Event()
        self.klipper_proxy.connect("g-signal", self.klipper_signal_handler)

        GLib.timeout_add_seconds(180, self.quit)

    def on_activate(self, _app: Gtk.Application) -> None:
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability(
            "environ",
            {
                "LC_ALL": "en_US.UTF-8",
                "QT_FATAL_WARNINGS": "0",  # DataControlOffer: timeout reading from pipe
                "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;qt.qml.typeresolution.cycle.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false;org.kde.klipper.debug=true",
            })
        options.set_capability("timeouts", {'implicit': 5000})
        self.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)
        self.driver_ready_event.set()

    def klipper_signal_handler(self, d_bus_proxy: Gio.DBusProxy, sender_name: str, signal_name: str, parameters: GLib.Variant) -> None:
        logging.info(f"received signal {signal_name}")
        if signal_name == "clipboardHistoryUpdated":
            self.klipper_updated_event.set()

    @staticmethod
    def spin() -> None:
        context = GLib.MainContext.default()
        count = 0
        while context.pending() or count < 10:
            count += 1
            if not context.pending():
                time.sleep(0.1)
                continue
            context.iteration(may_block=True)

    @staticmethod
    def spin_until(event: threading.Event) -> None:
        context = GLib.MainContext.default()
        while not event.is_set() or context.pending():
            if not context.pending():
                time.sleep(0.1)
                continue
            context.iteration(may_block=True)

    def gtk_copy(self, content_provider: Gdk.ContentProvider, clipboard_mode: int = 0) -> None:
        window = Gtk.Window()
        window.set_default_size(20, 20)
        button = Gtk.Button(label="Copy Content")
        window.set_child(button)
        window.set_visible(True)
        self.spin()

        # Click the button to update the latest serial. See also:
        # https://invent.kde.org/plasma/kwin/-/commit/31018c000bbad5dc3b263b7f452b0795dd153ceb
        # https://github.com/GNOME/gtk/blob/7da4844dcc2fb2a35457fc4e251c504c8f3d0206/gdk/wayland/gdkseat-wayland.c#L4390
        ActionChains(self.driver).send_keys(Keys.SPACE).perform()
        self.spin()
        if clipboard_mode == 0:
            clipboard = window.get_display().get_clipboard()  # Clipboard
        else:
            clipboard = window.get_display().get_primary_clipboard()  # Selection
        assert clipboard.set_content(content_provider)
        self.spin()

        window.set_visible(False)
        self.spin()

    def gtk_get_clipboard_mime_data(self, clipboard_mode: int = 0) -> dict[str, GLib.Bytes]:
        window = Gtk.Window()
        window.set_default_size(20, 20)
        button = Gtk.Button(label="Get Content")
        window.set_child(button)
        window.set_visible(True)
        self.spin()

        if clipboard_mode == 0:
            clipboard = window.get_display().get_clipboard()  # Clipboard
        else:
            clipboard = window.get_display().get_primary_clipboard()  # Selection
        self.spin()

        data: dict[str, GLib.Bytes] = {}
        read_event = threading.Event()

        def on_value_read(_clipboard: Gdk.Clipboard, result: Gio.AsyncResult, user_data) -> None:
            nonlocal data, read_event
            stream, t = _clipboard.read_finish(result)
            logging.info(f"reading data for {t}")
            data[t] = stream.read_bytes(10000, None)
            logging.info(f"reading done")
            read_event.set()

        for t in clipboard.get_formats().get_mime_types():
            if t.startswith("image/") and not t.endswith("png"):
                continue
            if t.startswith("application/x-qt-image"):
                continue
            ActionChains(self.driver).send_keys(Keys.SPACE).perform()
            clipboard.read_async([t, None], GLib.PRIORITY_DEFAULT, None, on_value_read, None)
            self.spin_until(read_event)
            read_event.clear()

        window.set_visible(False)
        self.spin()

        return data


class ClipboardTest(unittest.TestCase):
    """
    Tests for the clipboard widget
    """

    @classmethod
    def setUpClass(cls) -> None:
        app.driver_ready_event.wait(10)

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            app.driver.get_screenshot_as_file(f"failed_test_shot_{WIDGET_ID}_#{self.id()}.png")

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
        app.driver.quit()
        app.quit()

    def test_0_open(self) -> None:
        """
        Tests the widget can be opened and the history can be loaded
        """
        app.driver.find_element(AppiumBy.NAME, "Fushan Wen")
        app.driver.find_element(AppiumBy.NAME, "clipboard")

        self.assertEqual(app.klipper_proxy.getClipboardContents(), "Fushan Wen")

    def test_1_barcode(self) -> None:
        """
        Tests the barcode page can be opened
        """
        actions = ActionChains(app.driver)
        actions.send_keys(Keys.DOWN).perform()
        # Wait until the first item is selected
        try:
            app.driver.find_element(AppiumBy.NAME, "Show QR code").click()
        except NoSuchElementException:
            actions.send_keys(Keys.DOWN).perform()  # Try pressing down key again
            app.driver.find_element(AppiumBy.NAME, "Show QR code").click()

        app.driver.find_element(AppiumBy.NAME, "QR Code")
        app.driver.find_element(AppiumBy.NAME, "Return to Clipboard")
        app.driver.find_element(AppiumBy.NAME, "Change the QR code type")

        # Opens the barcode type menu and changes the current barcode type
        app.driver.find_element(AppiumBy.NAME, "Change the QR code type").click()
        menu_item = app.driver.find_element(AppiumBy.NAME, "Aztec")
        # Switch to Aztec
        actions = ActionChains(app.driver)
        for _ in range(3):
            actions.send_keys(Keys.DOWN).perform()
            time.sleep(1)
        actions.send_keys(Keys.SPACE).perform()
        WebDriverWait(app.driver, 5).until_not(lambda _: menu_item.is_displayed())
        app.driver.find_element(AppiumBy.NAME, "Aztec")  # This is from barcodeItem

        # Go back to the list from the barcode page
        button_item = app.driver.find_element(AppiumBy.NAME, "Return to Clipboard")
        self.assertTrue(button_item.is_displayed())
        button_item.click()
        app.driver.find_element(AppiumBy.NAME, "Fushan Wen")
        self.assertFalse(button_item.is_displayed())

    def test_2_list_1_bug475696(self) -> None:
        """
        Pressing Return on an item should trigger the copy action
        @see https://bugs.kde.org/show_bug.cgi?id=475696
        """
        ActionChains(app.driver).send_keys(Keys.TAB).send_keys(Keys.DOWN).pause(0.5).perform()
        app.driver.find_element(AppiumBy.NAME, "Show QR code")
        ActionChains(app.driver).send_keys(Keys.RETURN).perform()
        self.assertEqual(app.driver.get_clipboard_text(), "clipboard")
        ActionChains(app.driver).send_keys(Keys.ENTER).perform()
        self.assertEqual(app.driver.get_clipboard_text(), "Fushan Wen")
        ActionChains(app.driver).send_keys(Keys.SPACE).perform()
        self.assertEqual(app.driver.get_clipboard_text(), "clipboard")

    def test_2_list_2_delete(self) -> None:
        """
        Deletes the top item and tests if the current clipboard changes
        @see https://bugs.kde.org/show_bug.cgi?id=475696
        """
        # Now "clipboard" is the first item
        ActionChains(app.driver).send_keys(Keys.UP).perform()
        app.driver.find_element(AppiumBy.NAME, "Remove from history").click()
        # The first item becomes the current clipboard item
        self.assertEqual(app.driver.get_clipboard_text(), "Fushan Wen")

        item = app.driver.find_element(AppiumBy.NAME, "Fushan Wen")
        app.driver.find_element(AppiumBy.NAME, "Clear History").click()
        app.driver.find_element(AppiumBy.NAME, "Delete").click()
        WebDriverWait(app.driver, 5).until_not(lambda _: item.is_displayed())

    def test_2_list_3_edit(self) -> None:
        """
        In edit mode, the text area should be focused by default.
        """
        app.klipper_proxy.setClipboardContents("(s)", "clip thin")
        app.klipper_proxy.setClipboardContents("(s)", "clip medium")
        app.driver.find_element(AppiumBy.NAME, "clip medium")

        ActionChains(app.driver).send_keys(Keys.DOWN).send_keys(Keys.DOWN).perform()
        app.driver.find_element(AppiumBy.NAME, "Edit contents").click()
        app.driver.find_element(AppiumBy.NAME, "Text edit area")
        time.sleep(1)

        # By default the text area is focused, so typing anything will appear in the text area.
        new_text = "clip bold"
        ActionChains(app.driver).key_down(Keys.CONTROL).send_keys("a").key_up(Keys.CONTROL).perform()  # Select all
        ActionChains(app.driver).key_down(Keys.CONTROL).send_keys("a").key_up(Keys.CONTROL).perform()  # Perform twice to make it less flaky
        ActionChains(app.driver).send_keys(new_text).pause(1).perform()
        ActionChains(app.driver).key_down(Keys.CONTROL).send_keys("s").key_up(Keys.CONTROL).perform()  # Save
        app.driver.find_element(AppiumBy.NAME, new_text)
        self.assertEqual(app.driver.get_clipboard_text(), new_text)

        # BUG 494145: update uuid after editing so the item can be removed
        delete_button = app.driver.find_element(AppiumBy.NAME, "Remove from history")
        delete_button.click()
        WebDriverWait(app.driver, 5).until_not(lambda _: delete_button.is_displayed())
        self.assertNotEqual(app.driver.get_clipboard_text(), new_text)

    def test_3_dbus_interface(self) -> None:
        """
        D-Bus interface for Klipper
        """
        app.klipper_updated_event.clear()
        # setClipboardContents with a valid string
        clipboard_content = "setFromTest"
        app.klipper_proxy.setClipboardContents("(s)", clipboard_content)
        element = app.driver.find_element(AppiumBy.NAME, clipboard_content)
        app.spin()
        self.assertTrue(app.klipper_updated_event.is_set())

        # setClipboardContents with an empty string
        app.klipper_updated_event.clear()
        app.klipper_proxy.setClipboardContents("(s)", "")
        app.spin()
        self.assertFalse(app.klipper_updated_event.is_set())

        # clearClipboardHistory
        app.klipper_updated_event.clear()
        last_modified = os.stat(app.klipper_data_file).st_mtime
        app.klipper_proxy.clearClipboardHistory()
        app.klipper_proxy.saveClipboardHistory()
        app.spin()
        self.assertTrue(app.klipper_updated_event.is_set())
        self.assertFalse(element.is_displayed())
        self.assertNotEqual(last_modified, os.stat(app.klipper_data_file).st_mtime)

        # saveClipboardHistory
        last_modified = os.stat(app.klipper_data_file).st_mtime
        app.klipper_proxy.setClipboardContents("(s)", clipboard_content)
        app.klipper_proxy.saveClipboardHistory()
        self.assertNotEqual(last_modified, os.stat(app.klipper_data_file).st_mtime)

        # History item
        self.assertEqual(app.klipper_proxy.getClipboardHistoryMenu(), [clipboard_content])
        self.assertEqual(app.klipper_proxy.getClipboardHistoryItem("(i)", 0), clipboard_content)
        self.assertEqual(app.klipper_proxy.getClipboardHistoryItem("(i)", 123), "")  # Invalid index

    def update_config(self, group: str | list[str], key: str | list[str], new_value: str | list[str]) -> None:
        if isinstance(group, str):
            subprocess.check_call([f"kwriteconfig{KDE_VERSION}", "--file", "klipperrc", "--group", group, "--key", key, new_value])
        else:
            for g, k, v in zip(group, key, new_value):
                subprocess.check_call([f"kwriteconfig{KDE_VERSION}", "--file", "klipperrc", "--group", g, "--key", k, v])

        app.klipper_proxy.reloadConfig()

    def take_screenshot(self) -> str:
        with tempfile.TemporaryDirectory() as temp_dir:
            saved_image_path = os.path.join(temp_dir, "tray.png")
            app.driver.get_screenshot_as_file(saved_image_path)
            return base64.b64encode(Gdk.Texture.new_from_filename(saved_image_path).save_to_png_bytes().get_data()).decode()

    def test_4_url_preview(self) -> None:
        """
        The PreviewImageProvider registers a custom image provider to load previews for URLs.
        """
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
            app.gtk_copy(content_union)  # URL has the highest priority

            time.sleep(1)  # Asynchronous loading
            partial_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 16, 16)
            for color in colors:
                partial_pixbuf.fill(color)
                partial_image = base64.b64encode(Gdk.Texture.new_for_pixbuf(partial_pixbuf).save_to_png_bytes().get_data()).decode()
                try:
                    app.driver.find_image_occurrence(self.take_screenshot(), partial_image)
                except WebDriverException:  # Flaky
                    time.sleep(1)
                    app.driver.find_image_occurrence(self.take_screenshot(), partial_image)

            self.assertRaises(NoSuchElementException, app.driver.find_element, AppiumBy.NAME, new_text)

    def test_5_1_bug491488_copy_cells(self) -> None:
        """
        A cell has both image data and text data, which should not be ignored when images are ignored.
        """
        new_text = "clip thin"
        pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 256, 256)
        pixbuf.fill(0xff0000ff)
        content_image = Gdk.ContentProvider.new_for_bytes("image/png", Gdk.Texture.new_for_pixbuf(pixbuf).save_to_png_bytes())
        content_text = Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes(new_text, "utf-8")))
        content_union = Gdk.ContentProvider.new_union([content_text, content_image])
        app.gtk_copy(content_union)
        app.driver.find_element(AppiumBy.NAME, new_text)

    def test_5_2_bug496331_secret_password_hint(self) -> None:
        """
        When a mimedata has "x-kde-passwordManagerHint" set to "secret", the clip should not be saved to history.
        """
        process = subprocess.Popen(["python3", os.path.join(os.path.dirname(os.path.abspath(__file__)), "clipboardtest", "copysecret.py")], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.addCleanup(process.kill)
        process.stderr.readline()
        process.stderr.readline()
        ActionChains(app.driver).send_keys(Keys.SPACE).perform()
        app.driver.find_element(AppiumBy.NAME, "123456789")
        self.assertRaises(NoSuchElementException, app.driver.find_element, AppiumBy.NAME, "123456789test")

    def test_5_3_bug491961_mimetypes(self) -> None:
        """
        Klipper's history should preserve more MIME types
        """
        app.klipper_proxy.clearClipboardHistory()

        old_text = "bug491961 appium test"
        app.gtk_copy(Gdk.ContentProvider.new_for_bytes("text/plain;charset=utf-8", GLib.Bytes.new(bytes(old_text, "utf-8"))))

        pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 1, 1)  # 1x1 pixel
        pixbuf.fill(0xff0000ff)
        content_image = Gdk.ContentProvider.new_for_bytes("image/png", Gdk.Texture.new_for_pixbuf(pixbuf).save_to_png_bytes())
        text_data = GLib.Bytes.new(bytes("bug491961", "utf-8"))
        content_text = Gdk.ContentProvider.new_for_bytes("text/plain;charset=utf-8", text_data)
        temp_file = tempfile.NamedTemporaryFile(suffix=".txt")
        urls_data = GLib.Bytes.new(bytes(f"file://{temp_file.name}\r\n", "utf-8"))
        content_urls = Gdk.ContentProvider.new_for_bytes("text/uri-list", urls_data)
        content_application = Gdk.ContentProvider.new_for_bytes("application/x-kde-appiumtest", GLib.Bytes.new(bytes("abcdefg", "utf-8")))
        content_union = Gdk.ContentProvider.new_union([content_text, content_image, content_urls, content_application])
        app.gtk_copy(content_union)
        app.driver.find_element(AppiumBy.NAME, f"file://{temp_file.name}")

        actions = ActionChains(app.driver)
        actions.send_keys(Keys.DOWN).send_keys(Keys.END).perform()
        try:
            app.driver.find_element(AppiumBy.XPATH, f"//list_item[@name='{old_text}' and contains(@states, 'focused')]")
        except NoSuchElementException:
            actions.send_keys(Keys.DOWN).send_keys(Keys.END).perform()
        actions.send_keys(Keys.RETURN).perform()
        self.assertNotIn("image/png", app.gtk_get_clipboard_mime_data())

        app.driver.find_element(AppiumBy.XPATH, f"//list_item[@name='file://{temp_file.name}' and contains(@states, 'focused')]")
        actions.send_keys(Keys.RETURN).perform()

        def check_clipboard() -> None:
            mime_data = app.gtk_get_clipboard_mime_data()
            self.assertEqual(mime_data["text/plain;charset=utf-8"].get_data(), text_data.get_data())
            self.assertEqual(mime_data["text/uri-list"].get_data(), urls_data.get_data())
            self.assertIn("application/x-kde-appiumtest", mime_data)
            with tempfile.NamedTemporaryFile(mode="wb", suffix=".png") as temp_file:
                temp_file.write(mime_data["image/png"].get_data())
                temp_file.flush()
                image_from_clipboard = GdkPixbuf.Pixbuf.new_from_file(temp_file.name)
                self.assertEqual(image_from_clipboard.get_pixels(), pixbuf.get_pixels())

        try:
            check_clipboard()
        except AssertionError:
            time.sleep(1)
            check_clipboard()

    def test_6_bug487843_bug466414_empty_clip_crash(self) -> None:
        """
        When "Text selection - Always save in history" is enabled, a clip with empty text can crash klipper.
        @see https://bugs.kde.org/show_bug.cgi?id=487843
        @see https://bugs.kde.org/show_bug.cgi?id=466414
        """
        # Enable "Text selection - Always save in history" to test the two bugs
        self.update_config(["General"] * 2, ["IgnoreSelection", "SyncClipboards"], ["false", "true"])

        content_text = Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes("", "utf-8")))
        # Clip data from Firefox have additional mime types, which cause the crash
        content_application = Gdk.ContentProvider.new_for_bytes("application/whatever", GLib.Bytes.new(bytes("abc", "utf-8")))
        content_union = Gdk.ContentProvider.new_union([content_text, content_application])
        app.gtk_copy(content_union)

        new_text = "Hello World"
        app.gtk_copy(Gdk.ContentProvider.new_for_bytes("text/plain", GLib.Bytes.new(bytes(new_text, "utf-8"))))
        # self.assertEqual(app.driver.get_clipboard_text(), new_text) Broken in CI
        app.driver.find_element(AppiumBy.NAME, new_text)  # Still alive

    def test_7_ignore_image(self) -> None:
        """
        When `IgnoreImages` is set to false, the clipboard should save images.
        """
        # Enable "Only when explicitly copied" to test the two bugs
        self.update_config(["General"] * 3, ["IgnoreImages", "IgnoreSelection", "SyncClipboards"], ["false", "true", "false"])

        app.klipper_proxy.clearClipboardHistory()
        self.assertEqual(0, len(app.klipper_proxy.getClipboardHistoryMenu()))
        memory_usage_before = int(subprocess.check_output(["ps", "-o", "rss", "-C", "plasmawindowed"]).decode("utf-8").strip().split("\n")[1])

        # Copy 3 color blocks to clipboard
        colors = (0xff0000ff, 0x00ff00ff, 0x0000ffff)
        process = subprocess.Popen(["python3", os.path.join(os.path.dirname(os.path.abspath(__file__)), "clipboardtest", "bug497735_simultaneous_clipboard_requests.py"), "-w", "1000", "-h", "1000"], stdout=sys.stderr, stderr=subprocess.PIPE)
        self.addCleanup(process.kill)
        assert process.stderr is not None
        process.stderr.readline()  # From resizeEvent
        process.stderr.readline()  # From resizeEvent
        action = ActionBuilder(app.driver, mouse=PointerInput(POINTER_MOUSE, "mouse"))
        actions = action.pointer_action.move_to_location(100, 100)
        for _ in range(len(colors)):
            actions = actions.click(None, MouseButton.LEFT).pause(0.1)
        action.perform()
        process.stderr.readline()
        process.stderr.readline()
        process.stderr.readline()
        try:
            self.assertEqual(len(colors), len(app.klipper_proxy.getClipboardHistoryMenu()))
        except AssertionError:  # Flaky
            time.sleep(1)
            self.assertEqual(len(colors), len(app.klipper_proxy.getClipboardHistoryMenu()))

        for color in colors:
            # Match the color block in the history
            partial_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 16, 16)
            partial_pixbuf.fill(color)
            partial_image = base64.b64encode(Gdk.Texture.new_for_pixbuf(partial_pixbuf).save_to_png_bytes().get_data()).decode()
            try:
                app.driver.find_image_occurrence(self.take_screenshot(), partial_image)
            except WebDriverException:  # Flaky
                time.sleep(1)
                app.driver.find_image_occurrence(self.take_screenshot(), partial_image)

        memory_usage_after = int(subprocess.check_output(["ps", "-o", "rss", "-C", "plasmawindowed"]).decode("utf-8").strip().split("\n")[1])
        logging.info(f"before: {memory_usage_before} after: {memory_usage_after} diff: {memory_usage_after - memory_usage_before}")

    def test_8_bug492170_disable_history_across_session(self) -> None:
        """
        Clips should not be saved across desktop sessions when "Save history across desktop sessions" is disabled.
        """
        app.klipper_proxy.setClipboardContents("(s)", "appiumtest")
        app.driver.find_element(AppiumBy.NAME, "appiumtest")
        self.update_config("General", "KeepClipboardContents", "false")
        app.driver.find_element(AppiumBy.NAME, "Clipboard is empty")


if __name__ == '__main__':
    assert subprocess.call(["pidof", "plasmashell"]) != 0, "The test requires plasmashell to quit"
    logging.getLogger().setLevel(logging.INFO)
    app = TestApplication()
    app.run(None)
    unittest.main(failfast=True)
