#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

# pylint: disable=too-many-arguments

import logging
from typing import Final

from gi.repository import Gio, GLib


class OrgKdeTestdbusactivation:
    """
    DBus interface for org.kde.testdbusactivation
    """

    BUS_NAME: Final = "org.kde.testdbusactivation"
    OBJECT_PATH: Final = "/test"

    connection: Gio.DBusConnection | None = None

    def __init__(self) -> None:
        self.reg_id: int = 0
        self.owner_id: int = Gio.bus_own_name(Gio.BusType.SESSION, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.owner_id > 0

    def quit(self) -> None:
        if self.connection is None:
            return
        self.connection.unregister_object(self.reg_id)
        Gio.bus_unown_name(self.owner_id)
        self.connection.flush_sync(None)

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        """
        Interface is ready, now register objects.
        """
        self.connection = connection

        introspection_data = Gio.DBusNodeInfo.new_for_xml("""
<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
    "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.kde.testdbusactivation">
    <method name="ping" />
  </interface>
</node>
""")
        self.reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.handle_method_call, None, None)
        assert self.reg_id > 0
        logging.info("Interface is ready")

    def handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        invocation.return_value(None)


dbus_interface: OrgKdeTestdbusactivation | None = None

if __name__ == '__main__':
    logging.getLogger().setLevel(logging.INFO)
    dbus_interface = OrgKdeTestdbusactivation()
    GLib.MainLoop().run()
