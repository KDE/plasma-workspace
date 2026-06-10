#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import logging
import os
import sys
from typing import Any
import unittest

_applets_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)
_appiumtests_dir = os.path.join(_applets_dir, os.pardir)
sys.path.insert(0, _appiumtests_dir)
sys.path.insert(0, _applets_dir)

from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gio, GLib
from selenium.common.exceptions import NoSuchElementException, WebDriverException
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

from notificationstest.base import NotificationsTestBase
from utils.notification_helpers import send_notification


class TestInlineReply(NotificationsTestBase):

    def _test_reply(self, body: str, actions: list[str], reply_text: str, hints: dict | None = None, send_button: str = "Send", placeholder: str = "Type a reply…"):
        loop = GLib.MainLoop()
        notification_replied = False
        params: list[Any] = []

        def handler(d_bus_proxy: Gio.DBusProxy, sender_name: str, signal_name: str, parameters: GLib.Variant) -> None:
            nonlocal params, notification_replied
            logging.info(f"received signal {signal_name}")
            if signal_name == "NotificationReplied":
                params = parameters.unpack()
                notification_replied = True
                loop.quit()

        connection_id = self.notification_proxy.connect("g-signal", handler)
        self.addCleanup(lambda: self.notification_proxy.disconnect(connection_id))

        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": body,
            "actions": actions,
            "hints": hints or {},
            "timeout": 30 * 1000,
        })
        self.driver.find_element(AppiumBy.NAME, "begin reply").click()
        self.driver.find_element(AppiumBy.NAME, placeholder).send_keys(reply_text)
        element = self.driver.find_element(AppiumBy.NAME, send_button)
        element.click()
        loop.run()
        self.assertTrue(notification_replied)
        self.assertEqual(params[0], notification_id)
        self.assertEqual(params[1], reply_text)
        WebDriverWait(self.driver, 5, ignored_exceptions=(NoSuchElementException, WebDriverException)).until(EC.invisibility_of_element(element))

    def test_0_inline_reply(self) -> None:
        """
        When the action list has "inline-reply", the notification popup will contain a text field and a reply button.
        """
        self._test_reply("A notification with actions 1", ["inline-reply", ""], "this is a reply")

        self._test_reply("A notification with actions 2", ["inline-reply", ""], "this is another reply",
                         hints={
                             "x-kde-reply-submit-button-text": GLib.Variant("s", "Reeply"),
                             "x-kde-reply-placeholder-text": GLib.Variant("s", "A placeholder"),
                         },
                         send_button="Reeply", placeholder="A placeholder")

        loop = GLib.MainLoop()
        notification_replied = False
        params: list[Any] = []

        def handler(d_bus_proxy: Gio.DBusProxy, sender_name: str, signal_name: str, parameters: GLib.Variant) -> None:
            nonlocal params, notification_replied
            logging.info(f"received signal {signal_name}")
            if signal_name == "NotificationReplied":
                params = parameters.unpack()
                notification_replied = True
                loop.quit()

        connection_id = self.notification_proxy.connect("g-signal", handler)
        self.addCleanup(lambda: self.notification_proxy.disconnect(connection_id))

        notification_id = send_notification({
            "app_name": "Appium Test",
            "body": "A notification with actions 3",
            "actions": ["inline-reply", "Replyy", "foo", "Foo", "bar", "Bar"],
            "timeout": 30 * 1000,
        })
        self.driver.find_element(AppiumBy.NAME, "Foo")
        self.driver.find_element(AppiumBy.NAME, "Bar")
        element = self.driver.find_element(AppiumBy.NAME, "Replyy")
        element.click()
        self.driver.find_element(AppiumBy.NAME, "Type a reply…").send_keys("Click Replyy to reply")
        WebDriverWait(self.driver, 5, ignored_exceptions=(NoSuchElementException, WebDriverException)).until(EC.invisibility_of_element(element))
        element = self.driver.find_element(AppiumBy.NAME, "Send")
        element.click()
        loop.run()
        self.assertTrue(notification_replied)
        self.assertEqual(params[0], notification_id)
        self.assertEqual(params[1], "Click Replyy to reply")


if __name__ == '__main__':
    unittest.main()
