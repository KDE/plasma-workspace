#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import sys
import threading
from typing import Final

from gi.repository import Gio, GLib

CMAKE_BINARY_DIR: Final = os.getenv("CMAKE_BINARY_DIR")
QMLTEST_PATH: Final = os.getenv("QMLTEST_PATH")


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


class TestInterface:

    BUS_NAME: Final = "org.kde.mpristest"
    OBJECT_PATH: Final = "/mpristest"

    connection: Gio.DBusConnection
    pid: int

    def __init__(self) -> None:
        self.reg_id: int = 0
        self.registered_event = threading.Event()
        self.owner_id: int = Gio.bus_own_name(Gio.BusType.SESSION, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.owner_id > 0

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
  <interface name="org.kde.mpristest">
    <method name="GetPid">
      <arg name="outArg" type="i" direction="out"/>
    </method>
  </interface>
</node>
""")
        self.reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.handle_method_call, None, None)
        assert self.reg_id > 0

        self.registered_event.set()

    def handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        if method_name == "GetPid":
            invocation.return_value(GLib.Variant("(i)", [self.pid]))
        else:
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.UNKNOWN_METHOD, f"Unknown method {method_name}")


if __name__ == '__main__':
    assert QMLTEST_PATH is not None
    assert os.path.exists(QMLTEST_PATH)
    result: int = 1

    loop = GLibMainLoopThread()
    loop.start()
    dbus_interface = TestInterface()

    player_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "appiumtests", "applets", "mediacontrollertest")
    assert os.path.exists(player_dir)
    with subprocess.Popen(["python3", os.path.join(player_dir, "mediaplayer.py"), os.path.join(player_dir, "player_b.json")], stdout=sys.stdout, stderr=sys.stderr) as player:
        TestInterface.pid = player.pid
        args = [
            QMLTEST_PATH,
            "-input",
            os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", sys.argv[1]),
        ]
        if CMAKE_BINARY_DIR is not None:
            args += ["-import", CMAKE_BINARY_DIR]

        print("Starting test")
        test_runner = subprocess.Popen(args, stdout=sys.stdout, stderr=sys.stderr)
        try:
            result = test_runner.wait(timeout=60)
        except subprocess.TimeoutExpired as e:
            print("Timeout", e)
        finally:
            test_runner.terminate()
            player.terminate()
            test_runner.wait(10)

    dbus_interface.quit()
    loop.quit()

    sys.exit(result)
