#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import logging
import os
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from typing import Any, Final

import gi
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.common.exceptions import WebDriverException
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

gi.require_version('Gdk', '4.0')
gi.require_version('GdkPixbuf', '2.0')
from gi.repository import Gdk, GdkPixbuf, Gio, GLib

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "utils"))
from GLibMainLoopThread import GLibMainLoopThread

WIDGET_ID: Final = "org.kde.plasma.notifications"
KDE_VERSION: Final = 6
BUS_NAME: Final = "org.freedesktop.Notifications"
OBJECT_PATH: Final = "/org/freedesktop/Notifications"
IFACE_NAME: Final = BUS_NAME


def send_notification(data: dict[str, str | int | list[str] | dict[str, GLib.Variant] | GLib.Variant], session_bus: Gio.DBusConnection | None = None) -> int:
    """
    @returns Notification id
    """
    app_name: str = str(data.get("app_name", "Appium Test"))
    replaces_id: int = int(data.get("replaces_id", 0))
    app_icon: str = str(data.get("app_icon", "wayland"))
    summary: str = str(data.get("summary", ""))
    body: str = str(data.get("body", ""))
    actions: list[str] = data.get("actions", [])
    hints: dict[str, GLib.Variant] = data.get("hints", {})
    timeout: int = data.get("timeout", -1)
    parameters = GLib.Variant("(susssasa{sv}i)", [app_name, replaces_id, app_icon, summary, body, actions, hints, timeout])

    if session_bus is None:
        session_bus = Gio.bus_get_sync(Gio.BusType.SESSION)

    reply = session_bus.call_sync(BUS_NAME, OBJECT_PATH, IFACE_NAME, "Notify", parameters, None, Gio.DBusSendMessageFlags.NONE, 5000)
    return reply.get_child_value(0).get_uint32()


