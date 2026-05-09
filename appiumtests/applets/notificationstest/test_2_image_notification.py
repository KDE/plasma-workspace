#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
import logging
import os
import sys
import tempfile
import unittest

_applets_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)
_appiumtests_dir = os.path.join(_applets_dir, os.pardir)
sys.path.insert(0, _appiumtests_dir)
sys.path.insert(0, _applets_dir)

from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gdk, GdkPixbuf, GLib
from selenium.common.exceptions import WebDriverException
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

from notificationstest.base import NotificationsTestBase
from utils.notification_helpers import send_notification


class TestNotificationWithImage(NotificationsTestBase):

    def test_0_notification_with_image(self) -> None:
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

            def match_image(driver) -> bool:
                try:
                    self.driver.find_image_occurrence(self.take_screenshot(), partial_image)
                    return True
                except WebDriverException:
                    return False

            WebDriverWait(self.driver, 10).until(match_image)
            self.close_notifications()


if __name__ == '__main__':
    unittest.main()
