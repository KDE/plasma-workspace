#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import base64
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
from selenium.webdriver.support.ui import WebDriverWait

from notificationstest.base import NotificationsTestBase
from utils.notification_helpers import send_notification


class TestThumbnail(NotificationsTestBase):

    def test_0_thumbnail(self) -> None:
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
            partial_pixbuf.fill(colors[1])
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
