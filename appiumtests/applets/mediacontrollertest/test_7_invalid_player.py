#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import sys
import unittest

_applets_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)
_appiumtests_dir = os.path.join(_applets_dir, os.pardir)
sys.path.insert(0, _appiumtests_dir)
sys.path.insert(0, _applets_dir)

from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

from mediacontrollertest.base import MediaControllerTestBase


class TestInvalidPlayer(MediaControllerTestBase):

    def test_0_invalid_mpris2_interface(self) -> None:
        """
        Invalid MPRIS2 interfaces that return errors should not crash the widget.
        """
        player = self.start_invalid_player()
        try:
            wait = WebDriverWait(self.driver, 5)
            wait.until(EC.presence_of_element_located((AppiumBy.XPATH, "//*")))
        finally:
            self.stop_player(player)


if __name__ == '__main__':
    unittest.main()