#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# pylint: disable=too-many-arguments

import ctypes
import json
import subprocess
import sys
from tempfile import NamedTemporaryFile
import unittest
from os import getcwd, path
from time import sleep
from typing import Any

from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gio, GLib
from selenium.webdriver.remote.webelement import WebElement
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
    player_b: subprocess.Popen | None = None
    player_browser: subprocess.Popen | None = None
    player_plasma_browser_integration: subprocess.Popen | None = None
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

    def test_multiplexer(self) -> None:
        """
        The multiplexer should be hidden when there is only 1 player, and shows information from the active/playing player if there is one
        """
        self.addCleanup(self._cleanup_multiplexer)

        # Wait until the first player is ready
        wait: WebDriverWait = WebDriverWait(self.driver, 3)
        wait.until(
            EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))

        # Start Player B, Total 2 players
        player_b_json_path: str = path.join(getcwd(), "resources/player_b.json")
        self.player_b = subprocess.Popen(("python3", path.join(getcwd(), "utils/mediaplayer.py"), player_b_json_path))
        player_selector: WebElement = wait.until(EC.visibility_of_element_located((AppiumBy.ACCESSIBILITY_ID, "playerSelector")))

        # Find player tabs based on "Identity"
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "AppiumTest")))

        # Make sure the current index does not change after a new player appears
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())  # Title

        # Switch to Player B
        self.driver.find_element(by=AppiumBy.NAME, value="Audacious").click()
        with open(player_b_json_path, "r", encoding="utf-8") as f:
            player_b_metadata = read_player_metadata(json.load(f))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, player_b_metadata[0]["xesam:title"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, player_b_metadata[0]["xesam:album"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "0:00")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "-15:00")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, ', '.join(player_b_metadata[0]["xesam:artist"].unpack()))))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))

        # Switch to Multiplexer
        # A Paused, B Paused -> A (first added)
        self.driver.find_element(by=AppiumBy.NAME, value="Choose player automatically").click()
        wait.until(
            EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))

        # A Paused, B Playing -> B
        # Doc: https://lazka.github.io/pgi-docs/Gio-2.0/classes/DBusConnection.html
        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
        session_bus.call(f"org.mpris.MediaPlayer2.appiumtest.instance{str(self.player_b.pid)}", Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Play",
                         None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, player_b_metadata[0]["xesam:title"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Pause")))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))

        # Pause B -> Still B
        session_bus.call(f"org.mpris.MediaPlayer2.appiumtest.instance{str(self.player_b.pid)}", Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Pause",
                         None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, player_b_metadata[0]["xesam:title"].get_string())))

        # A Playing, B Paused -> A
        session_bus.call(self.mpris_interface.APP_INTERFACE, Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Play", None, None,
                         Gio.DBusSendMessageFlags.NONE, 1000)
        wait.until(
            EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Pause")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))

        # A Playing, B Playing -> Still A
        session_bus.call(f"org.mpris.MediaPlayer2.appiumtest.instance{str(self.player_b.pid)}", Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Play",
                         None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        sleep(1)
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())  # Title

        # A Paused, B Playing -> B
        session_bus.call(self.mpris_interface.APP_INTERFACE, Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Pause", None, None,
                         Gio.DBusSendMessageFlags.NONE, 1000)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, player_b_metadata[0]["xesam:title"].get_string())))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))

        # Close B -> A
        self.player_b.terminate()
        self.player_b = None
        wait.until(
            EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))
        self.assertFalse(player_selector.is_displayed())  # Tabbar is hidden again

    def _cleanup_multiplexer(self) -> None:
        if self.player_b:
            self.player_b.kill()
            self.player_b = None

    def test_filter_plasma_browser_integration(self) -> None:
        """
        When Plasma Browser Integration is installed, the widget should only show the player from p-b-i, and hide the player from the browser.
        """
        self.addCleanup(self._cleanup_filter_plasma_browser_integration)

        player_browser_json_path: str = path.join(getcwd(), "resources/player_browser.json")
        with open(player_browser_json_path, "r", encoding="utf-8") as f:
            browser_json_data = json.load(f)
        self.player_browser = subprocess.Popen(("python3", path.join(getcwd(), "utils/mediaplayer.py"), player_browser_json_path))
        wait: WebDriverWait = WebDriverWait(self.driver, 3)
        wait.until(EC.visibility_of_element_located((AppiumBy.ACCESSIBILITY_ID, "playerSelector")))
        browser_tab: WebElement = wait.until(EC.presence_of_element_located((AppiumBy.NAME, browser_json_data["base_properties"]["Identity"])))
        browser_tab.click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, browser_json_data["metadata"][0]["xesam:title"])))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, browser_json_data["metadata"][0]["xesam:album"])))
        self.assertFalse(self.driver.find_element(by=AppiumBy.NAME, value="Next Track").is_enabled())

        with open(path.join(getcwd(), "resources/player_plasma_browser_integration.json"), "r", encoding="utf-8") as f:
            pbi_json_data = json.load(f)
        pbi_json_data["metadata"][0]["kde:pid"] = self.player_browser.pid  # Simulate Plasma Browser Integration
        with NamedTemporaryFile("w", encoding="utf-8", suffix=".json", delete=False) as temp_file:
            json.dump(pbi_json_data, temp_file)
            temp_file.flush()

            self.player_plasma_browser_integration = subprocess.Popen(("python3", path.join(getcwd(), "utils/mediaplayer.py"), temp_file.name))
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, pbi_json_data["base_properties"]["Identity"]))).click()
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, pbi_json_data["metadata"][0]["xesam:title"])))
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, pbi_json_data["metadata"][0]["xesam:album"])))
            wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
            self.assertFalse(browser_tab.is_displayed())

        self._cleanup_filter_plasma_browser_integration()

    def _cleanup_filter_plasma_browser_integration(self) -> None:
        """
        A cleanup function to be called after the test is completed
        """
        if self.player_browser:
            self.player_browser.terminate()
            self.player_browser = None
        if self.player_plasma_browser_integration:
            self.player_plasma_browser_integration.terminate()
            self.player_plasma_browser_integration = None


if __name__ == '__main__':
    assert len(sys.argv) >= 2, f"Missing CMAKE_CURRENT_BINARY_DIR argument {len(sys.argv)}"
    CMAKE_BINARY_DIR = sys.argv.pop()
    unittest.main()
