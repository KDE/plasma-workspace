#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import sys
import threading
from typing import Any, Final

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
        self.default_variant = GLib.Variant("(bnixqutdysgoa{sv}v)", [
            True,
            32767,
            2147483647,
            21474836470,
            65535,
            4294967295,
            18446744073709551615,
            1.23,
            255,
            "abc",
            "(bnixqutdysgoa{sv}v)",
            "/abc/def",
            {
                "int64": GLib.Variant("n", 32767),
                "objectPath": GLib.Variant("o", "/abc/def"),
                "string": GLib.Variant("s", "string"),
            },
            GLib.Variant("s", "variant"),
        ])
        self.read_write_prop = 1

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
    <property name="readOnlyProp" type="(bnixqutdysgoa{sv}v)" access="read"/>
    <property name="readWriteProp" type="i" access="readwrite"/>
    <method name="setStage">
      <arg name="stage" type="s" direction="in"/>
    </method>
    <method name="updatePropertySilently"/>
    <method name="ping" />
    <signal name="pong">
      <arg name="arg1" type="b"/>
      <arg name="arg2" type="n"/>
      <arg name="arg3" type="i"/>
      <arg name="arg4" type="x"/>
      <arg name="arg5" type="q"/>
      <arg name="arg6" type="u"/>
      <arg name="arg7" type="t"/>
      <arg name="arg8" type="d"/>
      <arg name="arg9" type="y"/>
      <arg name="arg10" type="s"/>
      <arg name="arg11" type="g"/>
      <arg name="arg12" type="o"/>
      <arg name="arg13" type="a{sv}"/>
      <arg name="arg14" type="v"/>
    </signal>
    <method name="testAllTypes">
      <arg name="arg1" type="b" direction="in"/>
      <arg name="arg2" type="n" direction="in"/>
      <arg name="arg3" type="i" direction="in"/>
      <arg name="arg4" type="x" direction="in"/>
      <arg name="arg5" type="q" direction="in"/>
      <arg name="arg6" type="u" direction="in"/>
      <arg name="arg7" type="t" direction="in"/>
      <arg name="arg8" type="d" direction="in"/>
      <arg name="arg9" type="y" direction="in"/>
      <arg name="arg10" type="s" direction="in"/>
      <arg name="arg11" type="g" direction="in"/>
      <arg name="arg12" type="o" direction="in"/>
      <arg name="arg13" type="a{sv}" direction="in"/>
      <arg name="arg14" type="v" direction="in"/>
      <arg name="outArg" type="(bnixqutdysgoa{sv}v)" direction="out"/>
    </method>
    <method name="testListTypes">
      <arg name="arg1" type="ab" direction="in"/>
      <arg name="arg2" type="an" direction="in"/>
      <arg name="arg3" type="ai" direction="in"/>
      <arg name="arg4" type="ax" direction="in"/>
      <arg name="arg5" type="aq" direction="in"/>
      <arg name="arg6" type="au" direction="in"/>
      <arg name="arg7" type="at" direction="in"/>
      <arg name="arg8" type="ad" direction="in"/>
      <arg name="arg9" type="ay" direction="in"/>
      <arg name="arg10" type="as" direction="in"/>
      <arg name="arg11" type="ag" direction="in"/>
      <arg name="arg12" type="ao" direction="in"/>
      <arg name="arg13" type="aa{sv}" direction="in"/>
      <arg name="arg14" type="av" direction="in"/>
      <arg name="outArg" type="(abanaiaxaqauatadayasagaoaa{sv}av)" direction="out"/>
    </method>
  </interface>
</node>
""")
        self.reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.handle_method_call, self.handle_get_property, self.handle_set_property)
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
        elif method_name == "updatePropertySilently":
            self.read_write_prop += 1
            invocation.return_value(None)
        elif method_name == "ping":
            Gio.DBusConnection.emit_signal(connection, None, object_path, interface_name, "pong", self.default_variant)
            invocation.return_value(None)
        elif method_name == "testAllTypes":
            if not parameters.is_of_type(GLib.VariantType("(bnixqutdysgoa{sv}v)")):
                invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.INVALID_ARGS, f"Incorrect signature {parameters.get_type_string()}")
                return
            # Check value types in the variant
            sv_map = parameters.get_child_value(12)
            first_entry_value = sv_map.get_child_value(0).get_child_value(1).get_child_value(0)
            second_entry_value = sv_map.get_child_value(1).get_child_value(1).get_child_value(0)
            third_entry_value = sv_map.get_child_value(2).get_child_value(1).get_child_value(0)
            if not first_entry_value.is_of_type(GLib.VariantType("n")) or not second_entry_value.is_of_type(GLib.VariantType("o")) or not third_entry_value.is_of_type(GLib.VariantType("s")):
                invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.INVALID_ARGS, f"Incorrect signature {first_entry_value.get_type_string()} {second_entry_value.get_type_string()} {third_entry_value.get_type_string()}")
                return
            invocation.return_value(GLib.Variant.new_tuple(self.default_variant))
        elif method_name == "testListTypes":
            if not parameters.is_of_type(GLib.VariantType("(abanaiaxaqauatadayasagaoaa{sv}av)")):
                invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.INVALID_ARGS, f"Incorrect signature {parameters.get_type_string()}")
                return
            ret = GLib.Variant("(abanaiaxaqauatadayasagaoaa{sv}av)", [
                [True],
                [32767],
                [2147483647],
                [21474836470],
                [65535],
                [4294967295],
                [18446744073709551615],
                [1.23],
                [255],
                ["abc"],
                ["(bnixqutdysgoa{sv}v)"],
                ["/abc/def"],
                [{
                    "int64": GLib.Variant("n", 32767),
                    "objectPath": GLib.Variant("o", "/abc/def"),
                    "string": GLib.Variant("s", "string"),
                }],
                [GLib.Variant("s", "variant")],
            ])
            invocation.return_value(GLib.Variant.new_tuple(ret))
        else:
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.UNKNOWN_METHOD, f"Unknown method {method_name}")

    def handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: Any):
        print(f"ksplash get {value}", file=sys.stderr, flush=True)
        if value == "readOnlyProp":
            return self.default_variant
        if value == "readWriteProp":
            return GLib.Variant("i", self.read_write_prop)
        return None

    def handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        print(f"ksplash set {key} to {value}", file=sys.stderr, flush=True)
        if key == "readWriteProp":
            self.read_write_prop = value.get_int32()
            changed_properties = GLib.Variant('a{sv}', {
                "readWriteProp": GLib.Variant("i", self.read_write_prop),
            })
            Gio.DBusConnection.emit_signal(connection, None, object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", GLib.Variant.new_tuple(GLib.Variant("s", interface_name), changed_properties, GLib.Variant('as', ())))
        else:
            return False
        return True


if __name__ == '__main__':
    loop = GLibMainLoopThread()
    loop.start()
    dbus_interface = OrgKdeKSplash()
