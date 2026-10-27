#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import sys
import tempfile
import unittest

from typing import Optional

_applets_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)
_appiumtests_dir = os.path.join(_applets_dir, os.pardir)
sys.path.insert(0, _appiumtests_dir)
sys.path.insert(0, _applets_dir)

from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gio, GLib
from selenium.common.exceptions import NoSuchElementException, TimeoutException, WebDriverException
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

from notificationstest.base import NotificationsTestBase
from notificationstest.jobnotificationinterface import JobNotificationControlInterface


class _ProcessAliveOrElementPresent:
    """Custom WebDriverWait condition that also checks if a subprocess is still alive.

    When the jobnotification.py process exits unexpectedly, Plasma's notification manager
    detects the D-Bus service unregistration and terminates the job with
    "Application closed unexpectedly", which hides the job action buttons (Details, Pause,
    Cancel). This causes the test to hang for the full WebDriverWait timeout before failing
    with an unhelpful TimeoutException.

    This condition polls both the process status and the UI element, so the test fails
    immediately with a descriptive error if the subprocess has crashed.
    """

    def __init__(self, proc: subprocess.Popen, by: str, value: str, stderr_file: Optional[str] = None):
        self._proc = proc
        self._by = by
        self._value = value
        self._stderr_file = stderr_file

    def __call__(self, driver):
        ret = self._proc.poll()
        if ret is not None:
            stderr_output = ""
            if self._stderr_file:
                try:
                    with open(self._stderr_file, "r") as f:
                        stderr_output = f.read().strip()
                except OSError:
                    pass
            msg = f"jobnotification.py process exited unexpectedly with return code {ret}"
            if stderr_output:
                msg += f":\n{stderr_output}"
            raise AssertionError(msg)
        try:
            element = driver.find_element(self._by, self._value)
            return element
        except NoSuchElementException:
            return False


class TestJobNotification(NotificationsTestBase):

    def _set_progress(self, session_bus: Gio.DBusConnection, bytes_processed: int):
        session_bus.call_sync(
            JobNotificationControlInterface.BUS_NAME, JobNotificationControlInterface.OBJECT_PATH,
            JobNotificationControlInterface.BUS_NAME, "SetProcessedAmount",
            GLib.Variant("(t)", [bytes_processed]),
            None, Gio.DBusSendMessageFlags.NONE, 1000,
        )

    def _wait_until(self, wait: WebDriverWait, condition, stderr_file: str):
        """Run wait.until() but include the subprocess stderr in the timeout error.

        When the jobnotification.py subprocess stays alive but produces no UI
        (e.g. it fails to connect to the D-Bus session), the element never
        appears and WebDriverWait raises a bare TimeoutException with no
        context. Reading the stderr file makes the failure diagnosable.
        """
        try:
            return wait.until(condition)
        except TimeoutException:
            stderr_output = ""
            try:
                with open(stderr_file, "r") as f:
                    stderr_output = f.read().strip()
            except OSError:
                pass
            msg = "Timed out waiting for element"
            if stderr_output:
                msg += f"; jobnotification.py stderr:\n{stderr_output}"
            raise AssertionError(msg) from None

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
        # The subprocess creates a QGuiApplication via PySide6. Because Qt is
        # built with ASAN instrumentation while PySide6 is not, the subprocess
        # triggers a bad-free false positive in
        # QtWaylandClient::QWaylandIntegration::reconfigureInputContext().
        # This cannot be suppressed (see clipboarddeadlocktest.py), so
        # halt_on_error lets ASAN report it without aborting the subprocess.
        # The memory is actually owned by the Qt/PySide6 object, so the free is
        # harmless.
        subprocess_env["ASAN_OPTIONS"] = "new_delete_type_mismatch=0:detect_odr_violation=0:stack-use-after-scope=0:halt_on_error=0:exitcode=0:verify_asan_link_order=0:detect_leaks=0"

        # Capture stderr to a temporary file so we can include it in failure
        # messages when the subprocess exits unexpectedly (e.g. crashes).
        stderr_file = tempfile.NamedTemporaryFile(mode="w+", suffix=".stderr", delete=False, prefix="jobnotification_")
        stderr_file.close()

        stderr_fh = open(stderr_file.name, "w")
        proc = subprocess.Popen(
            ["python3", os.path.join(os.path.dirname(os.path.abspath(__file__)), "jobnotification.py")],
            stdout=subprocess.DEVNULL, stderr=stderr_fh, env=subprocess_env,
        )
        self.addCleanup(proc.kill)
        self.addCleanup(stderr_fh.close)
        self.addCleanup(os.unlink, stderr_file.name)

        # Check that the subprocess is alive before waiting for UI elements.
        # If jobnotification.py crashes (e.g. missing KCoreAddons/KJobWidgets),
        # report the stderr output immediately instead of timing out.
        self._wait_until(wait, _ProcessAliveOrElementPresent(proc, AppiumBy.NAME, "10%", stderr_file.name), stderr_file.name)
        self._wait_until(wait, _ProcessAliveOrElementPresent(proc, AppiumBy.XPATH, "//notification[contains(@description, '1 file')]", stderr_file.name), stderr_file.name)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Pause"))).click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Resume"))).click()
        self._wait_until(wait, _ProcessAliveOrElementPresent(proc, AppiumBy.XPATH, "//button[@name='Details' and contains(@accessibility-id, 'NotificationPopup')]", stderr_file.name), stderr_file.name).click()

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
