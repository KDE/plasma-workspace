#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import subprocess
import time
import unittest

import dbus
import dbusmock
import os

from pathlib import Path

from typing import Final

from selenium.webdriver.common.keys import Keys

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy

WIDGET_ID: Final = "org.kde.plasma.devicenotifier"
KDE_VERSION: Final = 6

UDISKS2_TEMPLATE_PATH: Final = "devicenotifiertest/udisks2_mock.py"

class DeviceNotifierTest(dbusmock.DBusTestCase):
    """
    Tests for the device notifier widget
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget, starts a system mock dbus session and initialize the webdriver
        """

        cls.start_system_bus()
        cls.dbus_con = cls.get_dbus(system_bus=True)

        template_path = Path(__file__).parent.resolve() / UDISKS2_TEMPLATE_PATH

        (cls.p_mock, cls.obj_udisks2) = cls.spawn_server_template(
            str(template_path),
            {},
            stdout=subprocess.PIPE,
        )

        cls.dbusmock = dbus.Interface(cls.obj_udisks2, dbusmock.MOCK_IFACE)

        print("DBUS_SYSTEM_BUS_ADDRESS=", os.environ["DBUS_SYSTEM_BUS_ADDRESS"])

        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
            "DBUS_SYSTEM_BUS_ADDRESS": os.environ["DBUS_SYSTEM_BUS_ADDRESS"],
            "QT_LOGGING_RULES": "org.kde.applets.devicenotifier.debug=true",
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
        subprocess.check_call([f"kquitapp{KDE_VERSION}", "plasmawindowed"])
        for _ in range(10):
            try:
                subprocess.check_call(["pidof", "plasmawindowed"])
            except subprocess.CalledProcessError:
                break
            time.sleep(1)
        cls.driver.quit()

        cls.p_mock.stdout.close()
        cls.p_mock.terminate()
        cls.p_mock.wait()

    def test_0_open(self) -> None:
        """
        Tests the widget can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "No removable devices attached")

    def test_1_show_devices(self) -> None:
        """
        Tests the widget can show devices
        """
        self.dbusmock.add_mock_device("sda", 1, 1000000000, False)
        self.driver.find_element(AppiumBy.NAME, "sda1")

        self.dbusmock.add_mock_device("sdb", 1, 1000000000, False)
        self.driver.find_element(AppiumBy.NAME, "sdb1")

        self.dbusmock.add_mock_device("sdc", 1, 1000000000, False)
        self.driver.find_element(AppiumBy.NAME, "sdc1")

        mount_and_open_elements = self.driver.find_elements(AppiumBy.NAME, "Mount and Open")
        self.assertNotEqual(len(mount_and_open_elements), 3)

        expand_elements = self.driver.find_elements(AppiumBy.NAME, "Expand")
        self.assertNotEqual(len(expand_elements), 3)

        for expand_element in expand_elements:
            expand_element.click()

        mount = self.driver.find_elements(AppiumBy.NAME, "Mount without verifying")
        self.assertNotEqual(len(mount), 3)

    def test_2_remove_devices(self) -> None:
        """
        Tests the widget can remove devices
        """

        self.dbusmock.remove_mock_device("sda1")
        self.dbusmock.remove_mock_device("sdb1")
        self.dbusmock.remove_mock_device("sdc1")

        self.driver.find_element(AppiumBy.NAME, "No removable devices attached")

    def test_3_mount_device(self) -> None:
        """
        Tests the widget can mount a device
        """

        self.dbusmock.add_mock_device("sda", 1, 1000000000, False)
        expand_element = self.driver.find_element(AppiumBy.NAME, "Expand")
        expand_element.click()
        mount_element = self.driver.find_element(AppiumBy.NAME, "Mount without verifying")
        mount_element.click()
        self.driver.find_element(AppiumBy.NAME, "Safely remove")

    def test_4_unmount_device(self) -> None:
        """
        Tests the widget can unmount a device
        """

        expand_element = self.driver.find_element(AppiumBy.NAME, "Safely remove")
        expand_element.send_keys(Keys.ENTER)
        self.driver.find_element(by="description", value="This device can now be safely removed.")
        self.driver.find_element(AppiumBy.NAME, "No removable devices attached")

    def test_5_check_device(self) -> None:
        """
        Tests the widget can check a device
        """

        self.dbusmock.add_mock_device("sdb", 1, 1000000000, True)
        expand_element = self.driver.find_element(AppiumBy.NAME, "Expand")
        expand_element.click()

        mount_element = self.driver.find_element(AppiumBy.NAME, "Check for Errors")
        mount_element.click()

        self.driver.find_element(by="description", value="This device has file system errors.")

    def test_6_fix_device(self) -> None:
        """
        Tests the widget can fix a device
        """

        fix_element = self.driver.find_element(AppiumBy.NAME, "Try to Fix")
        fix_element.send_keys(Keys.ENTER)

        self.driver.find_element(by="description", value="Successfully repaired!")

if __name__ == '__main__':
    unittest.main()
