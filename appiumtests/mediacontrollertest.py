#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2019 The GNOME Music developers
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# pylint: disable=too-many-arguments

import ctypes
import sys
from time import sleep
import unittest
from os import getcwd, getpid, path
from typing import Any, Final

from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gio, GLib
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

CMAKE_BINARY_DIR: str = ""


class Mpris2:
    """
    MPRIS2 interface implemented in GDBus, since dbus-python does not support property
    """

    BASE_IFACE: Final = "org.mpris.MediaPlayer2"
    OBJECT_PATH: Final = "/org/mpris/MediaPlayer2"
    PLAYER_IFACE: Final = GLib.Variant('s', "org.mpris.MediaPlayer2.Player")
    APP_INTERFACE: Final = f"org.mpris.MediaPlayer2.appiumtest.instance{str(getpid())}"

    __connection: Gio.DBusConnection

    def __init__(self) -> None:
        self.metadata: dict[str, GLib.Variant] = {
            'mpris:trackid': GLib.Variant('o', "/appiumtest/1"),
            'xesam:url': GLib.Variant('s', "file://" + path.join(getcwd(), "resources/media1.mp3")),
            'mpris:length': GLib.Variant('x', 30 * 10e6),  # ms
            'xesam:title': GLib.Variant('s', "Konqi ❤️️ Katie"),
            'xesam:album': GLib.Variant('s', "The Best of Konqi"),
            'xesam:artist': GLib.Variant('as', ["KDE Community", "La communauté KDE"]),
            "mpris:artUrl": GLib.Variant('s', "file://" + path.join(getcwd(), "resources/mediacontrollertests_art1.png"))
        }
        self.base_properties: dict[str, GLib.Variant] = {
            "CanQuit": GLib.Variant('b', True),
            "Fullscreen": GLib.Variant('b', False),
            "CanSetFullscreen": GLib.Variant('b', False),
            "CanRaise": GLib.Variant('b', True),
            "HasTrackList": GLib.Variant('b', True),
            "Identity": GLib.Variant('s', 'AppiumTest'),
            "DesktopEntry": GLib.Variant('s', 'appiumtest'),
            "SupportedUriSchemes": GLib.Variant('as', [
                'file',
            ]),
            "SupportedMimeTypes": GLib.Variant('as', [
                'application/ogg',
            ]),
        }
        self.player_properties: dict[str, GLib.Variant] = {
            'PlaybackStatus': GLib.Variant('s', "Stopped"),
            'LoopStatus': GLib.Variant('s', "None"),
            'Rate': GLib.Variant('d', 1.0),
            'Shuffle': GLib.Variant('b', False),
            'Metadata': GLib.Variant('a{sv}', self.metadata),
            "Position": GLib.Variant('x', 0),
            'MinimumRate': GLib.Variant('d', 1.0),
            'MaximumRate': GLib.Variant('d', 1.0),
            "Volume": GLib.Variant('d', 1.0),
            'CanGoNext': GLib.Variant('b', True),
            'CanGoPrevious': GLib.Variant('b', True),
            'CanPlay': GLib.Variant('b', True),
            'CanPause': GLib.Variant('b', True),
            'CanSeek': GLib.Variant('b', True),
            'CanControl': GLib.Variant('b', True),
        }

        self.__owner_id: int = Gio.bus_own_name(Gio.BusType.SESSION, self.APP_INTERFACE, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.__owner_id > 0

        self.__prop_reg_id: int = 0
        self.__player_reg_id: int = 0
        self.__base_reg_id: int = 0

    def quit(self) -> None:
        Gio.bus_unown_name(self.__owner_id)
        self.__connection.unregister_object(self.__prop_reg_id)
        self.__prop_reg_id = 0
        self.__connection.unregister_object(self.__player_reg_id)
        self.__player_reg_id = 0
        self.__connection.unregister_object(self.__base_reg_id)
        self.__base_reg_id = 0
        print("Player exit")

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        """
        Interface is ready, now register objects.
        """
        self.__connection = connection

        properties_introspection_xml: str = '\n'.join(open("../dataengines/mpris2/org.freedesktop.DBus.Properties.xml", encoding="utf-8").readlines())
        introspection_data = Gio.DBusNodeInfo.new_for_xml(properties_introspection_xml)
        self.__prop_reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.properties_handle_method_call, None, None)
        assert self.__prop_reg_id > 0

        player_introspection_xml: str = '\n'.join(open("../dataengines/mpris2/org.mpris.MediaPlayer2.Player.xml", encoding="utf-8").readlines())

        introspection_data = Gio.DBusNodeInfo.new_for_xml(player_introspection_xml)
        self.__player_reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.player_handle_method_call,
                                                          self.player_handle_get_property, self.player_handle_set_property)
        assert self.__player_reg_id != 0

        interface_introspection_xml: str = '\n'.join(open("../dataengines/mpris2/org.mpris.MediaPlayer2.xml", encoding="utf-8").readlines())
        introspection_data = Gio.DBusNodeInfo.new_for_xml(interface_introspection_xml)
        self.__base_reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.interface_handle_method_call,
                                                        self.interface_handle_get_property, self.interface_handle_set_property)
        assert self.__base_reg_id != 0

    def properties_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str,
                                      parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        Handles method calls for org.freedesktop.DBus.Properties
        """
        assert interface_name == "org.freedesktop.DBus.Properties", f"Wrong interface name {interface_name} from {sender}"

        if method_name == "Get":
            ret: Any = None
            interface: str = parameters[0]
            property_name: str = parameters[1]
            if interface == self.BASE_IFACE:
                ret = self.base_properties[property_name]
            elif interface == self.PLAYER_IFACE.get_string():
                ret = self.player_properties[property_name]
            else:
                assert False, f"Unknown interface {interface}"

            print(f"Get: {interface} {property_name} {ret}")
            # https://bugzilla.gnome.org/show_bug.cgi?id=765603
            invocation.return_value(GLib.Variant('(v)', [ret]))

        elif method_name == "GetAll":
            interface: str = parameters[0]
            if interface == self.BASE_IFACE:
                ret = GLib.Variant('a{sv}', self.base_properties)
            elif interface == self.PLAYER_IFACE.get_string():
                ret = GLib.Variant('a{sv}', self.player_properties)
            else:
                assert False, f"Unknown interface {interface}"

            print(f"GetAll: {interface} {ret}")
            invocation.return_value(GLib.Variant.new_tuple(ret))

        elif method_name == "Set":
            interface: str = parameters[0]
            property_name: str = parameters[1]
            value: Any = parameters[2]

            if interface != self.PLAYER_IFACE.get_string():
                assert False, f"Wrong interface {interface}"

            if property_name == "Rate":
                self.set_rate(value, connection, object_path)
            elif property_name == "LoopStatus":
                self.set_repeat(value, connection, object_path)
            elif property_name == "Shuffle":
                self.set_shuffle(value, connection, object_path)
            elif property_name == "Volume":
                self.set_volume(value, connection, object_path)
            else:
                assert False, f"Unknown property {property_name}"

            print(f"Set: {interface} {property_name} {value}")

        else:
            invocation.return_value(None)

    def set_playing(self, playing: bool, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Changes the playing state
        """
        self.player_properties["PlaybackStatus"] = GLib.Variant('s', "Playing" if playing else "Paused")
        changed_properties = GLib.Variant('a{sv}', {
            "PlaybackStatus": self.player_properties["PlaybackStatus"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                       GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def stop(self, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Changes the playing state
        """
        self.player_properties["PlaybackStatus"] = GLib.Variant('s', "Stopped")
        self.player_properties["Metadata"] = GLib.Variant('a{sv}', {})
        changed_properties = GLib.Variant('a{sv}', {
            "PlaybackStatus": self.player_properties["PlaybackStatus"],
            "Metadata": self.player_properties["Metadata"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                       GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def set_rate(self, rate: float, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Changes the shuffle state
        """
        assert isinstance(rate, float), f"argument is not a float but {type(rate)}"
        assert self.player_properties["MinimumRate"].get_double() <= rate <= self.player_properties["MaximumRate"].get_double(), f"Rate {rate} is out of bounds"
        self.player_properties["Rate"] = GLib.Variant('d', rate)
        changed_properties = GLib.Variant('a{sv}', {
            "Rate": self.player_properties["Rate"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                       GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def set_shuffle(self, shuffle: bool, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Changes the shuffle state
        """
        assert isinstance(shuffle, bool), f"argument is not a boolean but {type(shuffle)}"
        self.player_properties["Shuffle"] = GLib.Variant('b', shuffle)
        changed_properties = GLib.Variant('a{sv}', {
            "Shuffle": self.player_properties["Shuffle"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                       GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def set_repeat(self, repeat: str, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Changes the loop state
        """
        assert isinstance(repeat, str), f"argument is not a string but {type(repeat)}"
        self.player_properties["LoopStatus"] = GLib.Variant('s', repeat)
        changed_properties = GLib.Variant('a{sv}', {
            "LoopStatus": self.player_properties["LoopStatus"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                       GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def set_volume(self, volume: float, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Adjusts the volume
        """
        assert isinstance(volume, float) and 0 <= volume <= 1, f"Invalid volume {volume} of type {type(volume)}"
        self.player_properties["Volume"] = GLib.Variant('d', volume)
        changed_properties = GLib.Variant('a{sv}', {
            "Volume": self.player_properties["Volume"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                       GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def player_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str,
                                  parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        Handles method calls for org.mpris.MediaPlayer2.Player
        """
        assert interface_name == "org.mpris.MediaPlayer2.Player", f"Wrong interface name {interface_name} from {sender}"

        print(f"player_handle_method_call method_name: {method_name}")

        if method_name == "Next":
            self.metadata = {
                'mpris:trackid': GLib.Variant('o', "/appiumtest/2"),
                'xesam:url': GLib.Variant('s', "file://" + path.join(getcwd(), "resources/media2.mp3")),
                'mpris:length': GLib.Variant('x', 90 * 10e6),  # ms
                'xesam:title': GLib.Variant('s', "Konqi's Favorite"),
                'xesam:album': GLib.Variant('s', "The Best of Konqi"),
                'xesam:artist': GLib.Variant('as', ["KDE Community", "KDE 社区"]),
                "mpris:artUrl": GLib.Variant('s', "file://" + path.join(getcwd(), "resources/mediacontrollertests_art2.png"))
            }
            self.player_properties["Metadata"] = GLib.Variant('a{sv}', self.metadata)
            self.player_properties["CanGoPrevious"] = GLib.Variant('b', True)
            self.player_properties["CanGoNext"] = GLib.Variant('b', False)
            changed_properties = GLib.Variant(
                'a{sv}', {
                    'Metadata': self.player_properties["Metadata"],
                    "CanGoPrevious": self.player_properties["CanGoPrevious"],
                    "CanGoNext": self.player_properties["CanGoNext"],
                })
            Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                           GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

        elif method_name == "Previous":
            self.metadata = {
                'mpris:trackid': GLib.Variant('o', "/appiumtest/2"),
                'xesam:url': GLib.Variant('s', "file://" + path.join(getcwd(), "resources/media0.mp3")),
                'mpris:length': GLib.Variant('x', 60 * 10e6),  # ms
                'xesam:title': GLib.Variant('s', "Katie's Favorite"),
                'xesam:album': GLib.Variant('s', "The Best of Konqi"),
                'xesam:artist': GLib.Variant('as', ["KDE Community"]),
                "mpris:artUrl": GLib.Variant('s', "file://" + path.join(getcwd(), "resources/mediacontrollertests_art0.png"))
            }
            self.player_properties["Metadata"] = GLib.Variant('a{sv}', self.metadata)
            self.player_properties["CanGoPrevious"] = GLib.Variant('b', False)
            self.player_properties["CanGoNext"] = GLib.Variant('b', True)
            changed_properties = GLib.Variant(
                'a{sv}', {
                    'Metadata': self.player_properties["Metadata"],
                    "CanGoPrevious": self.player_properties["CanGoPrevious"],
                    "CanGoNext": self.player_properties["CanGoNext"],
                })
            Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                           GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

        elif method_name == "Pause":
            self.set_playing(False, connection, object_path)

        elif method_name == "PlayPause":
            self.set_playing(self.player_properties["PlaybackStatus"].get_string() != "Playing", connection, object_path)

        elif method_name == "Stop":
            self.stop(connection, object_path)

        elif method_name == "Play":
            self.set_playing(True, connection, object_path)

        elif method_name == "Seek":
            offset: int = parameters[0]
            length: int = int(self.metadata["mpris:length"])
            position: int = self.player_properties["Position"].get_int64()
            assert 0 <= position + offset <= length
            self.player_properties["Position"] = GLib.Variant('x', position + offset)
            changed_properties = GLib.Variant('a{sv}', {
                'Position': self.player_properties["Position"],
            })
            Gio.DBusConnection.emit_signal(connection, None, object_path, self.PLAYER_IFACE.get_string(), "Seeked",
                                           GLib.Variant.new_tuple(self.player_properties["Position"]))
            Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                           GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

        elif method_name == "SetPosition":
            assert parameters[0] == self.metadata["mpris:trackid"].get_string(
            ), f"expected trackid: {parameters[0]}, actual trackid: {self.metadata['mpris:trackid'].get_string()}"
            self.player_properties["Position"] = GLib.Variant('x', parameters[1])
            changed_properties = GLib.Variant('a{sv}', {
                'Position': self.player_properties["Position"],
            })
            Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                           GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

        elif method_name == "OpenUri":
            print("OpenUri")

        else:
            # In case the interface adds new methods, fail here for easier discovery
            assert False, f"{method_name} does not exist"

    def player_handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: Any):
        """
        Handles properties for org.mpris.MediaPlayer2.Player
        """
        assert interface_name == "org.mpris.MediaPlayer2.Player", f"Wrong interface name {interface_name} from {sender}"
        assert value in self.player_properties.keys(), f"{value} does not exist"

        return self.player_properties[value]

    def player_handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        """
        Handles properties for org.mpris.MediaPlayer2.Player
        """
        assert interface_name == "org.mpris.MediaPlayer2.Player", f"Wrong interface name {interface_name} from {sender}"
        assert key in self.player_properties.keys(), f"{key} does not exist"

        print(f"player_handle_set_property key: {key}, value: {value}")

        if key == "Rate":
            self.set_rate(value, connection, object_path)
        elif key == "LoopStatus":
            self.set_repeat(value, connection, object_path)
        elif key == "Shuffle":
            self.set_shuffle(value, connection, object_path)
        elif key == "Volume":
            self.set_volume(value, connection, object_path)
        else:
            assert False

        # What is the correct thing to return here on success?  It appears that
        # we need to return something other than None or what would be evaluated
        # to False for this call back to be successful.
        return True

    def interface_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str,
                                     parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        Handles method calls for org.mpris.MediaPlayer2
        """
        assert interface_name == "org.mpris.MediaPlayer2", f"Wrong interface name {interface_name} from {sender}"

        if method_name == "Raise":
            print("Raise")
        elif method_name == "Quit":
            print("Quit")
        else:
            assert False, f"method f{method_name} does not exist"

    def interface_handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: Any):
        """
        Handles properties for org.mpris.MediaPlayer2
        """
        assert interface_name == "org.mpris.MediaPlayer2", f"Wrong interface name {interface_name} from {sender}"
        assert value in self.base_properties.keys(), f"{value} does not exist"

        return self.base_properties[value]

    def interface_handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        """
        Handles properties for org.mpris.MediaPlayer2
        """
        assert interface_name == "org.mpris.MediaPlayer2", f"Wrong interface name {interface_name} from {sender}"
        assert key in self.base_properties.keys(), f"{key} does not exist"

        return True


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
        self.mpris_interface = Mpris2()

    def tearDown(self) -> None:
        self.mpris_interface.quit()
        self.mpris_interface = None
        self.driver.find_element(by=AppiumBy.NAME, value="No media playing")

    def test_track(self) -> None:
        """
        Tests the widget can show track metadata
        """
        play_button = self.driver.find_element(by=AppiumBy.NAME, value="Play")
        previous_button = self.driver.find_element(by=AppiumBy.NAME, value="Previous Track")
        next_button = self.driver.find_element(by=AppiumBy.NAME, value="Next Track")
        shuffle_button = self.driver.find_element(by=AppiumBy.NAME, value="Shuffle")
        repeat_button = self.driver.find_element(by=AppiumBy.NAME, value="Repeat")

        # Match song title, artist and album
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata["xesam:title"].get_string())  # Title
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata["xesam:album"].get_string())  # Album
        self.driver.find_element(by=AppiumBy.NAME, value="0:00")  # Current position
        self.driver.find_element(by=AppiumBy.NAME, value="-5:00")  # Remaining time
        self.driver.find_element(by=AppiumBy.NAME, value=', '.join(self.mpris_interface.metadata["xesam:artist"].unpack()))  # Artists

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
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata["xesam:title"].get_string())
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata["xesam:album"].get_string())
        self.driver.find_element(by=AppiumBy.NAME, value="0:00")
        self.driver.find_element(by=AppiumBy.NAME, value="-10:00")
        self.driver.find_element(by=AppiumBy.NAME, value=', '.join(self.mpris_interface.metadata["xesam:artist"].unpack()))

        # Switch to the next song, and match again
        next_button.click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Konqi's Favorite")))
        self.assertTrue(previous_button.is_enabled())
        self.assertFalse(next_button.is_enabled())
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata["xesam:title"].get_string())
        self.driver.find_element(by=AppiumBy.NAME, value=self.mpris_interface.metadata["xesam:album"].get_string())
        self.driver.find_element(by=AppiumBy.NAME, value="0:00")
        self.driver.find_element(by=AppiumBy.NAME, value="-15:00")
        self.driver.find_element(by=AppiumBy.NAME, value=', '.join(self.mpris_interface.metadata["xesam:artist"].unpack()))

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
