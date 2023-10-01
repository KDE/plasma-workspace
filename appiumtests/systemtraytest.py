#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import queue
import sys
import threading
import unittest
from datetime import date
from subprocess import PIPE, Popen
from time import sleep
from typing import IO, Final

import gi

gi.require_version("Gtk", "3.0")  # StatusIcon is removed in 4
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gio, GLib, Gtk
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

KDE_VERSION: Final = 6
WIDGET_ID: Final = "org.kde.plasma.systemtray"


class XEmbedTrayIcon(threading.Thread):
    """
    XEmbed tray icon implementation using Gtk.StatusIcon
    """

    def __init__(self, title: str) -> None:
        super().__init__()

        self.__timer: threading.Timer = threading.Timer(300, Gtk.main_quit)  # Failsafe

        self.__status_icon: Gtk.StatusIcon = Gtk.StatusIcon(title=title)
        self.__status_icon.set_from_icon_name("xorg")
        self.__status_icon.connect("button-press-event", self.__on_button_press_event)
        self.__status_icon.connect("button-release-event", self.__on_button_release_event)
        self.__status_icon.connect("popup-menu", self.__on_popup_menu)
        self.__status_icon.connect("scroll-event", self.__on_scroll_event)

    def run(self) -> None:
        self.__timer.start()
        Gtk.main()

    def quit(self) -> None:
        self.__timer.cancel()
        Gtk.main_quit()

    def __on_button_press_event(self, status_icon: Gtk.StatusIcon, button_event) -> None:
        print("button-press-event", button_event.button, file=sys.stderr, flush=True)

    def __on_button_release_event(self, status_icon: Gtk.StatusIcon, button_event) -> None:
        print("button-release-event", button_event.button, file=sys.stderr, flush=True)
        self.quit()

    def __on_popup_menu(self, status_icon: Gtk.StatusIcon, button: int, activate_time: int) -> None:
        print("popup-menu", button, activate_time, file=sys.stderr, flush=True)

    def __on_scroll_event(self, status_icon, scroll_event) -> None:
        print("scroll-event", scroll_event.delta_x, scroll_event.delta_y, int(scroll_event.direction), file=sys.stderr, flush=True)


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
    xembedsniproxy: Popen[bytes]
    xembed_tray_icon: XEmbedTrayIcon | None
    stream_reader_thread: StreamReaderThread | None

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def setUp(self) -> None:
        self.kded = Popen([f"kded{KDE_VERSION}"])
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
        self.assertTrue(kded_reply.get_child_value(0).get_boolean(), "Module is not loaded")

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_systemtraytest_#{self.id()}.png")
        self.kded.kill()

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.quit()

    def cleanup_xembed_tray_icon(self) -> None:
        """
        Cleanup function for test_xembed_tray_icon
        """
        self.xembedsniproxy.terminate()
        self.xembedsniproxy = None

        if self.xembed_tray_icon is not None:
            self.xembed_tray_icon.quit()
            self.xembed_tray_icon = None

        if self.stream_reader_thread is not None and self.stream_reader_thread.is_alive():
            self.stream_reader_thread.stop()
            self.stream_reader_thread = None

    def test_xembed_tray_icon(self) -> None:
        """
        Tests XEmbed tray icons can be listed and clicked in the tray.

        @note GTK doesn't like send_events and double checks the mouse position
        matches where the window is and is top level, so match the debug
        output from xembedsniproxy instead.
        """
        self.addCleanup(self.cleanup_xembed_tray_icon)

        debug_env: dict[str, str] = os.environ.copy()
        debug_env["QT_LOGGING_RULES"] = "kde.xembedsniproxy.debug=true"
        self.xembedsniproxy = Popen(['xembedsniproxy', '--platform', 'xcb'], env=debug_env, stderr=PIPE)  # For debug output
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


if __name__ == '__main__':
    unittest.main()
