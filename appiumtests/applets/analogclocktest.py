#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import unittest
from typing import Final

from appium.webdriver.common.appiumby import AppiumBy

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils.base_test import PlasmaAppletTest

WIDGET_ID: Final = "org.kde.plasma.analogclock"


class AnalogClockTest(PlasmaAppletTest):
    """
    Tests for the analog clock widget
    """

    widget_id = WIDGET_ID
    extra_environ = {
        "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;qt.qml.propertyCache.append.warning=false",
    }

    def test_0_open(self) -> None:
        """
        Tests the widget can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "Analog Clock")


if __name__ == '__main__':
    unittest.main()
