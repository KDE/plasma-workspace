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


class TestBasicPlayback(MediaControllerTestBase):

    def test_0_single_player_with_metadata(self) -> None:
        """
        A single MPRIS2 player should appear in the media controller
        with correct track title and artist metadata.
        """
        player = self.start_player("player_a.json", start_index=0)
        try:
            wait = WebDriverWait(self.driver, 10)
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Katie's Favorite")))
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "KDE Community")))
        finally:
            self.stop_player(player)

    def test_1_different_player_identity(self) -> None:
        """
        A player with a different identity and single track appears correctly.
        """
        player = self.start_player("player_b.json")
        try:
            wait = WebDriverWait(self.driver, 10)
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Audacious")))
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Clarinet Quintet")))
        finally:
            self.stop_player(player)


if __name__ == '__main__':
    unittest.main()