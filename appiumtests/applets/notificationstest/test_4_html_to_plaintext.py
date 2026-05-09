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
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

from notificationstest.base import NotificationsTestBase
from utils.notification_helpers import send_notification


class TestHtmlToPlaintext(NotificationsTestBase):

    def test_0_accessible_description_html_to_plaintext(self) -> None:
        """
        accessibleDescription provides the plain text of the description
        """
        send_notification({
            "app_name": "Appium Test",
            "body": "<b>b</b><i>i</i><u>u</u><blink>blink</blink><a href=\"https://www.example.org/\">www.example.org</a>",
        })
        wait = WebDriverWait(self.driver, 5)
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//*[@description='biublinkwww.example.org  from Appium Test']")))
        self.close_notifications()


if __name__ == '__main__':
    unittest.main()
