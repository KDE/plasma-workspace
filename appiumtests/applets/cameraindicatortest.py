#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import sys
import unittest
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

WIDGET_ID: Final = "org.kde.plasma.cameraindicator"


class CameraIndicatorTest(unittest.TestCase):
    """
    Tests for the camera indicator widget
    """

    driver: webdriver.Remote
    pipewire_already_running_before_test: bool = False
    process_list: list[subprocess.Popen] = []

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false;kpipewire_logging.warning=false",
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

        cls.pipewire_already_running_before_test = subprocess.Popen(["pidof", "pipewire"]).wait() == 0

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_cameraindicatortest_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        assert len(cls.process_list) > 0, len(cls.process_list)
        for p in reversed(cls.process_list):
            p.kill()
            p.wait(10)
        cls.driver.quit()

    def test_0_open(self) -> None:
        """
        Tests the widget can be opened
        """
        if self.pipewire_already_running_before_test:
            self.driver.find_element(AppiumBy.NAME, "No camera is in use")
        else:
            self.driver.find_element(AppiumBy.NAME, "Camera indicator is unavailable")

    def test_1_connect_to_pipewire(self) -> None:
        """
        Tests the widget can connect to pipewire
        """
        if self.pipewire_already_running_before_test:
            self.skipTest("Pipewire is already running.")

        self.process_list.append(subprocess.Popen(["pipewire"], stdout=sys.stderr, stderr=sys.stderr))
        self.process_list.append(subprocess.Popen(["wireplumber"], stdout=sys.stderr, stderr=sys.stderr))

        # Reconnecting takes at least 5s
        WebDriverWait(self.driver, 20).until(EC.presence_of_element_located((AppiumBy.NAME, "No camera is in use")))

    def test_2_camera_stream(self) -> None:
        """
        Camera status is reported in the widget
        """
        # Open two mock cameras to show the camera list
        bin_dir = os.getenv("CMAKE_RUNTIME_OUTPUT_DIRECTORY", os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "build", "bin"))
        mock_camera1 = subprocess.Popen([os.path.join(bin_dir, "pipewiremockcamera")], stdout=subprocess.PIPE, stderr=sys.stderr)
        mock_camera2 = subprocess.Popen([os.path.join(bin_dir, "pipewiremockcamera")], stdout=sys.stderr, stderr=sys.stderr)
        self.process_list.append(mock_camera1)
        self.process_list.append(mock_camera2)
        self.driver.find_element(AppiumBy.NAME, "KDE Camera")
        self.driver.find_element("description", "Suspended")

        # Get the node id of the first mock camera
        node_id: str = ""
        PREFIX: Final = "node id: "
        while True:
            line = mock_camera1.stdout.readline().decode().strip()
            if line.startswith(PREFIX):
                node_id = line[len(PREFIX):]
                break
        assert int(node_id) > 0, node_id

        # Open the camera player and connect to the first mock camera
        camera_player = subprocess.Popen([os.path.join(bin_dir, "pipewirecameraplay"), node_id], stdout=sys.stderr, stderr=sys.stderr)
        self.process_list.append(camera_player)
        self.driver.find_element("description", "Active")

        # Show the placeholder message when there is only one camera
        mock_camera2.terminate()
        self.driver.find_element(AppiumBy.NAME, "A camera is in use")


if __name__ == '__main__':
    unittest.main()
