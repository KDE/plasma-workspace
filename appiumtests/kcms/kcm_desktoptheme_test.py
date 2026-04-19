#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import unittest
from typing import Final

from appium.webdriver.common.appiumby import AppiumBy

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils.base_test import KCMTest

KCM_ID: Final = "kcm_desktoptheme"


class KCMDesktopThemeTest(KCMTest):
    """
    Tests for kcm_desktoptheme
    """

    kcm_id = KCM_ID

    def test_0_open(self) -> None:
        """
        Tests the KCM can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "All themes")
        self.driver.find_element(AppiumBy.NAME, "Breeze Light")


if __name__ == '__main__':
    unittest.main()
