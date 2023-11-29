#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-FileCopyrightText: 2023 Arjen Hiemstra <ahiemstra@heimr.nl>
# SPDX-License-Identifier: MIT

import unittest
from typing import Final
import sys
import argparse

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy


parser = argparse.ArgumentParser(description = "Performs an appium based smoke test of a plasmoid")
parser.add_argument("--plasmoid", dest = "plasmoid_id", type = str, help = "The id of the plasmoid to test", required = True)
parser.add_argument("--find-names", dest = "find_names", metavar = "NAME", nargs = "+", type = str, help = "Try to find these names to verify the plasmoid loaded correctly")
args, remaining_argv = parser.parse_known_args(sys.argv)


class IconTest(unittest.TestCase):
    """
    Tests for the icon widget
    """

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {args.plasmoid_id}")
        options.set_capability("environ", {
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false",
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def tearDown(self) -> None:
        """
        Take screenshot when the current test fails
        """
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_{args.plasmoid_id}_#{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.quit()

    def test_find_name(self) -> None:
        """
        Tests the widget can be opened
        """
        for name in args.find_names:
            self.driver.find_element(AppiumBy.NAME, name)


if __name__ == '__main__':
    unittest.main(argv = remaining_argv)
