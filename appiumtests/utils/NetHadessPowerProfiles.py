#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# pylint: disable=too-many-arguments

# For FreeBSD CI which only has python 3.9
from __future__ import annotations

import logging
import os
import sys
import threading
from typing import Any, Final

current_folder: Final = os.path.dirname(os.path.abspath(__file__))
sys.path.append(current_folder)

from gi.repository import Gio, GLib


class NetHadessPowerProfiles:
    """
    D-Bus interface for net.hadess.PowerProfiles/org.freedesktop.UPower.PowerProfiles
    """

    BUS_NAME: Final = "net.hadess.PowerProfiles"
    OBJECT_PATH: Final = "/net/hadess/PowerProfiles"
    IFACE_NAME: Final = "net.hadess.PowerProfiles"

    __connection: Gio.DBusConnection

    def __init__(self) -> None:
        self.__ppd_reg_id: int = 0
        self.ppd_properties: dict[str, GLib.Variant] = {
            "ActiveProfile": GLib.Variant("s", "balanced"),
            "PerformanceInhibited": GLib.Variant("s", ""),  # deprecated
            "PerformanceDegraded": GLib.Variant("s", ""),
            "Profiles": GLib.Variant("aa{sv}", [
                {
                    "Profile": GLib.Variant("s", "power-saver"),
                    "PlatformDriver": GLib.Variant("s", "appiumtest"),
                    "Driver": GLib.Variant("s", "appiumtest"),
                },
                {
                    "Profile": GLib.Variant("s", "balanced"),
                    "PlatformDriver": GLib.Variant("s", "appiumtest"),
                    "Driver": GLib.Variant("s", "appiumtest"),
                },
                {
                    "Profile": GLib.Variant("s", "performance"),
                    "PlatformDriver": GLib.Variant("s", "appiumtest"),
                    "Driver": GLib.Variant("s", "appiumtest"),
                },
            ]),
            "Actions": GLib.Variant("as", ["trickle_charge", "amdgpu_panel_power"]),
            "ActiveProfileHolds": {},  # aa{sv}
            "Version": GLib.Variant("s", "0.21"),
        }
        self.__global_cookie: int = 0
        self.registered_event = threading.Event()
        self.active_profile_set_event = threading.Event()

        self.__owner_id: int = Gio.bus_own_name(Gio.BusType.SYSTEM, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.__owner_id > 0

    def quit(self) -> None:
        self.__connection.unregister_object(self.__ppd_reg_id)
        self.__ppd_reg_id = 0
        Gio.bus_unown_name(self.__owner_id)
        self.__connection.flush_sync(None)  # Otherwise flaky

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        """
        Interface is ready, now register objects.
        """
        self.__connection = connection

        with open(os.path.join(current_folder, os.pardir, os.pardir, "applets/batterymonitor/dbus/net.hadess.PowerProfiles.xml"), encoding="utf-8") as file_handler:
            introspection_xml: str = '\n'.join(file_handler.readlines())
            introspection_data = Gio.DBusNodeInfo.new_for_xml(introspection_xml)
            self.__ppd_reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.ppd_handle_method_call, self.ppd_handle_get_property, self.ppd_handle_set_property)
            assert self.__ppd_reg_id > 0

        self.registered_event.set()

    def set_profile(self, new_profile: str) -> None:
        self.ppd_properties["ActiveProfile"] = GLib.Variant("s", new_profile)
        changed_properties = {
            "ActiveProfile": self.ppd_properties["ActiveProfile"],
        }
        if len(self.ppd_properties["ActiveProfileHolds"]) > 0:
            released_cookies: list[int] = list(self.ppd_properties["ActiveProfileHolds"].keys())
            self.ppd_properties["ActiveProfileHolds"] = {}
            changed_properties["ActiveProfileHolds"] = self.active_profile_holds()
            for cookie in released_cookies:
                # This signal will be emitted if the profile is released because the
                # "ActiveProfile" was manually changed. The signal will only be emitted
                # to the process that originally called "HoldProfile".
                Gio.DBusConnection.emit_signal(self.__connection, None, self.OBJECT_PATH, self.IFACE_NAME, "ProfileReleased", GLib.Variant.new_tuple(GLib.Variant("u", cookie)))

        Gio.DBusConnection.emit_signal(self.__connection, None, self.OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.IFACE_NAME), GLib.Variant('a{sv}', changed_properties), GLib.Variant('as', [])))
        self.active_profile_set_event.set()

    def hold_profile(self, profile: str, reason: str, application_id: str) -> int:
        self.__global_cookie += 1
        self.ppd_properties["ActiveProfileHolds"][self.__global_cookie] = {
            "Profile": GLib.Variant("s", profile),
            "Reason": GLib.Variant("s", reason),
            "ApplicationId": GLib.Variant("s", application_id),
        }
        changed_properties = GLib.Variant('a{sv}', {
            "ActiveProfileHolds": self.active_profile_holds(),
        })
        Gio.DBusConnection.emit_signal(self.__connection, None, self.OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.IFACE_NAME), changed_properties, GLib.Variant('as', [])))
        return self.__global_cookie

    def release_profile(self, cookie: int) -> None:
        del self.ppd_properties["ActiveProfileHolds"][cookie]
        changed_properties = GLib.Variant('a{sv}', {
            "ActiveProfileHolds": self.active_profile_holds(),
        })
        Gio.DBusConnection.emit_signal(self.__connection, None, self.OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.IFACE_NAME), changed_properties, GLib.Variant('as', [])))

    def active_profile_holds(self) -> GLib.Variant:
        return GLib.Variant("aa{sv}", list(self.ppd_properties["ActiveProfileHolds"].values()))

    def set_performance_degraded_reason(self, reason: str) -> None:
        self.ppd_properties["PerformanceDegraded"] = GLib.Variant("s", reason)
        changed_properties = GLib.Variant('a{sv}', {
            "PerformanceDegraded": self.ppd_properties["PerformanceDegraded"],
        })
        Gio.DBusConnection.emit_signal(self.__connection, None, self.OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.IFACE_NAME), changed_properties, GLib.Variant('as', [])))

    def ppd_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        assert interface_name == self.BUS_NAME, f"Unknown interface {interface_name}"
        logging.info("ppd calling %s", method_name)

        if method_name == "HoldProfile":
            invocation.return_value(GLib.Variant.new_tuple(GLib.Variant("u", self.hold_profile(parameters[0], parameters[1], parameters[2]))))

        elif method_name == "ReleaseProfile":
            self.release_profile(parameters[0])
            invocation.return_value(None)

        else:
            logging.error("Unhandled method: %s", method_name)
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.UNKNOWN_METHOD, f"Unknown method {method_name}")

    def ppd_handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: Any):
        if value not in self.ppd_properties:
            logging.error("%s does not exist", value)
            return None
        logging.info("ppd get_property %s", value)
        if value == "ActiveProfileHolds":
            return self.active_profile_holds()
        return self.ppd_properties[value]

    def ppd_handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        logging.info("ppd set_property %s %s", key, value)
        if key == "ActiveProfile":
            self.set_profile(value.get_string())
            return True
        return False
