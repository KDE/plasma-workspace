#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import logging
import os
import queue
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from datetime import date
from time import sleep
from typing import IO, Final

import cv2 as cv
import gi
import numpy as np

gi.require_version("Gdk", "3.0")  # StatusIcon is removed in 4
gi.require_version("Gtk", "3.0")  # StatusIcon is removed in 4
gi.require_version('GdkPixbuf', '2.0')

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gdk, GdkPixbuf, Gio, GLib, Gtk
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.actions.action_builder import ActionBuilder
from selenium.webdriver.common.actions.interaction import POINTER_MOUSE
from selenium.webdriver.common.actions.mouse_button import MouseButton
from selenium.webdriver.common.actions.pointer_input import PointerInput
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

KDE_VERSION: Final = 6
WIDGET_ID: Final = "org.kde.plasma.systemtray"
CMAKE_RUNTIME_OUTPUT_DIRECTORY: Final = os.getenv("CMAKE_RUNTIME_OUTPUT_DIRECTORY", os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "build", "bin"))


def generate_color_block(red: int, green: int, blue: int) -> str:
    cv_second_image = np.zeros((10, 10, 3), dtype=np.uint8)
    cv_second_image[:, :] = [blue, green, red]
    return base64.b64encode(cv.imencode('.png', cv_second_image)[1].tobytes()).decode()


class XEmbedTrayIcon(threading.Thread):
    """
    XEmbed tray icon implementation using Gtk.StatusIcon
    """

    def __init__(self, title: str) -> None:
        super().__init__()

        GLib.timeout_add_seconds(300, Gtk.main_quit)  # Failsafe

        # Red square
        self.__status_icon: Gtk.StatusIcon = Gtk.StatusIcon(title=title)
        pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 16, 16)
        pixbuf.fill(0xaa0000ff)  # 170, 0, 0
        self.__status_icon.set_from_pixbuf(pixbuf)

        self.__status_icon.connect("button-press-event", self._on_button_press_event)
        self.button_press_event = threading.Event()
        self.pressed_button: int = -1
        self.__status_icon.connect("button-release-event", self._on_button_release_event)
        self.button_release_event = threading.Event()
        self.__status_icon.connect("popup-menu", self._on_popup_menu)
        self.popup_menu_event = threading.Event()
        self.scroll_event = threading.Event()
        self.__status_icon.connect("scroll-event", self._on_scroll_event)

    def run(self) -> None:
        Gtk.main()

    def quit(self) -> None:
        Gtk.main_quit()

    def reset_events(self) -> None:
        """
        Reset all threading events
        """
        self.button_press_event.clear()
        self.pressed_button = -1
        self.button_release_event.clear()
        self.popup_menu_event.clear()
        self.scroll_event.clear()

    def _on_button_press_event(self, status_icon: Gtk.StatusIcon, button_event: Gdk.EventButton) -> None:
        logging.info(f"button-press-event {button_event.button}")
        self.pressed_button = button_event.button
        self.button_press_event.set()

    def _on_button_release_event(self, status_icon: Gtk.StatusIcon, button_event: Gdk.EventButton) -> None:
        logging.info(f"button-release-event {button_event.button}")
        self.button_release_event.set()

    def _on_popup_menu(self, status_icon: Gtk.StatusIcon, button: int, activate_time: int) -> None:
        logging.info(f"popup-menu {button} {activate_time}")
        self.popup_menu_event.set()

    def _on_scroll_event(self, status_icon, scroll_event: Gdk.EventScroll) -> None:
        logging.info(f"scroll-event {scroll_event.delta_x} {scroll_event.delta_y} {int(scroll_event.direction)}")
        self.scroll_event.set()


class StreamReaderThread(threading.Thread):
    """
    Non-blocking readline thread
    """

    def __init__(self, stream: IO[bytes]) -> None:
        """
        @param stream: the stream to read from
        """
        self.__stream: IO[bytes] = stream
        self.__queue = queue.Queue()

        self.__stop_event = threading.Event()

        # Create the thread
        super().__init__()

    def run(self) -> None:
        """
        Collects lines from the source stream and put them in the queue.
        """
        while self.__stream.readable() and not self.__stop_event.is_set():
            line_str: str = self.__stream.readline().decode(encoding="utf-8")
            if "Received click" in line_str:
                self.__queue.put(line_str)
            elif len(line_str) == 0:
                break

    def stop(self) -> None:
        """
        Stops the thread
        """
        self.__stop_event.set()

    def readline(self) -> str | None:
        """
        Non-blocking readline
        The default timeout is 5s.
        """
        try:
            return self.__queue.get(block=True, timeout=5)
        except queue.Empty:
            return None


