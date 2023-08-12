#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# pylint: disable=too-many-arguments

import ctypes
import json
import sys
import unittest
from os import getcwd, path
from time import sleep
from typing import Any

from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import GLib
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait
from utils.mediaplayer import (Mpris2, read_base_properties, read_player_metadata, read_player_properties)

CMAKE_BINARY_DIR: str = ""


class MediaControllerTests(unittest.TestCase):
    """
    Tests for the media controller widget
    """

    driver: webdriver.Remote
    mpris_interface: Mpris2 | None
    touch_input_iface: ctypes.CDLL

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        desired_caps: dict[str, Any] = {}
        desired_caps["app"] = "plasmawindowed -p org.kde.plasma.nano org.kde.plasma.mediacontroller"
        desired_caps["timeouts"] = {'implicit': 5000}
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', desired_capabilities=desired_caps)
        cls.driver.implicitly_wait = 10

        if path.exists(path.join(CMAKE_BINARY_DIR, "libtouchinputhelper.so")):
            cls.touch_input_iface = ctypes.cdll.LoadLibrary(path.join(CMAKE_BINARY_DIR, "libtouchinputhelper.so"))
        else:
            cls.touch_input_iface = ctypes.cdll.LoadLibrary("libtouchinputhelper.so")

        # This also initializes GLib.MainLoop as Qt also uses it
        # An instance can only have one GLib.MainLoop, so no need to add one manually
        cls.touch_input_iface.init_application()

    @classmethod
    def tearDownClass(cls) -> None:
        cls.touch_input_iface.unload_application()

    def setUp(self) -> None:
        json_path: str = path.join(getcwd(), "resources/player_a.json")
        with open(json_path, "r", encoding="utf-8") as f:
            json_dict: dict[str, list | dict] = json.load(f)
        metadata: list[dict[str, GLib.Variant]] = read_player_metadata(json_dict)
        base_properties: dict[str, GLib.Variant] = read_base_properties(json_dict)
        current_index: int = 1
        player_properties: dict[str, GLib.Variant] = read_player_properties(json_dict, metadata[current_index])

        self.mpris_interface = Mpris2(metadata, base_properties, player_properties, current_index)

    def tearDown(self) -> None:
        self.mpris_interface.quit()
        self.mpris_interface = None
        WebDriverWait(self.driver, 5, 0.2).until(EC.presence_of_element_located((AppiumBy.NAME, "No media playing")))

    def test_track(self) -> None:
        """
        Tests the widget can show track metadata
        """
        assert self.mpris_interface
        play_button = self.driver.find_element(by=AppiumBy.NAME, value="Play")
        previous_button = self.driver.find_element(by=AppiumBy.NAME, value="Previous Track")
        next_button = self.driver.find_element(by=AppiumBy.NAME, value="Next Track")
        shuffle_button = self.driver.find_element(by=AppiumBy.NAME, value="Shuffle")
        repeat_button = self.driver.find_element(by=AppiumBy.NAME, value="Repeat")

        # Match song title, artist and album
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())  # Title
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:album"].get_string())  # Album
        self.driver.find_element(by=AppiumBy.NAME, value="0:00")  # Current position
        self.driver.find_element(by=AppiumBy.NAME, value="-5:00")  # Remaining time
        self.driver.find_element(by=AppiumBy.NAME,
                                 value=', '.join(self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:artist"].unpack()))  # Artists

        # Now click the play button
        wait: WebDriverWait = WebDriverWait(self.driver, 5)
        play_button.click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Pause")))
        play_button.click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))

        # Now click the shuffle button
        shuffle_button.click()
        wait.until(lambda _: self.mpris_interface.player_properties["Shuffle"].get_boolean())
        # Click again to disable shuffle
        shuffle_button.click()
        wait.until(lambda _: not self.mpris_interface.player_properties["Shuffle"].get_boolean())

        # Now click the repeat button
        repeat_button.click()
        wait.until(lambda _: self.mpris_interface.player_properties["LoopStatus"].get_string() == "Playlist")
        # Click again to switch to Track mode
        repeat_button.click()
        wait.until(lambda _: self.mpris_interface.player_properties["LoopStatus"].get_string() == "Track")
        # Click again to disable repeat
        repeat_button.click()
        wait.until(lambda _: self.mpris_interface.player_properties["LoopStatus"].get_string() == "None")

        # Switch to the previous song, and match again
        previous_button.click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Katie's Favorite")))
        self.assertFalse(previous_button.is_enabled())
        self.assertTrue(next_button.is_enabled())
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:album"].get_string())
        self.driver.find_element(by=AppiumBy.NAME, value="0:00")
        self.driver.find_element(by=AppiumBy.NAME, value="-10:00")
        self.driver.find_element(by=AppiumBy.NAME, value=', '.join(self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:artist"].unpack()))

        # Switch to the next song (need to click twice), and match again
        next_button.click()
        next_button.click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Konqi's Favorite")))
        self.assertTrue(previous_button.is_enabled())
        self.assertFalse(next_button.is_enabled())
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:album"].get_string())
        self.driver.find_element(by=AppiumBy.NAME, value="0:00")
        self.driver.find_element(by=AppiumBy.NAME, value="-15:00")
        self.driver.find_element(by=AppiumBy.NAME, value=', '.join(self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:artist"].unpack()))

    def test_touch_gestures(self) -> None:
        """
        Tests touch gestures like swipe up/down/left/right to adjust volume/progress
        @see https://invent.kde.org/plasma/plasma-workspace/-/merge_requests/2438
        """
        assert self.mpris_interface
        wait: WebDriverWait = WebDriverWait(self.driver, 5)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "0:00")))  # Current position
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "-5:00")))  # Remaining time

        # Initialize TasksModel and fake_input
        self.touch_input_iface.init_task_manager()
        self.touch_input_iface.init_fake_input()

        self.assertGreater(self.touch_input_iface.get_task_count(), 0)

        self.touch_input_iface.maximize_window(0)
        sleep(1)  # Window animation

        # Get the geometry of the widget window
        self.touch_input_iface.get_window_rect.restype = ctypes.POINTER(ctypes.c_int * 4)
        rect = self.touch_input_iface.get_window_rect(0).contents  # x,y,width,height
        self.assertGreater(rect[2], 1, f"Invalid width {rect[2]}")
        self.assertGreater(rect[3], 1, f"Invalid height {rect[3]}")

        # Get the geometry of the virtual screen
        self.touch_input_iface.get_screen_rect.restype = ctypes.POINTER(ctypes.c_int * 4)
        screen_rect = self.touch_input_iface.get_screen_rect(0).contents  # x,y,width,height
        self.assertEqual(screen_rect[0], 0)
        self.assertEqual(screen_rect[1], 0)
        self.assertGreater(screen_rect[2], 1, f"Invalid screen width {rect[2]}")
        self.assertGreater(screen_rect[3], 1, f"Invalid screen height {rect[3]}")

        # Get the center point
        center_pos_x: int = rect[1] + int(rect[3] / 2)
        center_pos_y: int = rect[0] + int(rect[2] / 2)
        move_distance_x: int = screen_rect[2] - int(rect[2] / 2)
        move_distance_y: int = screen_rect[3] - int(rect[3] / 2)

        # Touch the window
        self.touch_input_iface.touch_down(center_pos_x, center_pos_y)
        self.touch_input_iface.touch_up()

        # Swipe right -> Position++
        self.touch_input_iface.touch_down(center_pos_x, center_pos_y)
        [self.touch_input_iface.touch_move(int(center_pos_x + distance), center_pos_y) for distance in range(1, move_distance_x)]
        self.touch_input_iface.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Position"].get_int64() > 0)

        # Swipe left -> Position--
        old_position: int = self.mpris_interface.player_properties["Position"].get_int64()
        self.touch_input_iface.touch_down(center_pos_x, center_pos_y)
        [self.touch_input_iface.touch_move(center_pos_x - distance, center_pos_y) for distance in range(1, move_distance_x)]
        self.touch_input_iface.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Position"].get_int64() < old_position)

        # Swipe down: Volume--
        self.touch_input_iface.touch_down(center_pos_x, center_pos_y)
        [self.touch_input_iface.touch_move(center_pos_x, center_pos_y + distance) for distance in range(1, move_distance_y)]
        self.touch_input_iface.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Volume"].get_double() < 1.0)

        # Swipe up: Volume++
        old_volume: float = self.mpris_interface.player_properties["Volume"].get_double()
        self.touch_input_iface.touch_down(center_pos_x, center_pos_y)
        [self.touch_input_iface.touch_move(center_pos_x, center_pos_y - distance) for distance in range(1, move_distance_y)]
        self.touch_input_iface.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Volume"].get_double() > old_volume)

        # Swipe down and then swipe right, only volume should change
        old_volume = self.mpris_interface.player_properties["Volume"].get_double()
        old_position = self.mpris_interface.player_properties["Position"].get_int64()
        self.touch_input_iface.touch_down(center_pos_x, center_pos_y)
        [self.touch_input_iface.touch_move(center_pos_x, center_pos_y + distance) for distance in range(1, move_distance_y)]  # Swipe down
        sleep(0.5)  # Qt may ignore some touch events if there are too many, so explicitly wait a moment
        [self.touch_input_iface.touch_move(center_pos_x + distance, center_pos_y + move_distance_y - 1)
         for distance in range(1, move_distance_x)]  # Swipe right
        self.touch_input_iface.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Volume"].get_double() < old_volume)
        self.assertEqual(old_position, self.mpris_interface.player_properties["Position"].get_int64())

        # Swipe right and then swipe up, only position should change
        old_volume = self.mpris_interface.player_properties["Volume"].get_double()
        old_position = self.mpris_interface.player_properties["Position"].get_int64()
        self.touch_input_iface.touch_down(center_pos_x, center_pos_y)
        [self.touch_input_iface.touch_move(center_pos_x + distance, center_pos_y) for distance in range(1, move_distance_x)]  # Swipe right
        sleep(0.5)
        [self.touch_input_iface.touch_move(center_pos_x + move_distance_x - 1, center_pos_y - distance) for distance in range(1, move_distance_y)]  # Swipe up
        self.touch_input_iface.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Position"].get_int64() > old_position)
        self.assertAlmostEqual(old_volume, self.mpris_interface.player_properties["Volume"].get_double())


if __name__ == '__main__':
    assert len(sys.argv) >= 2, f"Missing CMAKE_CURRENT_BINARY_DIR argument {len(sys.argv)}"
    CMAKE_BINARY_DIR = sys.argv.pop()
    unittest.main()
