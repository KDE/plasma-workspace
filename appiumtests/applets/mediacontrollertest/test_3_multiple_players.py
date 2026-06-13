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


class TestMultiplePlayers(MediaControllerTestBase):

    def test_0_two_players(self) -> None:
        """
        Two simultaneous MPRIS2 players should both be accessible.
        """
        player_a = self.start_player("player_a.json", start_index=0)
        wait = WebDriverWait(self.driver, 10)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Katie's Favorite")))

        player_b = self.start_player("player_b.json")
        try:
            # Switch to the second player via the player selector
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Audacious"))).click()
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Clarinet Quintet")))
        finally:
            self.stop_player(player_a)
            self.stop_player(player_b)


if __name__ == '__main__':
    unittest.main()