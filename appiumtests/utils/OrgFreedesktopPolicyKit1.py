#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# pylint: disable=too-many-arguments

# For FreeBSD CI which only has python 3.9
from __future__ import annotations

import logging
import os
import threading
from typing import Any, Final

from gi.repository import Gio, GLib

current_folder: Final = os.path.dirname(os.path.abspath(__file__))


class OrgFreedesktopPolicyKit1:
    """
    D-Bus interface for org.freedesktop.PolicyKit1 which authorizes all actions for testing purpose
    """

    BUS_NAME: Final = "org.freedesktop.PolicyKit1"
    OBJECT_PATH: Final = "/org/freedesktop/PolicyKit1/Authority"
    IFACE_NAME: Final = "org.freedesktop.PolicyKit1.Authority"

    __connection: Gio.DBusConnection

    def __init__(self) -> None:
        self.__reg_id: int = 0
        self.registered_event = threading.Event()
        self.__owner_id: int = Gio.bus_own_name(Gio.BusType.SYSTEM, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.__owner_id > 0

        self.registered_actions = GLib.Variant(
            "a(ssssssuuua{ss})",
            [
                [
                    "org.kde.powerdevil.backlighthelper.brightness",  # action_id
                    "",  # description
                    "",  # message
                    "",  # vendor_name
                    "",  # vendor_url
                    "",  # icon_name
                    0,  # implicit_any
                    0,  # implicit_active
                    5,  # implicit_active
                    {},  # annotations
                ],
                [
                    "org.kde.powerdevil.backlighthelper.brightnessmax",  # action_id
                    "",  # description
                    "",  # message
                    "",  # vendor_name
                    "",  # vendor_url
                    "",  # icon_name
                    0,  # implicit_any
                    0,  # implicit_active
                    5,  # implicit_active
                    {},  # annotations
                ],
                [
                    "org.kde.powerdevil.backlighthelper.setbrightness",  # action_id
                    "",  # description
                    "",  # message
                    "",  # vendor_name
                    "",  # vendor_url
                    "",  # icon_name
                    0,  # implicit_any
                    0,  # implicit_active
                    5,  # implicit_active
                    {},  # annotations
                ],
                [
                    "org.kde.powerdevil.backlighthelper.syspath",  # action_id
                    "",  # description
                    "",  # message
                    "",  # vendor_name
                    "",  # vendor_url
                    "",  # icon_name
                    0,  # implicit_any
                    0,  # implicit_active
                    5,  # implicit_active
                    {},  # annotations
                ],
            ])
        self.authorization_result = GLib.Variant(
            "(bba{ss})",
            [
                True,  # is_authorized
                False,  # is_challenge
                {}  # details
            ])
        self.polkit_properties: dict = {
            "BackendName": GLib.Variant("s", "js"),
            "BackendVersion": GLib.Variant("s", "123"),
            "BackendFeatures": GLib.Variant("u", 1),
        }

    def quit(self) -> None:
        self.__connection.unregister_object(self.__reg_id)
        self.__reg_id = 0
        Gio.bus_unown_name(self.__owner_id)
        self.__connection.flush_sync(None)  # Otherwise flaky

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        """
        Interface is ready, now register objects.
        """
        self.__connection = connection

        with open(os.path.join(current_folder, "org.freedesktop.PolicyKit1.Authority.xml"), encoding="utf-8") as file_handler:
            introspection_data = Gio.DBusNodeInfo.new_for_xml("\n".join(file_handler.readlines()))
            self.__reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.handle_method_call, self.handle_get_property, self.handle_set_property)
        assert self.__reg_id > 0

        self.registered_event.set()

    def handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        logging.info(f"polkit call {method_name} {parameters}")

        if method_name == "EnumerateActions":
            invocation.return_value(GLib.Variant.new_tuple(self.registered_actions))
        elif method_name == "CheckAuthorization":
            invocation.return_value(GLib.Variant.new_tuple(self.authorization_result))
        else:
            invocation.return_value(None)

    def handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: Any):
        logging.info(f"polkit get_property {value}")
        return self.polkit_properties.get(value, None)

    def handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        logging.info(f"polkit set_property {key} {value}")
        return False
