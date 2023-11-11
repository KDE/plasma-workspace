#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

import os
import subprocess
import sys
import threading
import unittest
from typing import Any, Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from gi.repository import Gio, GLib
from utils.GLibMainLoopThread import GLibMainLoopThread

KDE_INSTALL_FULL_LIBEXECDIR: Final = os.environ.get("KDE_INSTALL_FULL_LIBEXECDIR", "/usr/libexec")


def default_value_for_signature(signature: str) -> Any:
    if signature == "b":
        return True
    elif signature == "s":
        return str()
    elif signature.startswith("a"):
        return []
    elif signature == "u" or signature == "t":
        return 0
    elif signature == "(st)":
        return ["", 0]
    return None


class OrgFreeDesktopLogin1Manager:
    """
    D-Bus interfaces for org.freedesktop.login1.Manager
    """

    BUS_NAME: Final = "org.freedesktop.login1"
    OBJECT_PATH: Final = "/org/freedesktop/login1"
    PROP_IFACE_NAME: Final = "org.freedesktop.DBus.Properties"
    MANAGER_IFACE_NAME: Final = "org.freedesktop.login1.Manager"

    __connection: Gio.DBusConnection

    def __init__(self) -> None:
        self.__prop_reg_id: int = 0
        self.__manager_reg_id: int = 0
        self.manager_properties: dict[str, GLib.Variant] = {}
        self.registered_event = threading.Event()
        self.poweroff_event = threading.Event()
        self.reboot_event = threading.Event()
        self.suspend_event = threading.Event()
        self.hybrid_suspend_event = threading.Event()
        self.hibernate_event = threading.Event()
        self.suspend_then_hibernate_event = threading.Event()

        self.__owner_id: int = Gio.bus_own_name(Gio.BusType.SYSTEM, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.__owner_id > 0

    def quit(self) -> None:
        Gio.bus_unown_name(self.__owner_id)
        self.__connection.unregister_object(self.__prop_reg_id)
        self.__connection.unregister_object(self.__manager_reg_id)
        self.__prop_reg_id = 0
        self.__manager_reg_id = 0
        GLibMainLoopThread.process_events()  # Otherwise flaky

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        """
        Interface is ready, now register objects.
        """
        self.__connection = connection

        with open("/usr/share/dbus-1/interfaces/org.freedesktop.login1.Manager.xml", encoding="utf-8") as file_handler:
            introspection_data = Gio.DBusNodeInfo.new_for_xml("\n".join(file_handler.readlines()))
            for interface_info in introspection_data.interfaces:
                if interface_info.name == self.PROP_IFACE_NAME:
                    self.__prop_reg_id = connection.register_object(self.OBJECT_PATH, interface_info, self.handle_prop_method_call, None, None)
                elif interface_info.name == self.MANAGER_IFACE_NAME:
                    assert len(interface_info.properties) > 0
                    for prop in interface_info.properties:
                        self.manager_properties[prop.name] = GLib.Variant(prop.signature, default_value_for_signature(prop.signature))
                    self.__manager_reg_id = connection.register_object(self.OBJECT_PATH, interface_info, self.handle_method_call, self.handle_get_property, self.handle_set_property)
            assert self.__prop_reg_id > 0
            assert self.__manager_reg_id > 0

        self.registered_event.set()

    def handle_prop_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        print(f"prop call {method_name}", file=sys.stderr, flush=True)

        if method_name == "Get":
            assert parameters[0] == self.MANAGER_IFACE_NAME
            property_name: str = parameters[1]
            invocation.return_value(GLib.Variant.new_tuple(self.manager_properties[property_name]))

        elif method_name == "GetAll":
            assert parameters[0] == self.MANAGER_IFACE_NAME
            invocation.return_value(GLib.Variant.new_tuple(GLib.Variant('a{sv}', self.manager_properties)))

        elif method_name == "Set":
            assert parameters[0] == self.MANAGER_IFACE_NAME
            property_name = parameters[1]
            assert property_name in self.manager_properties
            value: Any = parameters[2]
            self.manager_properties[property_name] = GLib.Variant(self.manager_properties[property_name].get_type_string(), value)

        else:
            assert False

    def handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        print(f"login1 call {method_name}", file=sys.stderr, flush=True)

        if method_name.startswith("Can"):
            invocation.return_value(GLib.Variant.new_tuple(GLib.Variant('s', "yes")))
            return

        if method_name == "PowerOff":
            self.poweroff_event.set()
        elif method_name == "Reboot":
            self.reboot_event.set()
        elif method_name == "Suspend":
            self.suspend_event.set()
        elif method_name == "HybridSleep":
            self.hybrid_suspend_event.set()
        elif method_name == "Hibernate":
            self.hibernate_event.set()
        elif method_name == "SuspendThenHibernate":
            self.suspend_then_hibernate_event.set()

        invocation.return_value(None)

    def handle_get_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, value: str):
        print(f"login1 get_property {value}")
        assert value in self.manager_properties
        return self.manager_properties[value]

    def handle_set_property(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, key: str, value: Any) -> bool:
        print(f"login1 set_property {value}")
        assert key in self.manager_properties
        self.manager_properties[key] = GLib.Variant(self.manager_properties[key].get_type_string(), value)
        return True