class SystemTrayTests(unittest.TestCase):
    """
    Tests for the system tray widget
    """

    driver: webdriver.Remote
    xembedsniproxy: subprocess.Popen[bytes] | None = None
    kded: subprocess.Popen[bytes] | None = None
    xembed_tray_icon: XEmbedTrayIcon | None = None
    stream_reader_thread: StreamReaderThread | None = None

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
        })
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

        cls.kded = subprocess.Popen([f"kded{KDE_VERSION}"])
        # Doc: https://lazka.github.io/pgi-docs/Gio-2.0/classes/DBusConnection.html
        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
        SERVICE_NAME: Final = "org.freedesktop.DBus"
        OBJECT_PATH: Final = "/"
        INTERFACE_NAME: Final = SERVICE_NAME
        message: Gio.DBusMessage = Gio.DBusMessage.new_method_call(SERVICE_NAME, OBJECT_PATH, INTERFACE_NAME, "NameHasOwner")
        message.set_body(GLib.Variant("(s)", [f"org.kde.kded{KDE_VERSION}"]))
        for _ in range(5):
            reply, _ = session_bus.send_message_with_reply_sync(message, Gio.DBusSendMessageFlags.NONE, 1000)

            if reply and reply.get_signature() == 'b' and reply.get_body().get_child_value(0).get_boolean():
                break
            print(f"waiting for kded to appear on the dbus session", file=sys.stderr, flush=True)
            sleep(1)

        kded_reply: GLib.Variant = session_bus.call_sync(f"org.kde.kded{KDE_VERSION}", "/kded", f"org.kde.kded{KDE_VERSION}", "loadModule", GLib.Variant("(s)", [f"statusnotifierwatcher"]), GLib.VariantType("(b)"), Gio.DBusSendMessageFlags.NONE, 1000)
        assert kded_reply.get_child_value(0).get_boolean(), "Module is not loaded"

    def setUp(self) -> None:
        pass

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            if os.environ.get("TEST_WITH_KWIN_WAYLAND", "1") == "0":
                subprocess.check_call(["import", "-window", "root", f"failed_test_shot_systemtraytest_#{self.id()}.png"])
            else:
                self.driver.get_screenshot_as_file(f"failed_test_shot_systemtraytest_#{self.id()}.png")

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

        if cls.kded is not None:
            subprocess.check_call([f"kquitapp{KDE_VERSION}", f"kded{KDE_VERSION}"])
            cls.kded.wait(5)
        cls.driver.quit()

    def take_screenshot(self) -> str:
        """
        Take screenshot of the current screen and use png+base64 to encode the image
        """
        with tempfile.TemporaryDirectory() as temp_dir:
            saved_image_path = os.path.join(temp_dir, "tray.png")
            if os.getenv("TEST_WITH_KWIN_WAYLAND", "1") == "0":
                subprocess.check_call(["import", "-window", "root", saved_image_path])
            else:
                self.driver.get_screenshot_as_file(saved_image_path)
            cv_image = cv.imread(saved_image_path, cv.IMREAD_COLOR)
        return base64.b64encode(cv.imencode('.png', cv_image)[1].tobytes()).decode()

    def cleanup_xembed_tray_icon(self) -> None:
        """
        Cleanup function for test_xembed_tray_icon
        """
        if self.xembed_tray_icon is not None:
            self.xembed_tray_icon.quit()
            self.xembed_tray_icon = None

        if self.xembedsniproxy is not None:
            subprocess.check_call([f"kquitapp{KDE_VERSION}", "xembedsniproxy"])
            self.xembedsniproxy.wait(5)
            self.xembedsniproxy = None

        if self.stream_reader_thread is not None and self.stream_reader_thread.is_alive():
            self.stream_reader_thread.stop()
            self.stream_reader_thread = None

    def click_center(self, rect: dict[str, int], mouse_button: int = MouseButton.LEFT) -> None:
        """
        Move the mouse to the center of the area and click
        """
        if self.xembed_tray_icon is not None:
            self.xembed_tray_icon.reset_events()
        action = ActionBuilder(self.driver, mouse=PointerInput(POINTER_MOUSE, "mouse"))
        action.pointer_action.move_to_location(int(rect["x"] + rect["width"] / 2), int(rect["y"] + rect["height"] / 2)).click(None, mouse_button).pause(1)
        action.perform()
        if self.xembed_tray_icon is None:
            return
        self.assertTrue(self.xembed_tray_icon.button_press_event.is_set())
        self.assertTrue(self.xembed_tray_icon.button_release_event.is_set())
        # the button which was pressed or released, numbered from 1 to 5. Normally button 1 is the left mouse button, 2 is the middle button, and 3 is the right button. On 2-button mice, the middle button can often be simulated by pressing both mouse buttons together.
        if mouse_button == MouseButton.LEFT:
            self.assertEqual(self.xembed_tray_icon.pressed_button, 1)
        elif mouse_button == MouseButton.MIDDLE:
            self.assertEqual(self.xembed_tray_icon.pressed_button, 2)
        elif mouse_button == MouseButton.RIGHT:
            self.assertEqual(self.xembed_tray_icon.pressed_button, 3)

    def scroll_center(self, rect: dict[str, int], delta_x: float, delta_y: float) -> None:
        """
        Move the mouse to the center of the area and scroll
        """
        if self.xembed_tray_icon is not None:
            self.xembed_tray_icon.reset_events()
        action = ActionBuilder(self.driver)
        action.wheel_action.scroll(int(rect["x"] + rect["width"] / 2), int(rect["y"] + rect["height"] / 2), delta_x, delta_y).pause(1)
        action.perform()
        if self.xembed_tray_icon is not None:
            self.assertTrue(self.xembed_tray_icon.scroll_event.is_set())

    def test_1_xembed_tray_icon(self) -> None:
        """
        Tests XEmbed tray icons can be listed and clicked in the tray.

        @note GTK doesn't like send_events and double checks the mouse position
        matches where the window is and is top level, so match the debug
        output from xembedsniproxy instead.
        """
        self.addCleanup(self.cleanup_xembed_tray_icon)

        debug_env: dict[str, str] = os.environ.copy()
        debug_env["QT_LOGGING_RULES"] = "kde.xembedsniproxy.debug=true"
        debug_env["XDG_SESSION_TYPE"] = "x11" if os.getenv("TEST_WITH_KWIN_WAYLAND", "1") == "0" else "wayland"
        self.xembedsniproxy = subprocess.Popen([os.path.join(CMAKE_RUNTIME_OUTPUT_DIRECTORY, "xembedsniproxy"), '--platform', 'xcb'], env=debug_env, stderr=subprocess.PIPE)  # For debug output
        if not self.xembedsniproxy.stderr or self.xembedsniproxy.poll() != None:
            self.fail("xembedsniproxy is not available")
        print(f"xembedsniproxy PID: {self.xembedsniproxy.pid}", file=sys.stderr, flush=True)

        title: str = f"XEmbed Status Icon Test {date.today().strftime('%Y%m%d')}"
        self.xembed_tray_icon = XEmbedTrayIcon(title)
        self.xembed_tray_icon.start()

        wait: WebDriverWait = WebDriverWait(self.driver, 10)
        try:
            # FocusScope in StatusNotifierItem.qml
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, title)))
        except TimeoutException:
            self.fail(f"Cannot find the XEmbed icon in the system tray: {self.xembedsniproxy.stderr.readlines()}")

        # Create a reader thread to work around read block
        self.stream_reader_thread = StreamReaderThread(self.xembedsniproxy.stderr)
        self.stream_reader_thread.start()
        self.assertTrue(self.stream_reader_thread.is_alive(), "The reader thread is not running")

        # Now test clickable
        self.driver.find_element(AppiumBy.NAME, title).click()

        success: bool = False
        for _ in range(10):
            if self.stream_reader_thread.readline() is not None:
                success = True
                break

            print("Retrying...", file=sys.stderr, flush=True)
            self.driver.find_element(AppiumBy.NAME, title).click()

        self.assertTrue(success, "xembedsniproxy did not receive the click event")

        # The tray icon is a red square
        rect: dict[str, int] = self.driver.find_image_occurrence(self.take_screenshot(), generate_color_block(170, 0, 0))["rect"]

        if os.getenv("TEST_WITH_KWIN_WAYLAND", "1") == "0":
            return

        # Left click
        self.click_center(rect)

        # Middle click
        self.click_center(rect, MouseButton.MIDDLE)

        # Right click
        self.click_center(rect, MouseButton.RIGHT)
        self.assertTrue(self.xembed_tray_icon.popup_menu_event.is_set())

        # Scroll up
        self.scroll_center(rect, 0, -15)

        # Scroll down
        self.scroll_center(rect, 0, 15)

        # Scroll left
        self.scroll_center(rect, -15, 0)

        # Scroll right
        self.scroll_center(rect, 15, 0)

    def test_2_statusnotifieritem(self) -> None:
        """
        Tests for org.kde.StatusNotifierItem
        1. Left click
        2. Right click
        3. Middle click
        4. Wheel
        5. Activate menu actions
        6. NeedsAttention/Active/Passive status
        """
        if os.getenv("TEST_WITH_KWIN_WAYLAND", "1") == "0":
            self.skipTest("inputsynth only works on Wayland")

        status_notifier = subprocess.Popen([os.path.join(CMAKE_RUNTIME_OUTPUT_DIRECTORY, "systemtray_statusnotifiertest")], stdout=subprocess.PIPE)
        self.addCleanup(status_notifier.terminate)
        time.sleep(1)  # Wait until the icon appears
        rect: dict = self.driver.find_image_occurrence(self.take_screenshot(), generate_color_block(255, 0, 0))["rect"]  # Red

        expected_result: list[str] = []

        # Left click
        self.click_center(rect)
        expected_result.append("Activated")

        # Middle click
        self.click_center(rect, MouseButton.MIDDLE)
        expected_result.append("SecondaryActivated")

        # Scroll up
        self.scroll_center(rect, 0, -15)
        expected_result.append("Scrolled by 180 Vertically")

        # Scroll down
        self.scroll_center(rect, 0, 15)
        expected_result.append("Scrolled by -180 Vertically")
        time.sleep(1)

        # Scroll left
        self.scroll_center(rect, -15, 0)
        expected_result.append("Scrolled by 180 Horizontally")

        # Scroll right
        self.scroll_center(rect, 15, 0)
        expected_result.append("Scrolled by -180 Horizontally")

        # Right click
        self.click_center(rect, MouseButton.RIGHT)

        # Click the first action to change the color to blue
        rect = self.driver.find_image_occurrence(self.take_screenshot(), generate_color_block(0, 255, 0))["rect"]  # Green
        self.click_center(rect)
        expected_result.append("NeedsAttention")

        # Right click
        rect = self.driver.find_image_occurrence(self.take_screenshot(), generate_color_block(0, 0, 255))["rect"]  # Blue
        self.click_center(rect, MouseButton.RIGHT)

        # Click the second action to change the color to red
        rect = self.driver.find_image_occurrence(self.take_screenshot(), generate_color_block(255, 85, 255))["rect"]  # Purple
        self.click_center(rect)
        expected_result.append("Active")

        # Right click
        rect = self.driver.find_image_occurrence(self.take_screenshot(), generate_color_block(255, 0, 0))["rect"]  # Red
        self.click_center(rect, MouseButton.RIGHT)

        # Move to the submenu item
        rect = self.driver.find_image_occurrence(self.take_screenshot(), generate_color_block(85, 0, 255))["rect"]
        self.click_center(rect)

        # Move to the submenu
        rect = self.driver.find_image_occurrence(self.take_screenshot(), generate_color_block(255, 255, 0))["rect"]  # Yellow
        self.click_center(rect)
        expected_result.append("Passive")
        # The icon is hidden
        self.assertRaises(Exception, self.driver.find_image_occurrence, self.take_screenshot(), generate_color_block(255, 0, 0))

        status_notifier.terminate()
        status_notifier.wait(10)
        output = status_notifier.stdout.readlines()
        self.assertEqual(len(expected_result), len(output), f"output: f{output} expected: {expected_result}")
        for l in range(0, len(expected_result)):
            self.assertEqual(output[l].decode().strip(), expected_result[l])

    def test_3_bug479466_keyboard_navigation_in_HiddenItemsView(self) -> None:
        """
        Make sure iconContainer in AbstractItem.qml has the default focus so it can receive key presses
        """
        if os.environ.get("TEST_WITH_KWIN_WAYLAND", "1") == "0":
            self.skipTest("In openbox, the popup is not focused by default, so sending keys will not work.")

        self.driver.find_element(AppiumBy.NAME, "Show hidden icons").click()
        wait = WebDriverWait(self.driver, 10)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Notifications")))
        time.sleep(1)

        # By default the focused item is "Notifications"
        # Press Enter key directly to open the widget
        if os.environ.get("TEST_WITH_KWIN_WAYLAND", "1") == "0":
            subprocess.check_call(["xdotool", "key", "Return"])
        else:
            actions = ActionChains(self.driver)
            actions.send_keys(Keys.ENTER).perform()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Do not disturb")))


if __name__ == '__main__':
    logging.getLogger().setLevel(logging.INFO)
    unittest.main()
