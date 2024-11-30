#!/usr/bin/env python3

# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022-2023 Harald Sitter <sitter@kde.org>

import logging
import os
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from typing import Final

from gi.repository import Gio, GLib

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "utils"))
from GLibMainLoopThread import GLibMainLoopThread

KDE_VERSION: Final = 6
KACTIVITYMANAGERD_SERVICE_NAME: Final = "org.kde.ActivityManager"
KACTIVITYMANAGERD_PATH: Final = os.getenv("KACTIVITYMANAGERD_PATH", "/usr/libexec/kactivitymanagerd")
QMLTEST_EXEC: Final = os.getenv("QMLTEST_EXEC", "/usr/bin/qmltestrunner6")


def name_has_owner(session_bus: Gio.DBusConnection, name: str) -> bool:
    """
    Whether the given name is available on session bus
    """
    message: Gio.DBusMessage = Gio.DBusMessage.new_method_call("org.freedesktop.DBus", "/", "org.freedesktop.DBus", "NameHasOwner")
    message.set_body(GLib.Variant("(s)", [name]))
    reply, _ = session_bus.send_message_with_reply_sync(message, Gio.DBusSendMessageFlags.NONE, 1000)
    return reply and reply.get_signature() == 'b' and reply.get_body().get_child_value(0).get_boolean()


def build_ksycoca() -> None:
    subprocess.check_call([f"kbuildsycoca{KDE_VERSION}"], stdout=sys.stderr, stderr=sys.stderr, env=os.environ)


def start_kactivitymanagerd() -> subprocess.Popen:
    session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
    assert not name_has_owner(session_bus, KACTIVITYMANAGERD_SERVICE_NAME)

    os.makedirs(os.path.join(GLib.get_user_config_dir(), "menus"))
    shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "applications.menu"), os.path.join(GLib.get_user_config_dir(), "menus"))

    kactivitymanagerd = subprocess.Popen([KACTIVITYMANAGERD_PATH], stdout=sys.stderr, stderr=sys.stderr, env=os.environ)
    kactivitymanagerd_started: bool = False
    for _ in range(10):
        if name_has_owner(session_bus, KACTIVITYMANAGERD_SERVICE_NAME):
            kactivitymanagerd_started = True
            break
        logging.info("waiting for kactivitymanagerd to appear on the DBus session")
        time.sleep(1)
    assert kactivitymanagerd_started

    build_ksycoca()

    return kactivitymanagerd


class TestDBusInterface:
    """
    D-Bus interface for org.kde.kickertest
    """

    BUS_NAME: Final = "org.kde.kickertest"
    OBJECT_PATH: Final = "/test"
    INTERFACE_NAME: Final = "org.kde.kickertest"

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
  <interface name="org.kde.kickertest">
    <method name="DeleteAndRebuildDatabase1"/>
    <method name="DeleteAndRebuildDatabase2"/>
  </interface>
</node>
""")
        self.reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.handle_method_call, None, None)
        assert self.reg_id > 0
        logging.info("interface registered")

    def handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        logging.info("method call %s", method_name)

        if method_name == "DeleteAndRebuildDatabase1":
            os.remove(KickerTest.desktop_entry_1)
            build_ksycoca()
            invocation.return_value(None)
        elif method_name == "DeleteAndRebuildDatabase2":
            os.remove(KickerTest.desktop_entry_2)
            build_ksycoca()
            invocation.return_value(None)


class KickerTest(unittest.TestCase):
    kactivitymanagerd: subprocess.Popen
    loop_thread: GLibMainLoopThread
    dbus_interface: TestDBusInterface

    temp_dir: tempfile.TemporaryDirectory
    desktop_entry_1: str
    desktop_entry_2: str

    @classmethod
    def setUpClass(cls) -> None:
        # Prepare desktop files
        # 1
        os.makedirs(os.path.join(GLib.get_user_data_dir(), "applications"))
        shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "kickertest.desktop"), os.path.join(GLib.get_user_data_dir(), "applications"))
        cls.desktop_entry_1 = os.path.join(GLib.get_user_data_dir(), "applications", "kickertest.desktop")
        # 2
        cls.temp_dir = tempfile.TemporaryDirectory()
        os.makedirs(os.path.join(cls.temp_dir.name, "applications"))
        shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "kickertest.desktop"), os.path.join(cls.temp_dir.name, "applications"))
        cls.desktop_entry_2 = os.path.join(cls.temp_dir.name, "applications", "kickertest.desktop")

        os.environ["LC_ALL"] = "en_US.UTF-8"
        os.environ["QT_LOGGING_RULES"] = "org.kde.plasma.kicker.debug=true;kf.coreaddons.kdirwatch.debug=true"
        os.environ["XDG_DATA_DIRS"] = os.environ["XDG_DATA_DIRS"] + ":" + cls.temp_dir.name

        cls.kactivitymanagerd = start_kactivitymanagerd()

        cls.loop_thread = GLibMainLoopThread()
        cls.loop_thread.start()
        cls.dbus_interface = TestDBusInterface()

    @classmethod
    def tearDownClass(cls) -> None:
        cls.loop_thread.quit()
        cls.kactivitymanagerd.kill()
        cls.kactivitymanagerd.wait(10)

    def test_qml(self) -> None:
        """
        1. Add an entry to Favorites
        2. Remove the entry from Favorites
        3. Hide invalid entries automatically and don't crash when there are multiple entries with the same desktop name
        """
        with subprocess.Popen([QMLTEST_EXEC, "-input", os.path.join(os.path.dirname(os.path.abspath(__file__)), 'favoritetest.qml')], stdout=sys.stderr, stderr=sys.stderr) as process:
            self.assertEqual(process.wait(60), 0)


if __name__ == '__main__':
    assert "USE_CUSTOM_BUS" in os.environ
    logging.getLogger().setLevel(logging.INFO)
    unittest.main()
