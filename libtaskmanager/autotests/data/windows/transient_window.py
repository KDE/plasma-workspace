#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT
# Idea inspired by Demitrius Belai's Java code: https://bugs.kde.org/attachment.cgi?id=170202
# See also: https://bugs.kde.org/show_bug.cgi?id=484647

# pylint: disable=global-statement

import os

# set_urgency_hint only works on X11
os.environ["GDK_BACKEND"] = "x11"
# Otherwise the following tests will fail
os.environ["NO_AT_BRIDGE"] = "1"
os.environ["GTK_A11Y"] = "none"
# Avoid temporary files
os.environ["GSETTINGS_BACKEND"] = "memory"
os.environ["GVFS_DISABLE_FUSE"] = "1"
os.environ["GIO_USE_VFS"] = "local"

import logging
from typing import Final, cast

import gi

gi.require_version("Gtk", "4.0")
gi.require_version("GdkX11", "4.0")
from gi.repository import GdkX11, Gio, GLib, Gtk

app: Gtk.Application | None = None
frame_1: Gtk.Window | None = None
frame_2: Gtk.Window | None = None
dialog_1: Gtk.Window | None = None
dbus_interface = None


class OrgKdeBug484647:
    """
    D-Bus interface for org.kde.bug484647
    """

    BUS_NAME: Final = "org.kde.bug484647"
    OBJECT_PATH: Final = "/484647"
    INTERFACE_NAME: Final = "org.kde.bug484647"

    connection: Gio.DBusConnection

    def __init__(self) -> None:
        self.reg_id: int = 0
        self.owner_id: int = Gio.bus_own_name(Gio.BusType.SESSION, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.owner_id > 0

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        """
        The interface is ready, now register objects.
        """
        self.connection = connection
        introspection_data = Gio.DBusNodeInfo.new_for_xml("""
<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
    "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.kde.bug484647">
    <method name="ChangeLeader1"/>
    <method name="ChangeLeader2"/>
    <method name="UnsetLeader"/>
    <method name="ShowTransientWindow"/>
    <method name="CloseTransientWindow"/>
    <method name="SetUrgencyHint"/>
    <method name="UnsetUrgencyHint"/>
  </interface>
</node>
""")
        self.reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.handle_method_call, None, None)
        assert self.reg_id > 0
        logging.info("interface registered")

    def handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        assert dialog_1 is not None and frame_1 is not None
        logging.info("method call %s", method_name)

        if method_name == "ChangeLeader1":
            dialog_1.set_modal(True)
            dialog_1.set_transient_for(frame_1)
            invocation.return_value(None)
        elif method_name == "ChangeLeader2":
            dialog_1.set_modal(True)
            dialog_1.set_transient_for(frame_2)
            invocation.return_value(None)
        elif method_name == "UnsetLeader":
            dialog_1.set_modal(False)
            dialog_1.set_transient_for(None)
            invocation.return_value(None)
        elif method_name == "ShowTransientWindow":
            dialog_1.set_visible(True)
            invocation.return_value(None)
        elif method_name == "CloseTransientWindow":
            dialog_1.set_visible(False)
            invocation.return_value(None)
        elif method_name == "SetUrgencyHint":
            cast(GdkX11.X11Surface, dialog_1.get_surface()).set_urgency_hint(True)
            invocation.return_value(None)
        elif method_name == "UnsetUrgencyHint":
            cast(GdkX11.X11Surface, dialog_1.get_surface()).set_urgency_hint(False)
            invocation.return_value(None)
        else:
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.UNKNOWN_METHOD, f"Unknown method {method_name}")


def on_show(widget) -> None:
    global dialog_1
    dialog_1 = Gtk.Window(application=app, title="Dialog1")
    dialog_1.set_default_size(100, 100)
    dialog_1.set_modal(True)
    dialog_1.set_transient_for(widget)
    dialog_1.set_visible(True)


def on_activate(_app: Gtk.Application) -> None:
    global dbus_interface, frame_1, frame_2
    dbus_interface = OrgKdeBug484647()
    frame_1 = Gtk.Window(application=_app, title="Frame1")
    frame_2 = Gtk.Window(application=_app, title="Frame2")

    frame_1.set_default_size(200, 200)
    frame_2.set_default_size(200, 200)

    frame_2.connect("show", on_show)

    frame_1.set_visible(True)
    frame_2.set_visible(True)

    GLib.timeout_add_seconds(60, _app.quit)


if __name__ == "__main__":
    logging.getLogger().setLevel(logging.INFO)
    app = Gtk.Application(application_id='org.kde.testwindow')
    app.connect('activate', on_activate)
    app.run(None)
