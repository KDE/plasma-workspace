# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

"""Notification utilities for tests."""

from typing import Any

import gi

gi.require_version('Gdk', '4.0')
from gi.repository import Gio, GLib

BUS_NAME: str = "org.freedesktop.Notifications"
OBJECT_PATH: str = "/org/freedesktop/Notifications"
IFACE_NAME: str = BUS_NAME


def send_notification(
    data: dict[str, str | int | list[str] | dict[str, GLib.Variant] | GLib.Variant],
    session_bus: Gio.DBusConnection | None = None
) -> int:
    """
    Send a notification via D-Bus.
    
    Args:
        data: Notification data dictionary with keys like app_name, summary, body, etc.
        session_bus: Optional D-Bus connection. If None, uses default session bus.
    
    Returns:
        Notification id
    """
    app_name: str = str(data.get("app_name", "Appium Test"))
    replaces_id: int = int(data.get("replaces_id", 0))
    app_icon: str = str(data.get("app_icon", "wayland"))
    summary: str = str(data.get("summary", ""))
    body: str = str(data.get("body", ""))
    actions: list[str] = data.get("actions", [])  # type: ignore
    hints: dict[str, GLib.Variant] = data.get("hints", {})  # type: ignore
    timeout: int = data.get("timeout", -1)  # type: ignore
    parameters = GLib.Variant("(susssasa{sv}i)", [app_name, replaces_id, app_icon, summary, body, actions, hints, timeout])

    if session_bus is None:
        session_bus = Gio.bus_get_sync(Gio.BusType.SESSION)

    reply = session_bus.call_sync(BUS_NAME, OBJECT_PATH, IFACE_NAME, "Notify", parameters, None, Gio.DBusSendMessageFlags.NONE, 5000)
    return reply.get_child_value(0).get_uint32()
