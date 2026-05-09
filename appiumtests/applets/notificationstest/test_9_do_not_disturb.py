#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import sys
import unittest

_applets_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)
_appiumtests_dir = os.path.join(_applets_dir, os.pardir)
sys.path.insert(0, _appiumtests_dir)
sys.path.insert(0, _applets_dir)

from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import GLib
from selenium.common.exceptions import NoSuchElementException, WebDriverException
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

from notificationstest.base import NotificationsTestBase
from utils.notification_helpers import send_notification


class TestDoNotDisturb(NotificationsTestBase):

    def test_0_do_not_disturb(self) -> None:
        """
        Suppress inhibited notifications after "Do not disturb" is turned off, and show a summary for unread inhibited notifications.
        """
        wait = WebDriverWait(self.driver, 5, ignored_exceptions=(NoSuchElementException, WebDriverException))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Do not disturb"))).click()
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//*[@name='Do not disturb' and contains(@states, 'checked')]")))

        summary = "Do not disturb me"
        for i in range(2):
            send_notification({
                "app_name": "Appium Test",
                "summary": summary + str(i),
                "hints": {"desktop-entry": GLib.Variant("s", "org.kde.plasmashell")},
                "timeout": 60 * 1000,
            })
        title = wait.until(EC.presence_of_element_located((AppiumBy.XPATH, f"//heading[starts-with(@name, '{summary}') and contains(@accessibility-id, 'FullRepresentation')]")))
        self.assertRaises((NoSuchElementException, WebDriverException), self.driver.find_element, AppiumBy.XPATH, f"//notification[starts-with(@name, '{summary}')]")

        send_notification({
            "app_name": "Appium Test",
            "summary": "CriticalInDndMode",
            "hints": {
                "desktop-entry": GLib.Variant("s", "org.kde.plasmashell"),
                "urgency": GLib.Variant("i", 2),
            },
        })
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//notification[@name='CriticalInDndMode']")))

        # Disable DnD — suppressed notifications appear as "Unread Notifications" summary
        self.driver.find_element(AppiumBy.NAME, "Do not disturb").click()
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//notification[@name='Unread Notifications' and @description='2 notifications were received while Do Not Disturb was active.  from Notification Manager']")))
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//button[@name='Close' and contains(@accessibility-id, 'NotificationPopup')]"))).click()

        # Toggle DnD on and off again — verify the summary is not shown for the same notifications
        self.driver.find_element(AppiumBy.NAME, "Do not disturb").click()
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//*[@name='Do not disturb' and contains(@states, 'checked')]")))
        self.driver.find_element(AppiumBy.NAME, "Do not disturb").click()
        self.assertRaises((NoSuchElementException, WebDriverException), self.driver.find_element, AppiumBy.XPATH, f"//notification[starts-with(@name, '{summary}')]")
        self.assertRaises((NoSuchElementException, WebDriverException), self.driver.find_element, AppiumBy.XPATH, "//notification[@name='Unread Notifications' and @description='2 notifications were received while Do Not Disturb was active.  from Notification Manager']")

        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Clear All Notifications"))).click()
        wait.until(EC.invisibility_of_element(title))


if __name__ == '__main__':
    unittest.main()
