#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2019 The GNOME Music developers
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# pylint: disable=too-many-arguments

# For FreeBSD CI which only has python 3.9
from __future__ import annotations

import logging
import json
import signal
import sys
import threading
from os import getcwd, getpid, path
from typing import Any, Final

sys.path.append(path.dirname(path.abspath(__file__)))

from gi.repository import Gio, GLib
from GLibMainLoopThread import GLibMainLoopThread


def read_player_metadata(json_dict: dict[str, Any]) -> list[dict[str, GLib.Variant]]:
    song_list: list[dict[str, int | str | list[str]]] = json_dict["metadata"]
    assert len(song_list) > 0
    _metadata: list[dict[str, GLib.Variant]] = []
    for song in song_list:
        song_dict: dict[str, GLib.Variant] = {
            "mpris:trackid": GLib.Variant('o', song["mpris:trackid"]),
            "xesam:url": GLib.Variant('s', "file://" + path.join(getcwd(), song["xesam:url"])),
            "mpris:length": GLib.Variant('x', int(song["mpris:length"])),  # ms
            "xesam:title": GLib.Variant('s', song["xesam:title"]),
            "xesam:artist": GLib.Variant('as', song["xesam:artist"]),
            "mpris:artUrl": GLib.Variant('s', "")
        }
        if "xesam:album" in song.keys():
            song_dict["xesam:album"] = GLib.Variant('s', song["xesam:album"])
        if "mpris:artUrl" in song.keys():
            song_dict["mpris:artUrl"] = GLib.Variant('s', "file://" + path.join(getcwd(), song["mpris:artUrl"]))
        if "kde:pid" in song.keys():
            song_dict["kde:pid"] = GLib.Variant('u', song["kde:pid"])
        _metadata.append(song_dict)

    return _metadata


def read_base_properties(json_dict: dict[str, Any]) -> dict[str, GLib.Variant]:
    prop_dict: dict[str, bool | str | list[str]] = json_dict["base_properties"]
    assert len(prop_dict) > 0
    _base_properties: dict[str, GLib.Variant] = {
        "CanQuit": GLib.Variant('b', prop_dict["CanQuit"]),
        "Fullscreen": GLib.Variant('b', prop_dict["Fullscreen"]),
        "CanSetFullscreen": GLib.Variant('b', prop_dict["CanSetFullscreen"]),
        "CanRaise": GLib.Variant('b', prop_dict["CanRaise"]),
        "HasTrackList": GLib.Variant('b', prop_dict["HasTrackList"]),
        "Identity": GLib.Variant('s', prop_dict["Identity"]),
        "DesktopEntry": GLib.Variant('s', prop_dict["DesktopEntry"]),
        "SupportedUriSchemes": GLib.Variant('as', prop_dict["SupportedUriSchemes"]),
        "SupportedMimeTypes": GLib.Variant('as', prop_dict["SupportedMimeTypes"]),
    }
    return _base_properties


def read_player_properties(json_dict: dict[str, Any], _metadata: dict[str, GLib.Variant]) -> dict[str, GLib.Variant]:
    prop_dict: dict[str, int | float | bool | str | dict] = json_dict["player_properties"]
    assert len(prop_dict) > 0
    _player_properties: dict[str, GLib.Variant] = {
        "PlaybackStatus": GLib.Variant("s", prop_dict["PlaybackStatus"]),
        "LoopStatus": GLib.Variant("s", prop_dict["LoopStatus"]),
        "Rate": GLib.Variant("d", prop_dict["Rate"]),
        "Shuffle": GLib.Variant("b", prop_dict["Shuffle"]),
        "Metadata": GLib.Variant("a{sv}", _metadata),
        "Position": GLib.Variant("x", prop_dict["Position"]),
        "MinimumRate": GLib.Variant("d", prop_dict["MinimumRate"]),
        "MaximumRate": GLib.Variant("d", prop_dict["MaximumRate"]),
        "Volume": GLib.Variant("d", prop_dict["Volume"]),
        "CanGoNext": GLib.Variant("b", prop_dict["CanGoNext"]),
        "CanGoPrevious": GLib.Variant("b", prop_dict["CanGoPrevious"]),
        "CanPlay": GLib.Variant("b", prop_dict["CanPlay"]),
        "CanPause": GLib.Variant("b", prop_dict["CanPause"]),
        "CanSeek": GLib.Variant("b", prop_dict["CanSeek"]),
        "CanControl": GLib.Variant("b", prop_dict["CanControl"]),
    }
    return _player_properties