class NotificationsTest(unittest.TestCase):
    """
    Tests for the notification widget
    """

    loop_thread: GLibMainLoopThread
    notification_proxy: Gio.DBusProxy
    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
        })
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

        cls.loop_thread = GLibMainLoopThread()
        cls.loop_thread.start()

        cls.notification_proxy = Gio.DBusProxy.new_for_bus_sync(Gio.BusType.SESSION, 0, None, BUS_NAME, OBJECT_PATH, IFACE_NAME)

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
        cls.loop_thread.quit()
        subprocess.check_call([f"kquitapp{KDE_VERSION}", "plasmawindowed"])
        for _ in range(10):
            try:
                subprocess.check_call(["pidof", "plasmawindowed"])
            except subprocess.CalledProcessError:
                break
            time.sleep(1)
        cls.driver.quit()

    def test_0_open(self) -> None:
        """
        Tests the widget can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "Do not disturb")

    def test_1_bug481033_malformed_image_data(self) -> None:
        """
        If `image-data` is not a structure, the broken image hint would crash plasmashell.
        """
        summary: str = "Bug481033"
        send_notification({
            "app_name": "Appium Test",
            "summary": summary,
            "body": "Will it crash",
            "hints": {
                "desktop-entry": GLib.Variant("s", "systemsettings"),
                "image-data": GLib.Variant("s", "not a structure"),
            },
        })

        wait = WebDriverWait(self.driver, 5)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, summary)))
        close_button = self.driver.find_element(AppiumBy.NAME, "Close")
        close_button.click()
        wait.until_not(lambda _: close_button.is_displayed())

    def take_screenshot(self) -> str:
        with tempfile.TemporaryDirectory() as temp_dir:
            saved_image_path = os.path.join(temp_dir, "tray.png")
            self.driver.get_screenshot_as_file(saved_image_path)
            return base64.b64encode(Gdk.Texture.new_from_filename(saved_image_path).save_to_png_bytes().get_data()).decode()

    def test_2_notification_with_image(self) -> None:
        """
        Sends notifications with images
        """
        wait = WebDriverWait(self.driver, 5)
        summary: str = "Image notification"
        pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 256, 256)
        partial_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 16, 16)
        colors = (0xff0000ff, 0x00ff00ff, 0x0000ffff)
        for color in colors:
            pixbuf.fill(color)
            send_notification({
                "app_name": "Appium Test",
                "summary": summary + str(color),
                "body": f"Notification body {str(color)}",
                "hints": {
                    "desktop-entry": GLib.Variant("s", "firefox"),
                    "image-data": GLib.Variant("(iiibiiay)", [
                        pixbuf.get_width(),
                        pixbuf.get_height(),
                        pixbuf.get_rowstride(),
                        pixbuf.get_has_alpha(),
                        pixbuf.get_bits_per_sample(),
                        pixbuf.get_n_channels(),
                        pixbuf.get_pixels(),
                    ]),
                },
                "timeout": 10 * 1000,
            })
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, summary + str(color))))
            partial_pixbuf.fill(color)
            partial_image = base64.b64encode(Gdk.Texture.new_for_pixbuf(partial_pixbuf).save_to_png_bytes().get_data()).decode()
            self.driver.find_image_occurrence(self.take_screenshot(), partial_image)
            close_button = self.driver.find_element(AppiumBy.NAME, "Close")
            close_button.click()
            wait.until_not(lambda _: close_button.is_displayed())

    def test_3_accessible_description_html_to_plaintext(self) -> None:
        """
        accessibleDescription provides the plain text of the description
        """
        send_notification({
            "app_name": "Appium Test",
            "body": "<b>b</b><i>i</i><u>u</u><blink>blink</blink><a href=\"https://www.example.org/\">www.example.org</a>",
        })
        wait = WebDriverWait(self.driver, 5)
        wait.until(EC.presence_of_element_located(("description", "biublinkwww.example.org  from Appium Test")))
        close_button = self.driver.find_element(AppiumBy.NAME, "Close")
        close_button.click()
        wait.until_not(lambda _: close_button.is_displayed())

    def test_4_actions(self) -> None:
        """
        When the "actions" key is set, a notification can provide actions.
        """
        activation_token = threading.Event()
        params_1: list[Any] = []
        action_invoked = threading.Event()
        params_2: list[Any] = []
        notification_closed = threading.Event()
        params_3: list[Any] = []

        def notification_signal_handler(d_bus_proxy: Gio.DBusProxy, sender_name: str, signal_name: str, parameters: GLib.Variant) -> None:
            nonlocal params_2, params_3, params_1
            logging.info(f"received signal {signal_name}")
            match signal_name:
                case "ActivationToken":
                    params_1 = parameters.unpack()
                    activation_token.set()
                case "ActionInvoked":
                    params_2 = parameters.unpack()
                    action_invoked.set()
                case "NotificationClosed":
                    params_3 = parameters.unpack()
                    notification_closed.set()

        connection_id = self.notification_proxy.connect("g-signal", notification_signal_handler)
        self.addCleanup(lambda: self.notification_proxy.disconnect(connection_id))

        notification_closed.clear()
        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": "A notification with actions",
            "actions": ["action1", "FooAction", "action2", "BarAction"],
        })
        self.driver.find_element(AppiumBy.NAME, "BarAction")
        element = self.driver.find_element(AppiumBy.NAME, "FooAction")
        element.click()
        self.assertTrue(activation_token.wait(10))
        self.assertEqual(params_1[0], notification_id)
        self.assertTrue(action_invoked.wait(10))
        self.assertEqual(params_2[0], notification_id)
        self.assertEqual(params_2[1], "action1")
        self.assertTrue(notification_closed.wait(10))
        self.assertEqual(params_3[0], notification_id)
        self.assertEqual(params_3[1], 3)  # reason: Revoked
        self.assertFalse(element.is_displayed())

    def test_5_inline_reply(self) -> None:
        """
        When the action list has "inline-reply", the notification popup will contain a text field and a reply button.
        """
        notification_replied = threading.Event()
        params: list[Any] = []  # id, text

        def notification_signal_handler(d_bus_proxy: Gio.DBusProxy, sender_name: str, signal_name: str, parameters: GLib.Variant) -> None:
            nonlocal params
            logging.info(f"received signal {signal_name}")
            if signal_name == "NotificationReplied":
                params = parameters.unpack()
                notification_replied.set()

        connection_id = self.notification_proxy.connect("g-signal", notification_signal_handler)
        self.addCleanup(lambda: self.notification_proxy.disconnect(connection_id))

        # When there is only one action and it is a reply action, show text field right away
        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": "A notification with actions 1",
            "actions": ["inline-reply", ""],  # Use the default label
        })
        reply_text = "this is a reply"
        self.driver.find_element(AppiumBy.NAME, "begin reply").click()
        self.driver.find_element(AppiumBy.NAME, "Type a reply…").send_keys(reply_text)
        element = self.driver.find_element(AppiumBy.NAME, "Send")
        element.click()
        self.assertTrue(notification_replied.wait(10))
        self.assertEqual(params[0], notification_id)
        self.assertEqual(params[1], reply_text)
        self.assertFalse(element.is_displayed())

        notification_replied.clear()
        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": "A notification with actions 2",
            "actions": ["inline-reply", ""],
            "hints": {
                "x-kde-reply-submit-button-text": GLib.Variant("s", "Reeply"),  # Use a custom label
                "x-kde-reply-placeholder-text": GLib.Variant("s", "A placeholder"),  # Use a custom placeholder
            },
        })
        reply_text = "this is another reply"
        self.driver.find_element(AppiumBy.NAME, "begin reply").click()
        self.driver.find_element(AppiumBy.NAME, "A placeholder").send_keys(reply_text)
        element = self.driver.find_element(AppiumBy.NAME, "Reeply")
        element.click()
        self.assertTrue(notification_replied.wait(10))
        self.assertEqual(params[0], notification_id)
        self.assertEqual(params[1], reply_text)
        self.assertFalse(element.is_displayed())

        notification_replied.clear()
        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": "A notification with actions 3",
            "actions": ["inline-reply", "Replyy", "foo", "Foo", "bar", "Bar"],  # Click to show the text field
        })
        self.driver.find_element(AppiumBy.NAME, "Foo")
        self.driver.find_element(AppiumBy.NAME, "Bar")
        element = self.driver.find_element(AppiumBy.NAME, "Replyy")
        element.click()
        reply_text = "Click Replyy to reply"
        self.driver.find_element(AppiumBy.NAME, "Type a reply…").send_keys(reply_text)
        self.assertFalse(element.is_displayed())
        element = self.driver.find_element(AppiumBy.NAME, "Send")
        element.click()
        self.assertTrue(notification_replied.wait(10))
        self.assertEqual(params[0], notification_id)
        self.assertEqual(params[1], reply_text)

    def test_6_thumbnail(self) -> None:
        """
        When a notification has "x-kde-urls" hint, a thumbnail will be shown for the first url in the list
        """
        with tempfile.TemporaryDirectory() as temp_dir:
            pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 256, 256)
            colors = (0xff0000ff, 0x00ff00ff, 0x0000ffff)
            for color in colors:
                pixbuf.fill(color)
                pixbuf.savev(os.path.join(temp_dir, f"{str(color)}.png"), "png")

            url_list = [f"file://{os.path.join(temp_dir, path)}" for path in os.listdir(temp_dir)]
            url_list.sort()
            send_notification({
                "app_name": "Appium Test",
                "body": "Thumbnail",
                "hints": {
                    "x-kde-urls": GLib.Variant("as", url_list),
                },
                "timeout": 10 * 1000,
            })

            self.driver.find_element(AppiumBy.NAME, "More Options…")

            partial_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, 8, 100, 100)
            partial_pixbuf.fill(colors[1])  # Green is the first item
            partial_image = base64.b64encode(Gdk.Texture.new_for_pixbuf(partial_pixbuf).save_to_png_bytes().get_data()).decode()

            def match_image(driver) -> bool:
                try:
                    self.driver.find_image_occurrence(self.take_screenshot(), partial_image)
                    return True
                except WebDriverException:
                    return False

            WebDriverWait(self.driver, 10).until(match_image)


if __name__ == '__main__':
    assert "USE_CUSTOM_BUS" in os.environ
    logging.getLogger().setLevel(logging.INFO)
    unittest.main()
