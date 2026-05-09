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
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

from notificationstest.base import NotificationsTestBase
from utils.notification_helpers import send_notification


class TestMalformedImageData(NotificationsTestBase):

    def test_0_bug481033_malformed_image_data(self) -> None:
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


if __name__ == '__main__':
    unittest.main()
