#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import sys
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
from notificationstest.jobnotificationinterface import JobNotificationControlInterface


class TestJobNotification(NotificationsTestBase):

    def _set_progress(self, session_bus: Gio.DBusConnection, bytes_processed: int):
        session_bus.call_sync(
            JobNotificationControlInterface.BUS_NAME, JobNotificationControlInterface.OBJECT_PATH,
            JobNotificationControlInterface.BUS_NAME, "SetProcessedAmount",
            GLib.Variant("(t)", [bytes_processed]),
            None, Gio.DBusSendMessageFlags.NONE, 1000,
        )

    def test_0_job_notification(self) -> None:
        """
        https://bugs.kde.org/show_bug.cgi?id=501991
        """
        try:
            from PySide6.QtCore import QObject  # noqa: F401
        except ModuleNotFoundError:
            self.skipTest("PySide is not available")

        wait = WebDriverWait(self.driver, 10, ignored_exceptions=(NoSuchElementException, WebDriverException))

        subprocess_env: dict[str, str] = os.environ.copy()
        if "KDECI_BUILD" in os.environ:
            subprocess_env["LD_PRELOAD"] = subprocess.check_output(["gcc", "-print-file-name=libasan.so"]).strip().decode(encoding="utf-8")
        proc = subprocess.Popen(
            ["python3", os.path.join(os.path.dirname(os.path.abspath(__file__)), "jobnotification.py")],
            stdout=sys.stderr, stderr=sys.stderr, env=subprocess_env,
        )
        self.addCleanup(proc.kill)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "10%")))
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//notification[contains(@description, '1 file')]")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Pause"))).click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Resume"))).click()
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//button[@name='Details' and contains(@accessibility-id, 'NotificationPopup')]"))).click()

        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
        size = 1024 * 1024

        self._set_progress(session_bus, size * 2)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "20%")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "8 s remaining")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "1.0 MiB/s")))
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//chart[contains(@accessibility-id, 'LineChart')]")))

        self._set_progress(session_bus, size * 5)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "50%")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "2 s remaining")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "3.0 MiB/s")))

        self._set_progress(session_bus, size * 10)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Job Finished")))
        wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//notification[contains(@description, 'file:///home/foobar/test.txt')]")))
        proc.wait(10)


if __name__ == '__main__':
    unittest.main()