class LogoutGreeterTests(unittest.TestCase):

    dbus_daemon_pid: str
    proc: subprocess.Popen
    loop_thread: GLibMainLoopThread
    dbus_interface: OrgFreeDesktopLogin1Manager

    @classmethod
    def setUpClass(cls) -> None:
        cls.addClassCleanup(lambda: subprocess.check_call(["kill", "-9", cls.dbus_daemon_pid]))
        lines: list[str] = subprocess.check_output(['dbus-daemon', '--fork', '--print-address=1', '--print-pid=1', '--session'], universal_newlines=True).strip().splitlines()
        assert len(lines) == 2, "Expected exactly 2 lines of output from dbus-daemon"
        cls.dbus_daemon_pid = lines[1]
        assert int(cls.dbus_daemon_pid) > 0, "Failed to start dbus-daemon"
        os.environ["DBUS_SYSTEM_BUS_ADDRESS"] = lines[0]
        os.environ["DBUS_SESSION_BUS_ADDRESS"] = lines[0]

        cls.loop_thread = GLibMainLoopThread()
        cls.loop_thread.start()
        cls.dbus_interface = OrgFreeDesktopLogin1Manager()
        assert cls.dbus_interface.registered_event.wait(10), "login1 interface is not ready"

    def setUp(self) -> None:
        options = AppiumOptions()
        options.set_capability("app", f"{KDE_INSTALL_FULL_LIBEXECDIR}/ksmserver-logout-greeter --windowed")
        options.set_capability("environ", {
            "DBUS_SYSTEM_BUS_ADDRESS": os.environ["DBUS_SYSTEM_BUS_ADDRESS"],
            "DBUS_SESSION_BUS_ADDRESS": os.environ["DBUS_SESSION_BUS_ADDRESS"],
        })
        options.set_capability("timeouts", {'implicit': 10000})
        self.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def tearDown(self) -> None:
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_logoutgreetertest_{self.id()}.png")
        self.driver.quit()

    @classmethod
    def tearDownClass(cls) -> None:
        cls.dbus_interface.quit()
        cls.loop_thread.quit()

    def test_sleep(self) -> None:
        self.assertFalse(self.dbus_interface.suspend_event.is_set())
        self.driver.find_element(by=AppiumBy.NAME, value="Sleep").click()
        self.assertTrue(self.dbus_interface.suspend_event.wait(10))

    def test_hibernate(self) -> None:
        self.assertFalse(self.dbus_interface.hibernate_event.is_set())
        self.driver.find_element(by=AppiumBy.NAME, value="Hibernate").click()
        self.assertTrue(self.dbus_interface.hibernate_event.wait(10))

    def test_restart(self) -> None:
        self.driver.find_element(by=AppiumBy.NAME, value="Restart")

    def test_poweroff(self) -> None:
        self.driver.find_element(by=AppiumBy.NAME, value="Shut Down")


if __name__ == '__main__':
    unittest.main()
