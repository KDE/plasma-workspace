#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import unittest
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gio, GLib
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

WIDGET_ID: Final = "org.kde.plasma.notifications"


def send_notification(data: dict[str, str | int | list[str] | dict[str, GLib.Variant] | GLib.Variant], session_bus: Gio.DBusConnection | None = None):
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
    session_bus.call_sync("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify", parameters, None, Gio.DBusSendMessageFlags.NONE, 1000)


class NotificationsTest(unittest.TestCase):
    """
    Tests for the notification widget
    """

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

    def test_2_accessible_description_html_to_plaintext(self) -> None:
        """
        accessibleDescription provides the plain text of the description
        """
        send_notification({
            "app_name": "Appium Test",
            "body": "<b>b</b><i>i</i><u>u</u><blink>blink</blink><a href=\"https://www.example.org/\">www.example.org</a>",
        })
        wait = WebDriverWait(self.driver, 5)
        wait.until(EC.presence_of_element_located(("description", "biublinkwww.example.org  from Appium Test")))


if __name__ == '__main__':
    unittest.main()
