#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import shutil
import subprocess
import sys
import tempfile
from typing import Final

import gi

gi.require_version('Gdk', '4.0')
gi.require_version('GdkPixbuf', '2.0')
from gi.repository import Gdk, Gio, GLib
from kicker.favoritetest import start_kactivitymanagerd

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils.base_test import PlasmaAppletTest

WIDGET_ID: Final = "org.kde.plasma.notifications"
BUS_NAME: Final = "org.freedesktop.Notifications"
OBJECT_PATH: Final = "/org/freedesktop/Notifications"
IFACE_NAME: Final = BUS_NAME


class NotificationsTestBase(PlasmaAppletTest):
    """
    Base class for notification widget tests
    """

    widget_id = WIDGET_ID
    extra_environ = {
        "QT_LOGGING_RULES": "kf.notification*.debug=true;org.kde.plasma.notificationmanager.debug=true",
    }

    notification_proxy: Gio.DBusProxy
    kactivitymanagerd: subprocess.Popen

    @classmethod
    def setUpClass(cls) -> None:
        cls.kactivitymanagerd = start_kactivitymanagerd()

        os.makedirs(os.path.join(GLib.get_user_data_dir(), "knotifications6"))
        shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, os.pardir, "libnotificationmanager", "libnotificationmanager.notifyrc"), os.path.join(GLib.get_user_data_dir(), "knotifications6"))

        super().setUpClass()

        cls.notification_proxy = Gio.DBusProxy.new_for_bus_sync(Gio.BusType.SESSION, 0, None, BUS_NAME, OBJECT_PATH, IFACE_NAME)

    @classmethod
    def tearDownClass(cls) -> None:
        super().tearDownClass()
        cls.kactivitymanagerd.kill()
        cls.kactivitymanagerd.wait(10)

    def close_notifications(self) -> None:
        from selenium.common.exceptions import WebDriverException
        from selenium.webdriver.support import expected_conditions as EC
        from selenium.webdriver.support.ui import WebDriverWait

        from appium.webdriver.common.appiumby import AppiumBy

        wait = WebDriverWait(self.driver, 5)
        for button in self.driver.find_elements(AppiumBy.XPATH, "//button[@name='Close']"):
            try:
                button.click()
                wait.until(EC.invisibility_of_element(button))
            except WebDriverException:
                pass

    def take_screenshot(self) -> str:
        import base64

        with tempfile.TemporaryDirectory() as temp_dir:
            saved_image_path = os.path.join(temp_dir, "tray.png")
            self.driver.get_screenshot_as_file(saved_image_path)
            return base64.b64encode(Gdk.Texture.new_from_filename(saved_image_path).save_to_png_bytes().get_data()).decode()
