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


class TestActions(NotificationsTestBase):

    def _setup_signal_handler(self, loop: GLib.MainLoop):
        state: dict[str, Any] = {
            "activation_token": False, "params_1": [],
            "action_invoked": False, "params_2": [],
            "notification_closed": False, "params_3": [],
        }

        def handler(d_bus_proxy: Gio.DBusProxy, sender_name: str, signal_name: str, parameters: GLib.Variant) -> None:
            logging.info(f"Notification (Id: {parameters.unpack()[0]}) received signal {signal_name}")
            match signal_name:
                case "ActivationToken":
                    state["params_1"] = parameters.unpack()
                    state["activation_token"] = True
                case "ActionInvoked":
                    state["params_2"] = parameters.unpack()
                    state["action_invoked"] = True
                case "NotificationClosed":
                    state["params_3"] = parameters.unpack()
                    state["notification_closed"] = True
                    loop.quit()

        connection_id = self.notification_proxy.connect("g-signal", handler)
        self.addCleanup(lambda: self.notification_proxy.disconnect(connection_id))
        return state

    def _assert_action_signals(self, state: dict[str, Any], notification_id: int, action_key: str):
        self.assertTrue(state["activation_token"])
        self.assertEqual(state["params_1"][0], notification_id)
        self.assertTrue(state["action_invoked"])
        self.assertEqual(state["params_2"][0], notification_id)
        self.assertEqual(state["params_2"][1], action_key)
        self.assertTrue(state["notification_closed"])
        self.assertEqual(state["params_3"][0], notification_id)
        self.assertEqual(state["params_3"][1], 3)

    def test_0_actions(self) -> None:
        """
        When the "actions" key is set, a notification can provide actions.
        """
        wait = WebDriverWait(self.driver, 5, ignored_exceptions=(NoSuchElementException, WebDriverException))
        loop = GLib.MainLoop()
        state = self._setup_signal_handler(loop)

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
        self._assert_action_signals(state, notification_id, "action1")
        wait.until(EC.invisibility_of_element(element))

        notification_ids: list[int] = []
        for i in range(2):
            notification_id = send_notification({
                "app_name": f"Appium Test {i}",
                "body": f"A notification with actions {i}",
                "actions": ["action1", f"FooAction {i}", "action2", f"BarAction {i}"],
                "hints": {"desktop-entry": GLib.Variant("s", "org.kde.plasmashell")},
                "timeout": 100,
            })
            notification_ids.append(notification_id)
            wait.until(EC.presence_of_element_located((AppiumBy.XPATH, f"//button[@name='FooAction {i}' and contains(@accessibility-id, 'FullRepresentation')]")))
            wait.until(EC.presence_of_element_located((AppiumBy.XPATH, f"//button[@name='BarAction {i}' and contains(@accessibility-id, 'FullRepresentation')]")))

        for i in range(2):
            state = self._setup_signal_handler(loop)
            element = wait.until(EC.presence_of_element_located((AppiumBy.XPATH, f"//button[@name='BarAction {i}' and contains(@accessibility-id, 'FullRepresentation')]")))
            element.click()
            loop.run()
            self._assert_action_signals(state, notification_ids[i], "action2")
            wait.until(EC.invisibility_of_element(element))


if __name__ == '__main__':
    unittest.main()
