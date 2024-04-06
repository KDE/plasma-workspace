#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import sys
import threading
from typing import Final

from gi.repository import Gio, GLib


class GLibMainLoopThread(threading.Thread):

    def __init__(self) -> None:
        # Set up D-Bus loop
        self.loop = GLib.MainLoop()
        self.failSafeTimer = threading.Timer(600, self.loop.quit)

        # Create the thread
        super().__init__()

    def run(self) -> None:
        self.failSafeTimer.start()
        self.loop.run()

    def quit(self) -> None:
        self.failSafeTimer.cancel()
        self.loop.quit()


class OrgKdeKSplash:
    """
    D-Bus interfaces for org.kde.KSplash
    """

    BUS_NAME: Final = "org.kde.KSplash"
    OBJECT_PATH: Final = "/KSplash"

    connection: Gio.DBusConnection

    def __init__(self) -> None:
        self.reg_id: int = 0
        self.registered_event = threading.Event()
        self.owner_id: int = Gio.bus_own_name(Gio.BusType.SESSION, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.owner_id > 0

        self.stage: str = ""
        self.stage_set_event = threading.Event()

    def quit(self) -> None:
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
  <interface name="org.kde.KSplash">
    <method name="setStage">
      <arg name="stage" type="s" direction="in"/>
    </method>
  </interface>
</node>
""")
        self.reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.handle_method_call, None, None)
        assert self.reg_id > 0

        self.registered_event.set()

    def handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        Handles method calls for org.kde.KSplash
        """
        assert interface_name == self.BUS_NAME, f"Unknown interface {interface_name}"
        print(f"ksplash call {method_name}", file=sys.stderr, flush=True)

        if method_name == "setStage":
            if not parameters.is_of_type(GLib.VariantType("(s)")):
                invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.INVALID_ARGS, f"Incorrect signature {parameters.get_type_string()}")
                return
            self.stage = parameters.unpack()[0]
            self.stage_set_event.set()
            invocation.return_value(None)
        else:
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.UNKNOWN_METHOD, f"Unknown method {method_name}")


if __name__ == '__main__':
    loop = GLibMainLoopThread()
    loop.start()
    dbus_interface = OrgKdeKSplash()
