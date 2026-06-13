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


class TestPlaybackControls(MediaControllerTestBase):

    def test_0_play_pause(self) -> None:
        """
        Play/Pause toggle should change the playback state.
        """
        player = self.start_player("player_a.json", start_index=0)
        try:
            wait = WebDriverWait(self.driver, 10)
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play"))).click()
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Pause")))
            self.driver.find_element(AppiumBy.NAME, "Pause").click()
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))
        finally:
            self.stop_player(player)

    def test_1_next_previous(self) -> None:
        """
        Next and Previous track buttons should navigate through the playlist.
        """
        player = self.start_player("player_a.json", start_index=0)
        try:
            wait = WebDriverWait(self.driver, 10)
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Katie's Favorite")))
            self.driver.find_element(AppiumBy.NAME, "Next Track").click()
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Konqi \u2764\ufe0f\ufe0f Katie")))
            self.driver.find_element(AppiumBy.NAME, "Next Track").click()
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Konqi's Favorite")))
            self.driver.find_element(AppiumBy.NAME, "Previous Track").click()
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Konqi \u2764\ufe0f\ufe0f Katie")))
            self.driver.find_element(AppiumBy.NAME, "Previous Track").click()
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Katie's Favorite")))
        finally:
            self.stop_player(player)


if __name__ == '__main__':
    unittest.main()