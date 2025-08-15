#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import logging
import os
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from typing import Any, Final

import gi
from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.common.exceptions import (NoSuchElementException, WebDriverException)
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

gi.require_version('Gdk', '4.0')
gi.require_version('GdkPixbuf', '2.0')
from gi.repository import Gdk, GdkPixbuf, Gio, GLib
from kicker.favoritetest import start_kactivitymanagerd
from notificationstest.jobnotificationinterface import JobNotificationControlInterface

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

    notification_proxy: Gio.DBusProxy
    driver: webdriver.Remote
    kactivitymanagerd: subprocess.Popen

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        # Make history work
        cls.kactivitymanagerd = start_kactivitymanagerd()

        os.makedirs(os.path.join(GLib.get_user_data_dir(), "knotifications6"))
        shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "libnotificationmanager", "libnotificationmanager.notifyrc"), os.path.join(GLib.get_user_data_dir(), "knotifications6"))

        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
            "QT_LOGGING_RULES": "kf.notification*.debug=true;org.kde.plasma.notificationmanager.debug=true",
        })
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

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
        subprocess.check_call([f"kquitapp{KDE_VERSION}", "plasmawindowed"])
        cls.kactivitymanagerd.kill()
        cls.kactivitymanagerd.wait(10)
        for _ in range(10):
            try:
                subprocess.check_call(["pidof", "plasmawindowed"])
            except subprocess.CalledProcessError:
                break
            time.sleep(1)
        cls.driver.quit()

    def close_notifications(self) -> None:
        wait = WebDriverWait(self.driver, 5)
        for button in self.driver.find_elements(AppiumBy.XPATH, "//button[@name='Close']"):
            try:
                button.click()
                wait.until_not(lambda _: button.is_displayed())
            except WebDriverException:
                pass

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
        self.close_notifications()

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
            logging.info(f"Testing color: {color}")
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
                "timeout": 30 * 1000,
            })
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, summary + str(color))))
            partial_pixbuf.fill(color)
            partial_image = base64.b64encode(Gdk.Texture.new_for_pixbuf(partial_pixbuf).save_to_png_bytes().get_data()).decode()
            try:
                self.driver.find_image_occurrence(self.take_screenshot(), partial_image)
            except WebDriverException:  # Popup animation
                self.driver.find_image_occurrence(self.take_screenshot(), partial_image)
            self.close_notifications()

    def test_2_notification_with_explicit_timeout(self) -> None:
        """
        Sends notifications with expire_timeout
        """
        summary = "expire_timeout"
        send_notification({
            "app_name": "Appium Test",
            "summary": summary,
            "body": "Will it disappear automatically?",
            "timeout": 2000,
        })
        element = self.driver.find_element(AppiumBy.NAME, summary)
        WebDriverWait(self.driver, 5).until_not(lambda _: element.is_displayed())

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
        self.close_notifications()

    def test_4_actions(self) -> None:
        """
        When the "actions" key is set, a notification can provide actions.
        """
        loop = GLib.MainLoop()
        activation_token = False
        params_1: list[Any] = []
        action_invoked = False
        params_2: list[Any] = []
        notification_closed = False
        params_3: list[Any] = []

        def reset_state() -> None:
            nonlocal params_2, params_3, params_1, activation_token, action_invoked, notification_closed
            activation_token = False
            params_1 = []
            action_invoked = False
            params_2 = []
            notification_closed = False
            params_3 = []

        def notification_signal_handler(d_bus_proxy: Gio.DBusProxy, sender_name: str, signal_name: str, parameters: GLib.Variant) -> None:
            nonlocal params_2, params_3, params_1, activation_token, action_invoked, notification_closed
            logging.info(f"Notification (Id: {parameters.unpack()[0]}) received signal {signal_name}")
            match signal_name:
                case "ActivationToken":
                    params_1 = parameters.unpack()
                    activation_token = True
                case "ActionInvoked":
                    params_2 = parameters.unpack()
                    action_invoked = True
                case "NotificationClosed":
                    params_3 = parameters.unpack()
                    notification_closed = True
                    loop.quit()

        connection_id = self.notification_proxy.connect("g-signal", notification_signal_handler)
        self.addCleanup(lambda: self.notification_proxy.disconnect(connection_id))

        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": "A notification with actions",
            "actions": ["action1", "FooAction", "action2", "BarAction"],
            "timeout": 30 * 1000,
        })
        self.driver.find_element(AppiumBy.NAME, "BarAction")
        element = self.driver.find_element(AppiumBy.NAME, "FooAction")
        element.click()
        loop.run()
        self.assertTrue(activation_token)
        self.assertEqual(params_1[0], notification_id)
        self.assertTrue(action_invoked)
        self.assertEqual(params_2[0], notification_id)
        self.assertEqual(params_2[1], "action1")
        self.assertTrue(notification_closed)
        self.assertEqual(params_3[0], notification_id)
        self.assertEqual(params_3[1], 3)  # reason: Revoked
        WebDriverWait(self.driver, 5).until_not(lambda _: element.is_displayed())

        # Test actions in the history list
        # Consolidates https://invent.kde.org/plasma/plasma-workspace/-/commit/568f4c6cb2a2e0716538350cb9b1a4469838575b
        notification_ids: list[int] = []
        for i in range(2):
            notification_id = send_notification({
                "app_name": f"Appium Test {i}",
                "body": f"A notification with actions {i}",
                "actions": ["action1", f"FooAction {i}", "action2", f"BarAction {i}"],
                "hints": {
                    "desktop-entry": GLib.Variant("s", "org.kde.plasmashell"),
                },
                "timeout": 100,
            })
            notification_ids.append(notification_id)
            self.driver.find_element(AppiumBy.XPATH, f"//button[@name='FooAction {i}' and contains(@accessibility-id, 'FullRepresentation')]")

        # Click the button in the second(oldest) history item
        for i in range(2):
            reset_state()
            element = self.driver.find_element(AppiumBy.XPATH, f"//button[@name='BarAction {i}' and contains(@accessibility-id, 'FullRepresentation')]")
            element.click()
            loop.run()
            self.assertTrue(activation_token)
            self.assertEqual(params_1[0], notification_ids[i])
            self.assertTrue(action_invoked)
            self.assertEqual(params_2[0], notification_ids[i])
            self.assertEqual(params_2[1], "action2")
            self.assertTrue(notification_closed)
            self.assertEqual(params_3[0], notification_ids[i])
            self.assertEqual(params_3[1], 3)  # reason: Revoked
            WebDriverWait(self.driver, 5).until_not(lambda _: element.is_displayed())

    def test_5_inline_reply(self) -> None:
        """
        When the action list has "inline-reply", the notification popup will contain a text field and a reply button.
        """
        loop = GLib.MainLoop()
        notification_replied = False
        params: list[Any] = []  # id, text

        def notification_signal_handler(d_bus_proxy: Gio.DBusProxy, sender_name: str, signal_name: str, parameters: GLib.Variant) -> None:
            nonlocal params, notification_replied
            logging.info(f"received signal {signal_name}")
            if signal_name == "NotificationReplied":
                params = parameters.unpack()
                notification_replied = True
                loop.quit()

        connection_id = self.notification_proxy.connect("g-signal", notification_signal_handler)
        self.addCleanup(lambda: self.notification_proxy.disconnect(connection_id))

        # When there is only one action and it is a reply action, show text field right away
        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": "A notification with actions 1",
            "actions": ["inline-reply", ""],  # Use the default label
            "timeout": 30 * 1000,
        })
        reply_text = "this is a reply"
        self.driver.find_element(AppiumBy.NAME, "begin reply").click()
        self.driver.find_element(AppiumBy.NAME, "Type a reply…").send_keys(reply_text)
        element = self.driver.find_element(AppiumBy.NAME, "Send")
        element.click()
        loop.run()
        self.assertTrue(notification_replied)
        self.assertEqual(params[0], notification_id)
        self.assertEqual(params[1], reply_text)
        self.assertFalse(element.is_displayed())

        notification_replied = False
        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": "A notification with actions 2",
            "actions": ["inline-reply", ""],
            "hints": {
                "x-kde-reply-submit-button-text": GLib.Variant("s", "Reeply"),  # Use a custom label
                "x-kde-reply-placeholder-text": GLib.Variant("s", "A placeholder"),  # Use a custom placeholder
            },
            "timeout": 30 * 1000,
        })
        reply_text = "this is another reply"
        self.driver.find_element(AppiumBy.NAME, "begin reply").click()
        self.driver.find_element(AppiumBy.NAME, "A placeholder").send_keys(reply_text)
        element = self.driver.find_element(AppiumBy.NAME, "Reeply")
        element.click()
        loop.run()
        self.assertTrue(notification_replied)
        self.assertEqual(params[0], notification_id)
        self.assertEqual(params[1], reply_text)
        self.assertFalse(element.is_displayed())

        notification_replied = False
        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": "A notification with actions 3",
            "actions": ["inline-reply", "Replyy", "foo", "Foo", "bar", "Bar"],  # Click to show the text field
            "timeout": 30 * 1000,
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
        loop.run()
        self.assertTrue(notification_replied)
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
                "timeout": 30 * 1000,
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
            self.close_notifications()

    def test_7_job_notification(self) -> None:
        """
        https://bugs.kde.org/show_bug.cgi?id=501991
        """
        try:
            from PySide6.QtCore import QObject
        except ModuleNotFoundError:
            self.skipTest("PySide is not available")

        asan_env = os.environ.copy()
        asan_env["LD_PRELOAD"] = subprocess.check_output(["gcc", "-print-file-name=libasan.so"]).strip().decode(encoding="utf-8")
        proc = subprocess.Popen(["python3", os.path.join(os.path.dirname(os.path.abspath(__file__)), "notificationstest", "jobnotification.py")], stdout=sys.stderr, stderr=sys.stderr, env=asan_env)
        self.addCleanup(proc.kill)
        self.driver.find_element(AppiumBy.NAME, "10%")
        self.driver.find_element(AppiumBy.XPATH, "//notification[contains(@description, '1 file')]")
        self.driver.find_element(AppiumBy.NAME, "Pause").click()
        self.driver.find_element(AppiumBy.NAME, "Resume").click()
        self.driver.find_element(AppiumBy.XPATH, "//button[@name='Details' and contains(@accessibility-id, 'NotificationPopup')]").click()  # Expand details in the popup

        # Change progress to 20%
        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
        session_bus.call_sync(JobNotificationControlInterface.BUS_NAME, JobNotificationControlInterface.OBJECT_PATH, JobNotificationControlInterface.BUS_NAME, "SetProcessedAmount", GLib.Variant("(t)", [1024 * 1024 * 2]), None, Gio.DBusSendMessageFlags.NONE, 1000)
        self.driver.find_element(AppiumBy.NAME, "20%")
        self.driver.find_element(AppiumBy.NAME, "8 s remaining")
        self.driver.find_element(AppiumBy.NAME, "1.0 MiB/s")
        self.driver.find_element(AppiumBy.XPATH, "//chart[contains(@accessibility-id, 'LineChart')]")

        # Change progress to 50%
        session_bus.call_sync(JobNotificationControlInterface.BUS_NAME, JobNotificationControlInterface.OBJECT_PATH, JobNotificationControlInterface.BUS_NAME, "SetProcessedAmount", GLib.Variant("(t)", [1024 * 1024 * 5]), None, Gio.DBusSendMessageFlags.NONE, 1000)
        self.driver.find_element(AppiumBy.NAME, "50%")
        self.driver.find_element(AppiumBy.NAME, "2 s remaining")
        self.driver.find_element(AppiumBy.NAME, "3.0 MiB/s")

        # Change progress to 100%. The job should emit finished signal.
        session_bus.call_sync(JobNotificationControlInterface.BUS_NAME, JobNotificationControlInterface.OBJECT_PATH, JobNotificationControlInterface.BUS_NAME, "SetProcessedAmount", GLib.Variant("(t)", [1024 * 1024 * 10]), None, Gio.DBusSendMessageFlags.NONE, 1000)
        self.driver.find_element(AppiumBy.NAME, "Job Finished")
        self.driver.find_element(AppiumBy.XPATH, "//notification[contains(@description, 'file:///home/foobar/test.txt')]")
        proc.wait(10)

    def test_8_do_not_disturb(self) -> None:
        """
        Suppress inhibited notifications after "Do not disturb" is turned off, and show a summary for unread inhibited notifications.
        """
        self.driver.find_element(AppiumBy.NAME, "Do not disturb").click()
        dnd_button = self.driver.find_element(AppiumBy.XPATH, "//*[@name='Do not disturb' and contains(@states, 'checked')]")

        summary = "Do not disturb me"
        for i in range(2):
            send_notification({
                "app_name": "Appium Test",
                "summary": summary + str(i),
                "hints": {
                    "desktop-entry": GLib.Variant("s", "org.kde.plasmashell"),
                },
                "timeout": 60 * 1000,
            })
        title = self.driver.find_element(AppiumBy.XPATH, f"//heading[starts-with(@name, '{summary}') and contains(@accessibility-id, 'FullRepresentation')]")
        self.assertRaises((NoSuchElementException, WebDriverException), self.driver.find_element, AppiumBy.XPATH, f"//notification[starts-with(@name, '{summary}')]")

        # Test "CriticalInDndMode": show critical notifications even when "Do not disturb" is active
        send_notification({
            "app_name": "Appium Test",
            "summary": "CriticalInDndMode",
            "hints": {
                "desktop-entry": GLib.Variant("s", "org.kde.plasmashell"),
                "urgency": GLib.Variant("i", 2),  # Critical
            },
        })
        self.driver.find_element(AppiumBy.XPATH, "//notification[@name='CriticalInDndMode']")

        # Disable DND & verify only the summary is shown
        dnd_button.click()
        self.driver.find_element(AppiumBy.XPATH, "//notification[@name='Unread Notifications' and @description='2 notifications were received while Do Not Disturb was active.  from Notification Manager']")
        self.driver.find_element(AppiumBy.XPATH, "//button[@name='Close' and contains(@accessibility-id, 'NotificationPopup')]").click()

        # Toggle DND on/off again, and verify the summary is not shown for the same notifications
        dnd_button.click()
        dnd_button.click()
        self.assertRaises((NoSuchElementException, WebDriverException), self.driver.find_element, AppiumBy.XPATH, f"//notification[starts-with(@name, '{summary}')]")
        self.assertRaises((NoSuchElementException, WebDriverException), self.driver.find_element, AppiumBy.XPATH, "//notification[@name='Unread Notifications' and @description='2 notifications were received while Do Not Disturb was active.  from Notification Manager']")

        # Notifications can only be cleared after they are expired, otherwise they will stay in the list
        self.driver.find_element(AppiumBy.NAME, "Clear All Notifications").click()
        WebDriverWait(self.driver, 5).until_not(lambda _: title.is_displayed())


if __name__ == '__main__':
    assert "USE_CUSTOM_BUS" in os.environ or subprocess.call(["pidof", "plasmashell"]) != 0
    logging.getLogger().setLevel(logging.INFO)
    unittest.main(failfast=True)
