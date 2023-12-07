#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# pylint: disable=too-many-arguments

import json
import os
import pathlib
import subprocess
import sys
import sysconfig
import unittest
from os import getcwd, path
from tempfile import NamedTemporaryFile
from time import sleep
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gio, GLib
from selenium.webdriver.remote.webelement import WebElement
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait
from utils.mediaplayer import (InvalidMpris2, Mpris2, read_base_properties, read_player_metadata, read_player_properties)

if "KDECI_BUILD" not in os.environ:
    CMAKE_INSTALL_PREFIX: Final = os.environ.get("CMAKE_INSTALL_PREFIX", os.path.join(pathlib.Path.home(), "kde", "usr"))
    SITE_PACKAGES_DIR: Final = os.path.join(CMAKE_INSTALL_PREFIX, sysconfig.get_path("platlib")[len(sys.prefix + os.sep):])
    for subdir in os.listdir(SITE_PACKAGES_DIR):
        sys.path.append(os.path.join(SITE_PACKAGES_DIR, subdir))

# This also initializes GLib.MainLoop as QGuiApplication creates an internal one
# An instance can only have one GLib.MainLoop, so no need to create another one manually
import inputsynth as IS

WIDGET_ID: Final = "org.kde.plasma.mediacontroller"


class MediaControllerTests(unittest.TestCase):
    """
    Tests for the media controller widget
    """

    driver: webdriver.Remote
    mpris_interface: Mpris2 | None
    player_b: subprocess.Popen | None = None
    player_browser: subprocess.Popen | None = None
    player_plasma_browser_integration: subprocess.Popen | None = None

    @classmethod
    def setUpClass(cls) -> None:
        """
        Opens the widget and initialize the webdriver
        """
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("environ", {
            "QT_FATAL_WARNINGS": "1",
            "QT_LOGGING_RULES": "qt.accessibility.atspi.warning=false;kf.plasma.core.warning=false;kf.windowsystem.warning=false;kf.kirigami.platform.warning=false",
        })
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def setUp(self) -> None:
        # Can't init the module in setUpClass because the nested wayland session may not be ready yet
        IS.init_module()

        json_path: str = path.join(getcwd(), "resources/player_a.json")
        with open(json_path, "r", encoding="utf-8") as f:
            json_dict: dict[str, list | dict] = json.load(f)
        metadata: list[dict[str, GLib.Variant]] = read_player_metadata(json_dict)
        base_properties: dict[str, GLib.Variant] = read_base_properties(json_dict)
        current_index: int = 1
        player_properties: dict[str, GLib.Variant] = read_player_properties(json_dict, metadata[current_index])

        self.mpris_interface = Mpris2(metadata, base_properties, player_properties, current_index)
        assert self.mpris_interface.registered_event.wait(10)

    def tearDown(self) -> None:
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_{WIDGET_ID}_#{self.id()}.png")
        if self.mpris_interface is not None:
            self.mpris_interface.quit()
            self.mpris_interface = None
        WebDriverWait(self.driver, 5, 0.2).until(EC.presence_of_element_located((AppiumBy.NAME, "No media playing")))

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.quit()

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
        wait: WebDriverWait = WebDriverWait(self.driver, 5)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))  # Title
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:album"].get_string())))  # Album
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "0:00")))  # Current position
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "-5:00")))  # Remaining time
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, ', '.join(self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:artist"].unpack()))))  # Artists

        # Now click the play button
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

        IS.init_task_manager()
        self.assertGreater(IS.get_task_count(), 0)

        IS.maximize_window(0)
        sleep(1)  # Window animation
        wait.until(lambda _: IS.is_window_maximized(0))

        # Get the geometry of the widget window
        rect: tuple[int] = IS.get_window_rect(0)  # x,y,width,height
        self.assertGreater(rect[2], 1, f"Invalid width {rect[2]}")
        self.assertGreater(rect[3], 1, f"Invalid height {rect[3]}")

        # Get the geometry of the virtual screen
        screen_rect: tuple[int] = IS.get_screen_rect(0)  # x,y,width,height
        self.assertEqual(screen_rect[0], 0)
        self.assertEqual(screen_rect[1], 0)
        self.assertGreater(screen_rect[2], 1, f"Invalid screen width {rect[2]}")
        self.assertGreater(screen_rect[3], 1, f"Invalid screen height {rect[3]}")

        # Get the center point
        center_pos_x: int = screen_rect[0] + int(screen_rect[2] / 2)
        center_pos_y: int = screen_rect[1] + int(screen_rect[3] / 2)

        # Touch the window
        IS.touch_down(center_pos_x, center_pos_y)
        IS.touch_up()

        # Swipe right -> Position++
        IS.touch_down(center_pos_x, center_pos_y)
        [IS.touch_move(center_pos_x + distance, center_pos_y) for distance in range(1, center_pos_x)]
        IS.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Position"].get_int64() > 0)

        # Swipe left -> Position--
        old_position: int = self.mpris_interface.player_properties["Position"].get_int64()
        IS.touch_down(center_pos_x, center_pos_y)
        [IS.touch_move(center_pos_x - distance, center_pos_y) for distance in range(1, center_pos_x)]
        IS.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Position"].get_int64() < old_position)
        # Swipe down: Volume--
        IS.touch_down(center_pos_x, center_pos_y)
        [IS.touch_move(center_pos_x, center_pos_y + distance) for distance in range(1, center_pos_y)]
        IS.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Volume"].get_double() < 1.0)

        # Swipe up: Volume++
        old_volume: float = self.mpris_interface.player_properties["Volume"].get_double()
        IS.touch_down(center_pos_x, center_pos_y)
        [IS.touch_move(center_pos_x, center_pos_y - distance) for distance in range(1, center_pos_y)]
        IS.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Volume"].get_double() > old_volume)

        # Swipe down and then swipe right, only volume should change
        old_volume = self.mpris_interface.player_properties["Volume"].get_double()
        old_position = self.mpris_interface.player_properties["Position"].get_int64()
        IS.touch_down(center_pos_x, center_pos_y)
        [IS.touch_move(center_pos_x, center_pos_y + distance) for distance in range(1, center_pos_y)]  # Swipe down
        sleep(0.5)  # Qt may ignore some touch events if there are too many, so explicitly wait a moment
        [IS.touch_move(center_pos_x + distance, center_pos_y + center_pos_y - 1) for distance in range(1, center_pos_x)]  # Swipe right
        IS.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Volume"].get_double() < old_volume)
        self.assertEqual(old_position, self.mpris_interface.player_properties["Position"].get_int64())

        # Swipe right and then swipe up, only position should change
        old_volume = self.mpris_interface.player_properties["Volume"].get_double()
        old_position = self.mpris_interface.player_properties["Position"].get_int64()
        IS.touch_down(center_pos_x, center_pos_y)
        [IS.touch_move(center_pos_x + distance, center_pos_y) for distance in range(1, center_pos_x)]  # Swipe right
        sleep(0.5)
        [IS.touch_move(center_pos_x + center_pos_x - 1, center_pos_y - distance) for distance in range(1, center_pos_y)]  # Swipe up
        IS.touch_up()
        wait.until(lambda _: self.mpris_interface.player_properties["Position"].get_int64() > old_position)
        self.assertAlmostEqual(old_volume, self.mpris_interface.player_properties["Volume"].get_double())

    def _cleanup_multiplexer(self) -> None:
        if self.player_b:
            self.player_b.kill()
            self.player_b = None

    def test_multiplexer(self) -> None:
        """
        The multiplexer should be hidden when there is only 1 player, and shows information from the active/playing player if there is one
        """
        self.addCleanup(self._cleanup_multiplexer)

        # Wait until the first player is ready
        wait: WebDriverWait = WebDriverWait(self.driver, 3)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))

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
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))

        # A Paused, B Playing -> B
        # Doc: https://lazka.github.io/pgi-docs/Gio-2.0/classes/DBusConnection.html
        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
        session_bus.call(f"org.mpris.MediaPlayer2.appiumtest.instance{str(self.player_b.pid)}", Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Play", None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, player_b_metadata[0]["xesam:title"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Pause")))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))

        # Pause B -> Still B
        session_bus.call(f"org.mpris.MediaPlayer2.appiumtest.instance{str(self.player_b.pid)}", Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Pause", None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, player_b_metadata[0]["xesam:title"].get_string())))

        # A Playing, B Paused -> A
        session_bus.call(self.mpris_interface.APP_INTERFACE, Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Play", None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Pause")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))

        # A Playing, B Playing -> Still A
        session_bus.call(f"org.mpris.MediaPlayer2.appiumtest.instance{str(self.player_b.pid)}", Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Play", None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        sleep(1)
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())  # Title

        # A Paused, B Playing -> B
        session_bus.call(self.mpris_interface.APP_INTERFACE, Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Pause", None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, player_b_metadata[0]["xesam:title"].get_string())))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until_not(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))

        # Close B -> A
        self.player_b.terminate()
        self.player_b = None
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
        wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Previous Track")))
        self.assertFalse(player_selector.is_displayed())  # Tabbar is hidden again

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

    def test_filter_plasma_browser_integration(self) -> None:
        """
        When Plasma Browser Integration is installed, the widget should only show the player from p-b-i, and hide the player from the browser.
        """
        self.addCleanup(self._cleanup_filter_plasma_browser_integration)

        # Make sure the active player is not the browser so the bug can be tested
        wait = WebDriverWait(self.driver, 3)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, self.mpris_interface.metadata[self.mpris_interface.current_index]["xesam:title"].get_string())))  # Title

        player_browser_json_path: str = path.join(getcwd(), "resources/player_browser.json")
        with open(player_browser_json_path, "r", encoding="utf-8") as f:
            browser_json_data = json.load(f)
        self.player_browser = subprocess.Popen(("python3", path.join(getcwd(), "utils/mediaplayer.py"), player_browser_json_path))
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
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Play")))
            wait.until(EC.element_to_be_clickable((AppiumBy.NAME, "Next Track")))
            self.assertFalse(browser_tab.is_displayed())

        # When a browser starts playing a video
        # 1. It registers the browser MPRIS instance with PlaybackStatus: Stopped/Paused, and Mpris2FilterProxyModel has it but the active player can be others
        # 2. p-b-i also registers its MPRIS instance, and Mpris2FilterProxyModel filters out the Chromium MPRIS instance, so the browser MPRIS instance in Mpris2FilterProxyModel becomes invalid
        # 3. PlaybackStatus changes to Playing, and the container of the browser MPRIS instance emits playbackStatusChanged() signal. However, the signal should be ignored by Multiplexer (disconnect in Multiplexer::onRowsAboutToBeRemoved)
        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
        session_bus.call(f"org.mpris.MediaPlayer2.appiumtest.instance{str(self.player_browser.pid)}", Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Play", None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        session_bus.call(f"org.mpris.MediaPlayer2.appiumtest.instance{str(self.player_plasma_browser_integration.pid)}", Mpris2.OBJECT_PATH, Mpris2.PLAYER_IFACE.get_string(), "Play", None, None, Gio.DBusSendMessageFlags.NONE, 1000)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Pause")))  # Confirm the backend does not crash

        self._cleanup_filter_plasma_browser_integration()

    def test_bug477144_invalid_player(self) -> None:
        """
        Do not crash when a player is invalid or its DBus interface returns any errors on initialization
        @see https://bugs.kde.org/show_bug.cgi?id=477144
        """
        if self.mpris_interface is not None:
            self.mpris_interface.quit()

        placeholder_element: WebElement = self.driver.find_element(AppiumBy.NAME, "No media playing")
        self.mpris_interface = InvalidMpris2()
        self.assertTrue(self.mpris_interface.registered_event.wait(10))
        self.assertTrue(placeholder_element.is_displayed())

    def test_bug477335_decode_xesam_url(self) -> None:
        """
        Make sure "xesam_url" is decoded before using it in other places like album name
        """
        if self.mpris_interface is not None:
            self.mpris_interface.quit()

        player_with_encoded_url_json_path: str = path.join(getcwd(), "resources/player_with_encoded_url.json")
        with subprocess.Popen(("python3", path.join(getcwd(), "utils/mediaplayer.py"), player_with_encoded_url_json_path)) as player_with_encoded_url:
            wait = WebDriverWait(self.driver, 3)
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Flash Funk")))
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "League of Legends")))  # Album name deducted from folder name
            # Overflow check, 2160000000 (microsecond) > INT_MAX (2147483647)
            wait.until(EC.presence_of_element_located((AppiumBy.NAME, "-36:00")))
            player_with_encoded_url.terminate()


if __name__ == '__main__':
    unittest.main()
