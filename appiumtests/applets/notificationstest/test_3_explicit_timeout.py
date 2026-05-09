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


class TestExplicitTimeout(NotificationsTestBase):

    def test_0_notification_with_explicit_timeout(self) -> None:
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
        WebDriverWait(self.driver, 5).until(EC.invisibility_of_element(element))


if __name__ == '__main__':
    unittest.main()
