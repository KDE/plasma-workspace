#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import sys
from typing import Final

from gi.repository import Gio, GLib
from KCoreAddons import KJob
from KJobWidgets import KUiServerV2JobTracker
from PySide6.QtCore import QCoreApplication, QMetaObject, Qt, QTimer
from PySide6.QtGui import QGuiApplication

global_job = None


class TestInterface:

    BUS_NAME: Final = "org.kde.jobnotificationtest"
    OBJECT_PATH: Final = "/jobnotificationtest"

    connection: Gio.DBusConnection
    pid: int

    def __init__(self) -> None:
        self.reg_id: int = 0
        self.owner_id: int = Gio.bus_own_name(Gio.BusType.SESSION, self.BUS_NAME, Gio.BusNameOwnerFlags.NONE, self.on_bus_acquired, None, None)
        assert self.owner_id > 0

    def quit(self) -> None:
        self.connection.unregister_object(self.reg_id)
        Gio.bus_unown_name(self.owner_id)
        self.connection.flush_sync(None)

    def on_bus_acquired(self, connection: Gio.DBusConnection, name: str, *args) -> None:
        self.connection = connection

        introspection_data = Gio.DBusNodeInfo.new_for_xml("""
<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
    "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.kde.jobnotificationtest">
    <method name="SetProcessedAmount">
      <arg name="ProcessedAmount" type="t" direction="in"/>
    </method>
  </interface>
</node>
""")
        self.reg_id = connection.register_object(self.OBJECT_PATH, introspection_data.interfaces[0], self.handle_method_call, None, None)
        assert self.reg_id > 0

    def handle_method_call(self, connection: Gio.DBusConnection, sender: str, object_path: str, interface_name: str, method_name: str, parameters: GLib.Variant, invocation: Gio.DBusMethodInvocation) -> None:
        assert isinstance(global_job, TestJob)
        if method_name == "SetProcessedAmount":
            global_job.update_progress(parameters[0])
        else:
            invocation.return_error_literal(Gio.dbus_error_quark(), Gio.DBusError.UNKNOWN_METHOD, f"Unknown method {method_name}")
            return

        invocation.return_value(None)


class TestJob(KJob):

    def __init__(self) -> None:
        super().__init__()

        self.setProperty("immediateProgressReporting", True)
        self.setCapabilities(KJob.Capability.Killable | KJob.Capability.Suspendable)

    def start(self) -> None:
        # Delay reporting total bytes to test BUG 501991
        self.startElapsedTimer()
        QTimer.singleShot(500, self, self.init_progress)

    def doKill(self) -> bool:
        QMetaObject.invokeMethod(QCoreApplication.instance(), "quit", Qt.QueuedConnection)
        return True

    def doSuspend(self) -> bool:
        return True

    def doResume(self) -> bool:
        return True

    def init_progress(self) -> None:
        self.setProperty("destUrl", "file:///home/foobar/test.txt")
        self.setTotalAmount(KJob.Unit.Bytes, 1024 * 1024 * 10)
        self.setProcessedAmount(KJob.Unit.Bytes, 1024 * 1024 * 1)
        self.setTotalAmount(KJob.Unit.Files, 1)

    def update_progress(self, new_bytes: int) -> None:
        if new_bytes > self.processedAmount(KJob.Unit.Bytes):
            self.emitSpeed(new_bytes - self.processedAmount(KJob.Unit.Bytes))
        self.setProcessedAmount(KJob.Unit.Bytes, new_bytes)
        if self.processedAmount(KJob.Unit.Bytes) == self.totalAmount(KJob.Unit.Bytes):
            self.emitResult()


if __name__ == "__main__":
    app = QGuiApplication(sys.argv)
    app.setDesktopFileName("org.kde.plasmashell")

    tracker = KUiServerV2JobTracker(app)
    global_job = TestJob()
    global_job.finished.connect(app.quit)
    tracker.registerJob(global_job)
    global_job.start()

    control_interface = TestInterface()

    sys.exit(app.exec())
