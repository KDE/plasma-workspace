#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# pylint: disable=too-many-arguments

# For FreeBSD CI which only has python 3.9
from __future__ import annotations

import os
import sys
import threading
import time
from io import TextIOWrapper
from typing import Any, Final

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from gi.repository import Gio, GLib
from GLibMainLoopThread import GLibMainLoopThread


def skip_doc(file_handler: TextIOWrapper) -> str:
    """
    Gio.DBusNodeInfo.new_for_xml doesn't like doc
    """
    introspection_xml: str = ""
    skip_line: bool = False
    for l in file_handler.readlines():
        if not skip_line:
            if "<doc:doc>" in l:
                skip_line = "</doc:doc>" not in l  # In case they are in the same line
                continue
            else:
                introspection_xml += l + "\n"
        else:
            if "</doc:doc>" in l:
                skip_line = False
    return introspection_xml


class OrgFreedesktopUPower:
    """
    D-Bus interfaces for org.freedesktop.UPower and org.freedesktop.UPower.Device
    """

    BUS_NAME: Final = "org.freedesktop.UPower"
    OBJECT_PATH: Final = "/org/freedesktop/UPower"
    DEVICE_IFACE_NAME: Final = "org.freedesktop.UPower.Device"
    BATTERY0_OBJECT_PATH: Final = "/org/freedesktop/UPower/devices/battery_BAT0"
    BATTERY1_OBJECT_PATH: Final = "/org/freedesktop/UPower/devices/battery_BAT1"
    WIRELESS_MOUSE_OBJECT_PATH: Final = "/org/freedesktop/UPower/devices/mouse_hidpp_battery_0"
    WIRELESS_KEYBOARD_OBJECT_PATH: Final = "/org/freedesktop/UPower/devices/keyboard_hidpp_battery_1"
    AC_OBJECT_PATH: Final = "/org/freedesktop/UPower/devices/line_power_AC"
    DISPLAY_DEVICE_OBJECT_PATH: Final = "/org/freedesktop/UPower/devices/DisplayDevice"

    __connection: Gio.DBusConnection

    def __init__(self, device_properties: dict[str, dict[str, GLib.Variant]] | None = None, enable_display_device: bool = True) -> None:
        self.__upower_reg_id: int = 0
        self.__device_reg_id_map: dict[str, int] = {}  # object_path: reg_id
        self.__display_device_reg_id: int = 0

        self.upower_properties: dict[str, GLib.Variant] = {
            "DaemonVersion": GLib.Variant("s", "1.90.2"),
            "OnBattery": GLib.Variant("b", False),
        }

        if device_properties:
            self.device_properties = device_properties.copy()
        else:
            self.device_properties: dict[str, dict[str, GLib.Variant]] = {
                self.BATTERY0_OBJECT_PATH: {
                    "NativePath": GLib.Variant("s", "BAT0"),
                    "Vendor": GLib.Variant("s", "Konqi"),
                    "Model": GLib.Variant("s", "Primary"),
                    "Serial": GLib.Variant("s", "00150 2020/04/05"),
                    "UpdateTime": GLib.Variant('t', int(time.time())),
                    "Type": GLib.Variant("u", 2),  # Battery
                    "PowerSupply": GLib.Variant("b", True),
                    "HasHistory": GLib.Variant("b", False),
                    "HasStatistics": GLib.Variant("b", False),
                    "Online": GLib.Variant("b", False),  # only valid for AC
                    "Energy": GLib.Variant("d", 20.0),
                    "EnergyEmpty": GLib.Variant("d", 0.0),
                    "EnergyFull": GLib.Variant("d", 40.0),  # Wh
                    "EnergyFullDesign": GLib.Variant("d", 100.0),
                    "EnergyRate": GLib.Variant("d", -20.0),  # Charging
                    "Voltage": GLib.Variant("d", 12.184),
                    "ChargeCycles": GLib.Variant('i', 88),
                    "Luminosity": GLib.Variant("d", 0.0),
                    "TimeToEmpty": GLib.Variant("x", 0),
                    "TimeToFull": GLib.Variant("x", 0),
                    "Percentage": GLib.Variant("d", 50),
                    "Temperature": GLib.Variant("d", 20.0),
                    "IsPresent": GLib.Variant("b", True),
                    "State": GLib.Variant("u", 1),  # Charging
                    "IsRechargeable": GLib.Variant("b", True),
                    "Capacity": GLib.Variant("d", 40.0),
                    "Technology": GLib.Variant("u", 1),  # Lithium ion
                    "BatteryLevel": GLib.Variant("u", 1),  # None (the battery does not use a coarse level of battery reporting)
                    "WarningLevel": GLib.Variant("u", 1),  # None
                    "IconName": GLib.Variant("s", ""),
                },
                self.BATTERY1_OBJECT_PATH: {
                    "NativePath": GLib.Variant("s", "BAT1"),
                    "Vendor": GLib.Variant("s", "Katie"),
                    "Model": GLib.Variant("s", "Secondary"),
                    "Serial": GLib.Variant("s", "00150 2020/04/05"),
                    "UpdateTime": GLib.Variant('t', int(time.time())),
                    "Type": GLib.Variant("u", 2),  # Battery
                    "PowerSupply": GLib.Variant("b", True),
                    "HasHistory": GLib.Variant("b", False),
                    "HasStatistics": GLib.Variant("b", False),
                    "Online": GLib.Variant("b", False),  # only valid for AC
                    "Energy": GLib.Variant("d", 40.0),
                    "EnergyEmpty": GLib.Variant("d", 0.0),
                    "EnergyFull": GLib.Variant("d", 40.0),
                    "EnergyFullDesign": GLib.Variant("d", 100.0),
                    "EnergyRate": GLib.Variant("d", 0.0),  # Not Charging
                    "Voltage": GLib.Variant("d", 12.184),
                    "ChargeCycles": GLib.Variant('i', 88),
                    "Luminosity": GLib.Variant("d", 0.0),
                    "TimeToEmpty": GLib.Variant("x", 0),
                    "TimeToFull": GLib.Variant("x", 0),
                    "Percentage": GLib.Variant("d", 100),
                    "Temperature": GLib.Variant("d", 30.0),
                    "IsPresent": GLib.Variant("b", True),
                    "State": GLib.Variant("u", 4),  # Fully Charged
                    "IsRechargeable": GLib.Variant("b", True),
                    "Capacity": GLib.Variant("d", 60.0),
                    "Technology": GLib.Variant("u", 1),  # Lithium ion
                    "BatteryLevel": GLib.Variant("u", 1),  # None (the battery does not use a coarse level of battery reporting)
                    "WarningLevel": GLib.Variant("u", 1),  # None
                    "IconName": GLib.Variant("s", ""),
                },
                self.AC_OBJECT_PATH: {
                    "NativePath": GLib.Variant("s", "AC"),
                    "Vendor": GLib.Variant("s", ""),
                    "Model": GLib.Variant("s", ""),
                    "Serial": GLib.Variant("s", ""),
                    "UpdateTime": GLib.Variant('t', int(time.time())),
                    "Type": GLib.Variant("u", 1),  # Line Power
                    "PowerSupply": GLib.Variant("b", True),
                    "HasHistory": GLib.Variant("b", False),
                    "HasStatistics": GLib.Variant("b", False),
                    "Online": GLib.Variant("b", True),
                    "Energy": GLib.Variant("d", 0.0),  # only valid for batteries
                    "EnergyEmpty": GLib.Variant("d", 0.0),  # only valid for batteries
                    "EnergyFull": GLib.Variant("d", 0.0),  # only valid for batteries
                    "EnergyFullDesign": GLib.Variant("d", 0.0),  # only valid for batteries
                    "EnergyRate": GLib.Variant("d", 0.0),  # only valid for batteries
                    "Voltage": GLib.Variant("d", 0.0),
                    "ChargeCycles": GLib.Variant('i', -1),  # only valid for batteries
                    "Luminosity": GLib.Variant("d", 0.0),
                    "TimeToEmpty": GLib.Variant("x", 0),  # only valid for batteries
                    "TimeToFull": GLib.Variant("x", 0),  # only valid for batteries
                    "Percentage": GLib.Variant("d", 0.0),  # only valid for batteries
                    "Temperature": GLib.Variant("d", 0.0),  # only valid for batteries
                    "IsPresent": GLib.Variant("b", False),  # only valid for batteries
                    "State": GLib.Variant("u", 0),  # Unknown
                    "IsRechargeable": GLib.Variant("b", False),
                    "Capacity": GLib.Variant("d", 0.0),  # only valid for batteries
                    "Technology": GLib.Variant("u", 0),  # Unknown, only valid for batteries
                    "WarningLevel": GLib.Variant("u", 1),  # None
                    "BatteryLevel": GLib.Variant("u", 1),  # None
                    "IconName": GLib.Variant("s", ""),
                },
                self.WIRELESS_MOUSE_OBJECT_PATH: {
                    "NativePath": GLib.Variant("s", "hidpp_battery_0"),
                    "Vendor": GLib.Variant("s", "KDE"),
                    "Model": GLib.Variant("s", "Gaming Mouse"),
                    "Serial": GLib.Variant("s", "1234-5678-90"),
                    "UpdateTime": GLib.Variant('t', int(time.time())),
                    "Type": GLib.Variant("u", 5),  # Mouse
                    "PowerSupply": GLib.Variant("b", False),
                    "HasHistory": GLib.Variant("b", False),
                    "HasStatistics": GLib.Variant("b", False),
                    "Online": GLib.Variant("b", False),  # only valid for AC
                    "Energy": GLib.Variant("d", 0.0),  # only valid for batteries
                    "EnergyEmpty": GLib.Variant("d", 0.0),  # only valid for batteries
                    "EnergyFull": GLib.Variant("d", 0.0),  # only valid for batteries
                    "EnergyFullDesign": GLib.Variant("d", 0.0),  # only valid for batteries
                    "EnergyRate": GLib.Variant("d", 0.0),  # only valid for batteries
                    "Voltage": GLib.Variant("d", 0.0),
                    "ChargeCycles": GLib.Variant('i', -1),  # only valid for batteries
                    "Luminosity": GLib.Variant("d", 0.0),
                    "TimeToEmpty": GLib.Variant("x", 0),  # only valid for batteries
                    "TimeToFull": GLib.Variant("x", 0),  # only valid for batteries
                    "Percentage": GLib.Variant("d", 100.0),  # only valid for batteries
                    "Temperature": GLib.Variant("d", 0.0),  # only valid for batteries
                    "IsPresent": GLib.Variant("b", False),  # only valid for batteries
                    "State": GLib.Variant("u", 0),  # Unknown, only valid for batteries
                    "IsRechargeable": GLib.Variant("b", False),  # only valid for batteries
                    "Capacity": GLib.Variant("d", 0.0),  # only valid for batteries
                    "Technology": GLib.Variant("u", 0),  # Unknown, only valid for batteries
                    "WarningLevel": GLib.Variant("u", 1),  # None
                    "BatteryLevel": GLib.Variant("u", 8),  # Full
                    "IconName": GLib.Variant("s", ""),
                },
            }
        self.device_object_paths: GLib.Variant = GLib.Variant("(ao)", [list(self.device_properties.keys())])

        self.display_device_properties: dict[str, dict[str, GLib.Variant]] = {}
        self.__enable_display_device: bool = enable_display_device
        self.update_display_device_properties(self.BATTERY0_OBJECT_PATH)

        self.is_online: bool = False
        self.registered_event = threading.Event()

        self.__owner_id: int = Gio.bus_own_name(Gio.BusType.SYSTEM, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.__owner_id > 0

    def quit(self) -> None:
        if self.__enable_display_device:
            self.__connection.unregister_object(self.__display_device_reg_id)
            self.__display_device_reg_id = 0
        [self.__connection.unregister_object(self.__device_reg_id_map[object_path]) for object_path in self.__device_reg_id_map]
        self.__device_reg_id_map = {}
        self.__connection.unregister_object(self.__upower_reg_id)
        self.__upower_reg_id = 0
        Gio.bus_unown_name(self.__owner_id)
        self.__connection.flush_sync(None)  # Otherwise flaky

    def set_upower_property(self, property_name: str, value: GLib.Variant) -> None:
        self.upower_properties[property_name] = value
        changed_properties = GLib.Variant("a{sv}", {
            property_name: self.upower_properties[property_name],
        })
        Gio.DBusConnection.emit_signal(self.__connection, None, self.OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.BUS_NAME), changed_properties, GLib.Variant("as", ())))

    def add_device(self, object_path: str, properties: dict[str, GLib.Variant]) -> None:
        """
        Helper function to add a device to upower with the given object path and properties
        """
        assert object_path not in self.device_properties
        assert object_path not in self.__device_reg_id_map
        self.device_properties[object_path] = properties
        with open("../applets/batterymonitor/dbus/org.freedesktop.UPower.Device.xml", encoding="utf-8") as file_handler:
            introspection_data = Gio.DBusNodeInfo.new_for_xml(skip_doc(file_handler))
            reg_id: int = self.__connection.register_object(object_path, introspection_data.interfaces[0], self.device_handle_method_call, self.device_handle_get_property, self.device_handle_set_property)
            assert reg_id > 0
            self.__device_reg_id_map[object_path] = reg_id

        Gio.DBusConnection.emit_signal(self.__connection, None, self.OBJECT_PATH, "org.freedesktop.UPower", "DeviceAdded", GLib.Variant("(o)", [object_path]))

    def remove_device(self, object_path: str) -> None:
        """
        Helper function to remove a device from upower with the given object path
        """
        assert object_path in self.device_properties
        assert object_path in self.__device_reg_id_map
        Gio.DBusConnection.emit_signal(self.__connection, None, self.OBJECT_PATH, "org.freedesktop.UPower", "DeviceRemoved", GLib.Variant("(o)", [object_path]))
        self.__connection.unregister_object(self.__device_reg_id_map[object_path])
        del self.__device_reg_id_map[object_path]
        del self.device_properties[object_path]

    def set_device_property(self, object_path: str, property_name: str, value: GLib.Variant, update_time: int | None = None) -> None:
        assert object_path != self.DISPLAY_DEVICE_OBJECT_PATH
        if update_time:
            self.device_properties[object_path]["UpdateTime"] = GLib.Variant('t', update_time)
        else:
            self.device_properties[object_path]["UpdateTime"] = GLib.Variant('t', int(time.time()))

        self.device_properties[object_path][property_name] = value
        changed_properties = GLib.Variant("a{sv}", {
            property_name: self.device_properties[object_path][property_name],
            "UpdateTime": self.device_properties[object_path]["UpdateTime"],
        })
        Gio.DBusConnection.emit_signal(self.__connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.DEVICE_IFACE_NAME), changed_properties, GLib.Variant("as", ())))

        if self.__enable_display_device:
            self.update_display_device_properties(self.BATTERY0_OBJECT_PATH)
            changed_properties: dict[str, GLib.Variant] = {
                property_name: self.display_device_properties[property_name],
                "UpdateTime": self.display_device_properties["UpdateTime"],
            }
            if property_name == "IsPresent":
                changed_properties["Energy"] = self.display_device_properties["Energy"]
                changed_properties["EnergyEmpty"] = self.display_device_properties["EnergyEmpty"]
                changed_properties["EnergyFull"] = self.display_device_properties["EnergyFull"]
                changed_properties["EnergyFullDesign"] = self.display_device_properties["EnergyFullDesign"]
                changed_properties["EnergyRate"] = self.display_device_properties["EnergyRate"]
                changed_properties["TimeToEmpty"] = self.display_device_properties["TimeToEmpty"]
                changed_properties["TimeToFull"] = self.display_device_properties["TimeToFull"]
                changed_properties["Percentage"] = self.display_device_properties["Percentage"]
                changed_properties["Capacity"] = self.display_device_properties["Capacity"]

            Gio.DBusConnection.emit_signal(self.__connection, None, self.DISPLAY_DEVICE_OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.DEVICE_IFACE_NAME), GLib.Variant("a{sv}", changed_properties), GLib.Variant("as", ())))

    def update_display_device_properties(self, default_object_path: str) -> None:
        if not self.__enable_display_device:
            return

        def condition(e: str) -> bool:
            return self.device_properties[e]["PowerSupply"].get_boolean() and self.device_properties[e]["IsPresent"].get_boolean()

        self.display_device_properties = {
            "NativePath": GLib.Variant("s", ""),
            "Vendor": GLib.Variant("s", ""),
            "Model": GLib.Variant("s", ""),
            "Serial": GLib.Variant("s", ""),
            "UpdateTime": GLib.Variant('t', self.device_properties[default_object_path]["UpdateTime"].get_uint64() if default_object_path in self.device_properties else int(time.time())),
            "Type": GLib.Variant("u", 2),  # Battery
            "PowerSupply": GLib.Variant("b", True),
            "HasHistory": GLib.Variant("b", self.device_properties[default_object_path]["HasHistory"].get_boolean() if default_object_path in self.device_properties else False),
            "HasStatistics": GLib.Variant("b", self.device_properties[default_object_path]["HasStatistics"].get_boolean() if default_object_path in self.device_properties else False),
            "Online": GLib.Variant("b", self.device_properties[default_object_path]["Online"].get_boolean() if default_object_path in self.device_properties else False),
            "Energy": GLib.Variant("d", sum([self.device_properties[e]["Energy"].get_double() for e in self.device_properties if condition(e)])),
            "EnergyEmpty": GLib.Variant("d", sum([self.device_properties[e]["EnergyEmpty"].get_double() for e in self.device_properties if condition(e)])),
            "EnergyFull": GLib.Variant("d", sum([self.device_properties[e]["EnergyFull"].get_double() for e in self.device_properties if condition(e)])),  # Wh
            "EnergyFullDesign": GLib.Variant("d", sum([self.device_properties[e]["EnergyFullDesign"].get_double() for e in self.device_properties if condition(e)])),
            "EnergyRate": GLib.Variant("d", sum([self.device_properties[e]["EnergyRate"].get_double() for e in self.device_properties if condition(e)])),
            "Voltage": GLib.Variant("d", self.device_properties[default_object_path]["Voltage"].get_double() if default_object_path in self.device_properties else 0.0),
            "ChargeCycles": GLib.Variant('i', self.device_properties[default_object_path]["ChargeCycles"].get_int32() if default_object_path in self.device_properties else 0),
            "Luminosity": GLib.Variant("d", self.device_properties[default_object_path]["Luminosity"].get_double() if default_object_path in self.device_properties else 0.0),
            "TimeToEmpty": GLib.Variant("x", sum([self.device_properties[e]["TimeToEmpty"].get_int64() for e in self.device_properties if condition(e)])),
            "TimeToFull": GLib.Variant("x", sum([self.device_properties[e]["TimeToFull"].get_int64() for e in self.device_properties if condition(e)])),
            "Percentage": GLib.Variant("d", 100 * sum([self.device_properties[e]["Energy"].get_double() for e in self.device_properties if condition(e)]) / sum([self.device_properties[e]["EnergyFull"].get_double() for e in self.device_properties if condition(e)])),
            "Temperature": GLib.Variant("d", self.device_properties[default_object_path]["Temperature"].get_double() if default_object_path in self.device_properties else 20.0),
            "IsPresent": GLib.Variant("b", self.device_properties[default_object_path]["IsPresent"].get_boolean() if default_object_path in self.device_properties else True),
            "State": GLib.Variant("u", self.device_properties[default_object_path]["State"].get_uint32() if default_object_path in self.device_properties else 1),  # Charging
            "IsRechargeable": GLib.Variant("b", self.device_properties[default_object_path]["IsRechargeable"].get_boolean() if default_object_path in self.device_properties else True),
            "Capacity": GLib.Variant("d", sum([self.device_properties[e]["Capacity"].get_double() for e in self.device_properties if condition(e)])),
            "Technology": GLib.Variant("u", self.device_properties[default_object_path]["Technology"].get_uint32() if default_object_path in self.device_properties else 1),  # Lithium ion
            "BatteryLevel": GLib.Variant("u", self.device_properties[default_object_path]["BatteryLevel"].get_uint32() if default_object_path in self.device_properties else 1),  # None (the battery does not use a coarse level of battery reporting)
            "WarningLevel": GLib.Variant("u", self.device_properties[default_object_path]["WarningLevel"].get_uint32() if default_object_path in self.device_properties else 1),  # None
            "IconName": GLib.Variant("s", self.device_properties[default_object_path]["IconName"].get_string() if default_object_path in self.device_properties else ""),
        }

    def set_energy_props(self, object_path: str, energy_rate: float | None = None, percentage: float | None = None, update_time: int | None = None) -> None:
        """
        Helper function to update energy rate and remaining time.
        @param percentage 0-100
        """
        changed_properties: dict[str, GLib.Variant] = {}
        if energy_rate is not None:
            self.device_properties[object_path]["EnergyRate"] = GLib.Variant("d", energy_rate)
            changed_properties["EnergyRate"] = self.device_properties[object_path]["EnergyRate"]
        else:
            energy_rate = self.device_properties[object_path]["EnergyRate"].get_double()

        if update_time is not None:
            self.device_properties[object_path]["UpdateTime"] = GLib.Variant('t', update_time)
            self.device_properties[self.BATTERY0_OBJECT_PATH]["UpdateTime"] = self.device_properties[object_path]["UpdateTime"]
        else:
            self.device_properties[object_path]["UpdateTime"] = GLib.Variant('t', self.device_properties[self.BATTERY0_OBJECT_PATH]["UpdateTime"].get_uint64() + 100)
            self.device_properties[self.BATTERY0_OBJECT_PATH]["UpdateTime"] = self.device_properties[object_path]["UpdateTime"]
        changed_properties["UpdateTime"] = self.device_properties[object_path]["UpdateTime"]

        if percentage is not None:
            self.device_properties[object_path]["Percentage"] = GLib.Variant("d", percentage)
            self.device_properties[object_path]["Energy"] = GLib.Variant("d", percentage / 100 * self.device_properties[object_path]["EnergyFull"].get_double())
            changed_properties["Percentage"] = self.device_properties[object_path]["Percentage"]
            changed_properties["Energy"] = self.device_properties[object_path]["Energy"]
            if percentage == 100:
                self.device_properties[object_path]["State"] = GLib.Variant("u", 4)  # Charging
                changed_properties["State"] = self.device_properties[object_path]["State"]
        else:
            percentage = self.device_properties[object_path]["Percentage"].get_double()

        if energy_rate < 0:
            self.device_properties[object_path]["TimeToEmpty"] = GLib.Variant("x", 0)
            self.device_properties[object_path]["TimeToFull"] = GLib.Variant("x", int((100 - percentage) / 100 * self.device_properties[object_path]["EnergyFull"].get_double() / -energy_rate * 3600))  # seconds
            self.device_properties[object_path]["State"] = GLib.Variant("u", 1)  # Charging
            changed_properties["State"] = self.device_properties[object_path]["State"]
        elif energy_rate > 0:
            self.device_properties[object_path]["TimeToEmpty"] = GLib.Variant("x", int(self.device_properties[object_path]["Energy"].get_double() / energy_rate * 3600))  # seconds
            self.device_properties[object_path]["TimeToFull"] = GLib.Variant("x", 0)
            self.device_properties[object_path]["State"] = GLib.Variant("u", 2)  # Discharging
            changed_properties["State"] = self.device_properties[object_path]["State"]
        else:
            self.device_properties[object_path]["TimeToEmpty"] = GLib.Variant("x", 0)
            self.device_properties[object_path]["TimeToFull"] = GLib.Variant("x", 0)

        changed_properties["TimeToEmpty"] = self.device_properties[object_path]["TimeToEmpty"]
        changed_properties["TimeToFull"] = self.device_properties[object_path]["TimeToFull"]

        Gio.DBusConnection.emit_signal(self.__connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.DEVICE_IFACE_NAME), GLib.Variant("a{sv}", changed_properties), GLib.Variant("as", ())))

        if self.__enable_display_device:
            self.update_display_device_properties(self.BATTERY0_OBJECT_PATH)
            changed_properties = GLib.Variant("a{sv}", {
                "EnergyRate": self.display_device_properties["EnergyRate"],
                "Percentage": self.display_device_properties["Percentage"],
                "Energy": self.display_device_properties["Energy"],
                "State": self.display_device_properties["State"],
                "TimeToFull": self.display_device_properties["TimeToFull"],
                "TimeToEmpty": self.display_device_properties["TimeToEmpty"],
                "UpdateTime": self.display_device_properties["UpdateTime"],
            })
            Gio.DBusConnection.emit_signal(self.__connection, None, self.DISPLAY_DEVICE_OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.DEVICE_IFACE_NAME), changed_properties, GLib.Variant("as", ())))

    def set_ac_plugged(self) -> None:
        """
        Helper function to simulate plugging in the AC line
        """
        self.device_properties[self.AC_OBJECT_PATH]["Online"] = GLib.Variant("b", True)
        self.device_properties[self.AC_OBJECT_PATH]["IsPresent"] = GLib.Variant("b", True)
        ac_changed_properties = GLib.Variant("a{sv}", {
            "Online": self.device_properties[self.AC_OBJECT_PATH]["Online"],
            "IsPresent": self.device_properties[self.AC_OBJECT_PATH]["IsPresent"],
        })
        Gio.DBusConnection.emit_signal(self.__connection, None, self.AC_OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", self.DEVICE_IFACE_NAME), ac_changed_properties, GLib.Variant("as", ())))

        self.set_energy_props(self.BATTERY0_OBJECT_PATH, -20.0, 90.0)
        self.set_upower_property("OnBattery", GLib.Variant("b", False))

    def set_ac_unplugged(self) -> None:
        """
        Helper function to simulate unplugging the AC line
        """
        self.device_properties[self.AC_OBJECT_PATH]["Online"] = GLib.Variant("b", False)
        self.device_properties[self.AC_OBJECT_PATH]["IsPresent"] = GLib.Variant("b", False)
        ac_changed_properties = GLib.Variant("a{sv}", {
            "Online": self.device_properties[self.AC_OBJECT_PATH]["Online"],
            "IsPresent": self.device_properties[self.AC_OBJECT_PATH]["IsPresent"],
        })

        self.set_energy_props(self.BATTERY0_OBJECT_PATH, 20.0, 80.0)
        self.set_upower_property("OnBattery", GLib.Variant("b", True))

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        """
        Interface is ready, now register objects.
        """
        self.__connection = connection

        with open("../applets/batterymonitor/dbus/org.freedesktop.UPower.xml", encoding="utf-8") as file_handler:
            introspection_data = Gio.DBusNodeInfo.new_for_xml(skip_doc(file_handler))
            self.__upower_reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.upower_handle_method_call, self.upower_handle_get_property, self.upower_handle_set_property)
            assert self.__upower_reg_id > 0

        assert len(self.__device_reg_id_map) == 0
        with open("../applets/batterymonitor/dbus/org.freedesktop.UPower.Device.xml", encoding="utf-8") as file_handler:
            introspection_data = Gio.DBusNodeInfo.new_for_xml(skip_doc(file_handler))
            for object_path in self.device_properties:
                reg_id: int = connection.register_object(object_path, introspection_data.interfaces[0], self.device_handle_method_call, self.device_handle_get_property, self.device_handle_set_property)
                assert reg_id > 0
                self.__device_reg_id_map[object_path] = reg_id
            if self.__enable_display_device:
                self.__display_device_reg_id = connection.register_object(self.DISPLAY_DEVICE_OBJECT_PATH, introspection_data.interfaces[0], self.device_handle_method_call, self.device_handle_get_property, self.device_handle_set_property)
                assert self.__display_device_reg_id > 0

        self.is_online = True
        self.registered_event.set()

    def upower_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        Handles method calls for org.freedesktop.UPower
        """
        assert interface_name == self.BUS_NAME, f"Unknown interface {interface_name}"
        print(f"upower call {method_name}", file=sys.stderr, flush=True)

        if method_name == "EnumerateDevices":
            invocation.return_value(self.device_object_paths)
        elif method_name == "GetDisplayDevice":
            invocation.return_value(GLib.Variant("(o)", ["/org/freedesktop/UPower/devices/DisplayDevice"] if self.__enable_display_device else ["/"]))
        elif method_name == "GetCriticalAction":
            invocation.return_value(GLib.Variant("(s)", ["PowerOff"]))
        else:
            assert False, f"Unknown method {method_name}"

    def upower_handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: Any):
        """
        Handles properties for org.freedesktop.UPower
        """
        if value not in self.upower_properties:
            print(f"{value} does not exist", file=sys.stderr, flush=True)
            return None

        print(f"upower get_property {value}")
        return self.upower_properties[value]

    def upower_handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        """
        Handles properties for org.freedesktop.UPower
        """
        assert False, "Only read-only properties"

    def device_handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        Handles method calls for org.freedesktop.UPower.Device
        """
        assert interface_name == self.DEVICE_IFACE_NAME, f"Unknown interface {interface_name}"
        assert method_name in ("Refresh", "GetHistory", "GetStatistics"), f"Unknown method {method_name}"
        print(f"device {object_path} call {method_name}", file=sys.stderr, flush=True)

    def device_handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: Any):
        """
        Handles properties for org.freedesktop.UPower.Device
        """
        assert interface_name == self.DEVICE_IFACE_NAME, f"Wrong interface name {interface_name} from {sender}"
        assert object_path in self.device_properties or (object_path == self.DISPLAY_DEVICE_OBJECT_PATH and self.__enable_display_device), f"Unknown object path {object_path}"
        print(f"device {object_path} get_property {value}")

        return self.device_properties[object_path][value] if object_path != self.DISPLAY_DEVICE_OBJECT_PATH else self.display_device_properties[value]

    def device_handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        """
        Handles properties for org.freedesktop.UPower.Device
        """
        assert False, "Only read-only properties"