class Mpris2:
    """
    MPRIS2 interface implemented in GDBus, since dbus-python does not support property
    """

    BASE_IFACE: Final = "org.mpris.MediaPlayer2"
    OBJECT_PATH: Final = "/org/mpris/MediaPlayer2"
    PLAYER_IFACE: Final = GLib.Variant('s', "org.mpris.MediaPlayer2.Player")
    APP_INTERFACE: Final = f"org.mpris.MediaPlayer2.appiumtest.instance{str(getpid())}"

    connection: Gio.DBusConnection | None = None

    def __init__(self, metadata: list[dict[str, GLib.Variant]], base_properties: dict[str, GLib.Variant], player_properties: dict[str, GLib.Variant], current_index: int) -> None:
        self.metadata: list[dict[str, GLib.Variant]] = metadata
        self.base_properties: dict[str, GLib.Variant] = base_properties
        self.player_properties: dict[str, GLib.Variant] = player_properties
        self.current_index: int = current_index

        self.registered_event = threading.Event()
        self.playback_status_set_event = threading.Event()
        self.metadata_updated_event = threading.Event()

        self.__owner_id: int = Gio.bus_own_name(Gio.BusType.SESSION, self.APP_INTERFACE, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.__owner_id > 0

        self.__prop_reg_id: int = 0
        self.__player_reg_id: int = 0
        self.__base_reg_id: int = 0

    def quit(self) -> None:
        if self.connection is None:
            return
        self.connection.unregister_object(self.__prop_reg_id)
        self.__prop_reg_id = 0
        self.connection.unregister_object(self.__player_reg_id)
        self.__player_reg_id = 0
        self.connection.unregister_object(self.__base_reg_id)
        self.__base_reg_id = 0
        Gio.bus_unown_name(self.__owner_id)
        self.connection.flush_sync(None)  # Otherwise flaky
        logging.info("Player exit")

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        """
        Interface is ready, now register objects.
        """
        self.connection = connection

        with open("../libkmpris/dbus/org.freedesktop.DBus.Properties.xml", encoding="utf-8") as handler:
            properties_introspection_xml: str = '\n'.join(handler.readlines())
            introspection_data = Gio.DBusNodeInfo.new_for_xml(properties_introspection_xml)
            self.__prop_reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.properties_handle_method_call, None, None)
        assert self.__prop_reg_id > 0

        with open("../libkmpris/dbus/org.mpris.MediaPlayer2.Player.xml", encoding="utf-8") as handler:
            player_introspection_xml: str = '\n'.join(handler.readlines())
            introspection_data = Gio.DBusNodeInfo.new_for_xml(player_introspection_xml)
            self.__player_reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.player_handle_method_call, self.player_handle_get_property, self.player_handle_set_property)
        assert self.__player_reg_id != 0

        with open("../libkmpris/dbus/org.mpris.MediaPlayer2.xml", encoding="utf-8") as handler:
            interface_introspection_xml: str = '\n'.join(handler.readlines())
            introspection_data = Gio.DBusNodeInfo.new_for_xml(interface_introspection_xml)
            self.__base_reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.interface_handle_method_call, self.interface_handle_get_property, self.interface_handle_set_property)
        assert self.__base_reg_id != 0

        print("MPRIS registered", file=sys.stderr, flush=True)
        self.registered_event.set()

    def properties_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
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

            print(f"Get: {interface} {property_name} {ret}", file=sys.stderr, flush=True)
            # https://bugzilla.gnome.org/show_bug.cgi?id=765603
            invocation.return_value(GLib.Variant.new_tuple(ret))

        elif method_name == "GetAll":
            interface = parameters[0]
            if interface == self.BASE_IFACE:
                ret = GLib.Variant('a{sv}', self.base_properties)
            elif interface == self.PLAYER_IFACE.get_string():
                ret = GLib.Variant('a{sv}', self.player_properties)
            else:
                assert False, f"Unknown interface {interface}"

            print(f"GetAll: {interface} {ret}", file=sys.stderr, flush=True)
            invocation.return_value(GLib.Variant.new_tuple(ret))

        elif method_name == "Set":
            interface = parameters[0]
            property_name = parameters[1]
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

            print(f"Set: {interface} {property_name} {value}", file=sys.stderr, flush=True)

        else:
            logging.error("Unhandled method: %s", method_name)
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.UNKNOWN_METHOD, f"Unknown method {method_name}")

    def set_playing(self, playing: bool, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Changes the playing state
        """
        self.player_properties["PlaybackStatus"] = GLib.Variant('s', "Playing" if playing else "Paused")
        changed_properties = GLib.Variant('a{sv}', {
            "PlaybackStatus": self.player_properties["PlaybackStatus"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))
        self.playback_status_set_event.set()

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
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

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
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def set_shuffle(self, shuffle: bool, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Changes the shuffle state
        """
        assert isinstance(shuffle, bool), f"argument is not a boolean but {type(shuffle)}"
        self.player_properties["Shuffle"] = GLib.Variant('b', shuffle)
        changed_properties = GLib.Variant('a{sv}', {
            "Shuffle": self.player_properties["Shuffle"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def set_repeat(self, repeat: str, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Changes the loop state
        """
        assert isinstance(repeat, str), f"argument is not a string but {type(repeat)}"
        self.player_properties["LoopStatus"] = GLib.Variant('s', repeat)
        changed_properties = GLib.Variant('a{sv}', {
            "LoopStatus": self.player_properties["LoopStatus"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def set_volume(self, volume: float, connection: Gio.DBusConnection, object_path: str) -> None:
        """
        Adjusts the volume
        """
        assert isinstance(volume, float) and 0 <= volume <= 1, f"Invalid volume {volume} of type {type(volume)}"
        self.player_properties["Volume"] = GLib.Variant('d', volume)
        changed_properties = GLib.Variant('a{sv}', {
            "Volume": self.player_properties["Volume"],
        })
        Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

    def player_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        Handles method calls for org.mpris.MediaPlayer2.Player
        """
        assert interface_name == "org.mpris.MediaPlayer2.Player", f"Wrong interface name {interface_name} from {sender}"

        print(f"player_handle_method_call method_name: {method_name}", file=sys.stderr, flush=True)

        if method_name == "Next" or method_name == "Previous":
            self.current_index += 1 if method_name == "Next" else -1
            assert 0 <= self.current_index < len(self.metadata)
            self.player_properties["Metadata"] = GLib.Variant('a{sv}', self.metadata[self.current_index])
            self.player_properties["CanGoPrevious"] = GLib.Variant('b', self.current_index > 0)
            self.player_properties["CanGoNext"] = GLib.Variant('b', self.current_index < len(self.metadata) - 1)
            changed_properties = GLib.Variant('a{sv}', {
                "Metadata": self.player_properties["Metadata"],
                "CanGoPrevious": self.player_properties["CanGoPrevious"],
                "CanGoNext": self.player_properties["CanGoNext"],
            })
            Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))
            self.metadata_updated_event.set()

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
            length: int = int(self.metadata[self.current_index]["mpris:length"])
            position: int = self.player_properties["Position"].get_int64()
            assert 0 <= position + offset <= length
            self.player_properties["Position"] = GLib.Variant('x', position + offset)
            changed_properties = GLib.Variant('a{sv}', {
                'Position': self.player_properties["Position"],
            })
            Gio.DBusConnection.emit_signal(connection, None, object_path, self.PLAYER_IFACE.get_string(), "Seeked", GLib.Variant.new_tuple(self.player_properties["Position"]))
            Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

        elif method_name == "SetPosition":
            assert parameters[0] == self.metadata[self.current_index]["mpris:trackid"].get_string(), f"expected trackid: {parameters[0]}, actual trackid: {self.metadata[self.current_index]['mpris:trackid'].get_string()}"
            self.player_properties["Position"] = GLib.Variant('x', parameters[1])
            changed_properties = GLib.Variant('a{sv}', {
                'Position': self.player_properties["Position"],
            })
            Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(self.PLAYER_IFACE, changed_properties, GLib.Variant('as', ())))

        elif method_name == "OpenUri":
            print("OpenUri", file=sys.stderr, flush=True)

        else:
            # In case the interface adds new methods, fail here for easier discovery
            logging.error("%s does not exist", method_name)
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.NOT_SUPPORTED, f"Invalid method {method_name}")

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

        print(f"player_handle_set_property key: {key}, value: {value}", file=sys.stderr, flush=True)

        if key == "Rate":
            self.set_rate(value, connection, object_path)
        elif key == "LoopStatus":
            self.set_repeat(value, connection, object_path)
        elif key == "Shuffle":
            self.set_shuffle(value, connection, object_path)
        elif key == "Volume":
            self.set_volume(value, connection, object_path)
        else:
            return False

        # What is the correct thing to return here on success?  It appears that
        # we need to return something other than None or what would be evaluated
        # to False for this call back to be successful.
        return True

    def interface_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        Handles method calls for org.mpris.MediaPlayer2
        """
        assert interface_name == "org.mpris.MediaPlayer2", f"Wrong interface name {interface_name} from {sender}"

        if method_name == "Raise":
            print("Raise", file=sys.stderr, flush=True)
        elif method_name == "Quit":
            print("Quit", file=sys.stderr, flush=True)
        else:
            logging.error("%s does not exist", method_name)
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.NOT_SUPPORTED, f"Invalid method {method_name}")

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


class InvalidMpris2(Mpris2):
    """
    MPRIS2 interfaces with invalid responses
    """

    def __init__(self) -> None:
        super().__init__([], {}, {}, 0)

    def properties_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        @override
        """
        invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.NOT_SUPPORTED, "")

    def player_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        @override
        """
        invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.NOT_SUPPORTED, "")

    def player_handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: Any):
        """
        @override
        """
        return None

    def player_handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        """
        @override
        """
        return False

    def interface_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        @override
        """
        invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.NOT_SUPPORTED, "")

    def interface_handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: Any):
        """
        @override
        """
        return None

    def interface_handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        """
        @override
        """
        return False


player: Mpris2
loopThread: GLibMainLoopThread


def __on_terminate(signal, frame) -> None:
    player.quit()
    loopThread.quit()
    if loopThread.is_alive():
        loopThread.join(timeout=10)


if __name__ == '__main__':
    assert len(sys.argv) >= 2, "Insufficient arguments"
    signal.signal(signal.SIGTERM, __on_terminate)

    json_path: str = sys.argv.pop()
    with open(json_path, "r", encoding="utf-8") as f:
        json_dict: dict[str, list | dict] = json.load(f)
    metadata: list[dict[str, GLib.Variant]] = read_player_metadata(json_dict)
    base_properties: dict[str, GLib.Variant] = read_base_properties(json_dict)
    current_index: int = 0
    player_properties: dict[str, GLib.Variant] = read_player_properties(json_dict, metadata[current_index])

    loopThread = GLibMainLoopThread()
    loopThread.start()
    player = Mpris2(metadata, base_properties, player_properties, current_index)
    logging.info("Player %s started", base_properties['Identity'].get_string())
