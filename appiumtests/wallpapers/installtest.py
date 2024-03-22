#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import logging
import os
import subprocess
import sys
import tempfile
import threading
import unittest
from typing import Final

import gi

gi.require_version('GdkPixbuf', '2.0')
from gi.repository import GdkPixbuf, Gio, GLib

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "utils"))
from GLibMainLoopThread import GLibMainLoopThread

BIN_PATH: Final = os.path.join(os.getenv("CMAKE_BINARY_DIR", "/usr/bin/"), "plasma-apply-wallpaperimage")
DBUS_XML_PATH: Final = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "shell/dbus/org.kde.PlasmaShell.xml")


class OrgKdePlasmashell:
    """
    D-Bus interfaces for org.kde.plasmashell
    """

    BUS_NAME: Final = "org.kde.plasmashell"
    OBJECT_PATH: Final = "/PlasmaShell"
    INTERFACE_NAME: Final = "org.kde.PlasmaShell"

    connection: Gio.DBusConnection

    def __init__(self) -> None:
        self.reg_id: int = 0
        self.registered_event = threading.Event()
        self.owner_id: int = Gio.bus_own_name(Gio.BusType.SESSION, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.owner_id > 0

        self.script: str = ""
        self.script_set_event = threading.Event()

    def quit(self) -> None:
        self.connection.unregister_object(self.reg_id)
        Gio.bus_unown_name(self.owner_id)
        self.connection.flush_sync(None)

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        """
        Interface is ready, now register objects.
        """
        self.connection = connection
        with open(DBUS_XML_PATH, encoding="utf-8") as handler:
            properties_introspection_xml: str = '\n'.join(handler.readlines())
            introspection_data = Gio.DBusNodeInfo.new_for_xml(properties_introspection_xml)
            self.reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.handle_method_call, None, None)
        assert self.reg_id > 0
        self.registered_event.set()
        logging.info("interface registered")

    def handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        """
        Handles method calls
        """
        assert interface_name == self.INTERFACE_NAME, f"Unknown interface {interface_name}"
        logging.info("method call %s", method_name)

        if method_name == "evaluateScript":
            if not parameters.is_of_type(GLib.VariantType("(s)")):
                invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.INVALID_ARGS, f"Incorrect signature {parameters.get_type_string()}")
                return
            self.script = parameters.unpack()[0]
            self.script_set_event.set()
            invocation.return_value(GLib.Variant.new_tuple(GLib.Variant("s", "")))
        else:
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.UNKNOWN_METHOD, f"Unknown method {method_name}")


class InstallWallpaperTest(unittest.TestCase):
    """
    Tests for plasma-apply-wallpaperimage
    """

    loop_thread: GLibMainLoopThread
    plasmashell_interface: OrgKdePlasmashell

    @classmethod
    def setUpClass(cls) -> None:
        cls.loop_thread = GLibMainLoopThread()
        cls.loop_thread.start()
        cls.plasmashell_interface = OrgKdePlasmashell()

    def tearDown(self) -> None:
        self.plasmashell_interface.script_set_event.clear()

    @classmethod
    def tearDownClass(cls) -> None:
        cls.plasmashell_interface.quit()
        cls.loop_thread.quit()

    def test_0_install_image(self) -> None:
        """
        Install a png file
        """
        # Create an image file
        bits_per_sample = 8
        width = height = 16
        pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, bits_per_sample, width, height)
        pixbuf.fill(0xff0000ff)
        with tempfile.TemporaryDirectory() as temp_dir:
            image_path = os.path.join(temp_dir, "test.png")
            self.assertTrue(pixbuf.savev(image_path, "png"))
            subprocess.check_call([BIN_PATH, image_path])
            self.assertTrue(self.plasmashell_interface.script_set_event.wait(10))
            self.assertIn(image_path, self.plasmashell_interface.script)

    def test_1_install_kpackage(self) -> None:
        """
        Install a package
        """
        package_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "wallpapers/image/plugin/autotests/testdata/default/package")
        self.assertTrue(os.path.exists(package_path))
        subprocess.check_call([BIN_PATH, package_path])
        self.assertTrue(self.plasmashell_interface.script_set_event.wait(10))
        self.assertIn(package_path, self.plasmashell_interface.script)

    def test_2_install_nonexistent_path(self) -> None:
        """
        The file does not exist
        """
        self.assertRaises(subprocess.CalledProcessError, subprocess.check_call, [BIN_PATH, "thisfiledoesnotexist.png"])
        self.assertFalse(self.plasmashell_interface.script_set_event.wait(1))

    def test_3_install_dangerous_path(self) -> None:
        """
        There is a stray single quote in the filename of this wallpaper (')
        """
        # Create an image file
        bits_per_sample = 8
        width = height = 16
        pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, True, bits_per_sample, width, height)
        pixbuf.fill(0xff0000ff)
        with tempfile.TemporaryDirectory() as temp_dir:
            image_path = os.path.join(temp_dir, "konqi'sfavorite.png")
            self.assertTrue(pixbuf.savev(image_path, "png"))
            self.assertRaises(subprocess.CalledProcessError, subprocess.check_call, [BIN_PATH, image_path])
            self.assertFalse(self.plasmashell_interface.script_set_event.wait(1))


if __name__ == '__main__':
    assert os.path.exists(BIN_PATH)
    logging.getLogger().setLevel(logging.INFO)
    unittest.main()
