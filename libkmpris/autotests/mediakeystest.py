#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# pylint: disable=line-too-long

# For FreeBSD CI which only has python 3.9
from __future__ import annotations

import json
import os
import subprocess
import sys
import time
import unittest
from typing import Final

assert "appiumtests" in os.getcwd(), "Make sure the current directory is appiumtests"
sys.path.append(os.getcwd())  # for appiumtests.utils

from gi.repository import Gio, GLib
from utils.GLibMainLoopThread import GLibMainLoopThread
from utils.mediaplayer import (Mpris2, read_base_properties, read_player_metadata, read_player_properties)

KDE_VERSION: Final = 6
SERVICE_NAME: Final = "org.freedesktop.DBus"
ROOT_OBJECT_PATH: Final = "/"
INTERFACE_NAME: Final = SERVICE_NAME
KGLOBALACCELD_PATH: str = ""


def name_has_owner(session_bus: Gio.DBusConnection, name: str) -> bool:
    """
    Whether the given name is available on session bus
    """
    message: Gio.DBusMessage = Gio.DBusMessage.new_method_call(SERVICE_NAME, ROOT_OBJECT_PATH, INTERFACE_NAME, "NameHasOwner")
    message.set_body(GLib.Variant("(s)", [name]))
    reply, _ = session_bus.send_message_with_reply_sync(message, Gio.DBusSendMessageFlags.NONE, 1000)
    return reply and reply.get_signature() == 'b' and reply.get_body().get_child_value(0).get_boolean()


class MediaKeysTest(unittest.TestCase):
    """
    Tests for the media global shortcuts

    @see https://bugs.kde.org/show_bug.cgi?id=474531
    """

    loop_thread: GLibMainLoopThread
    mpris_interface: Mpris2
    kded: subprocess.Popen | None = None
    kglobalacceld: subprocess.Popen | None = None

    @classmethod
    def setUpClass(cls) -> None:
        cls.loop_thread = GLibMainLoopThread()
        cls.loop_thread.start()

        # Doc: https://lazka.github.io/pgi-docs/Gio-2.0/classes/DBusConnection.html
        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)

        # Start KGlobalAccel daemon service
        if not name_has_owner(session_bus, "org.kde.kglobalaccel"):
            cls.kglobalacceld = subprocess.Popen([KGLOBALACCELD_PATH])

        if not name_has_owner(session_bus, f"org.kde.kded{KDE_VERSION}"):
            cls.kded = subprocess.Popen([f"kded{KDE_VERSION}"])
            kded_started: bool = False
            for _ in range(10):
                if name_has_owner(session_bus, f"org.kde.kded{KDE_VERSION}"):
                    kded_started = True
                    break
                print(f"waiting for kded{KDE_VERSION} to appear on the dbus session")
                time.sleep(1)
            cls.assertTrue(kded_started, "kded is not started")

        cls.assertTrue(name_has_owner(session_bus, "org.kde.kglobalaccel"), "kglobalacceld is not started")

        kded_reply: GLib.Variant = session_bus.call_sync(f"org.kde.kded{KDE_VERSION}", "/kded", f"org.kde.kded{KDE_VERSION}", "loadModule",
                                                         GLib.Variant("(s)", ["mprisservice"]), GLib.VariantType("(b)"), Gio.DBusSendMessageFlags.NONE, 1000)
        cls.assertTrue(kded_reply.get_child_value(0).get_boolean(), "mprisservice module is not loaded")

        json_path: str = os.path.join(os.getcwd(), "resources/player_a.json")
        with open(json_path, "r", encoding="utf-8") as f:
            json_dict: dict[str, list | dict] = json.load(f)
        metadata: list[dict[str, GLib.Variant]] = read_player_metadata(json_dict)
        base_properties: dict[str, GLib.Variant] = read_base_properties(json_dict)
        current_index: int = 1
        player_properties: dict[str, GLib.Variant] = read_player_properties(json_dict, metadata[current_index])
        cls.mpris_interface = Mpris2(metadata, base_properties, player_properties, current_index)
        cls.mpris_interface.registered_event.wait(timeout=10)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.mpris_interface.quit()
        cls.loop_thread.quit()
        if cls.kded:
            cls.kded.terminate()
        if cls.kglobalacceld:
            cls.kglobalacceld.terminate()

    def setUp(self) -> None:
        pass

    def tearDown(self) -> None:
        pass

    def test_1_playpause(self) -> None:
        """
        Global shortcut for "Play/Pause media playback"
        """
        self.assertEqual(self.mpris_interface.player_properties["PlaybackStatus"].get_string(), "Stopped")
        self.assertEqual(subprocess.Popen(["xdotool", "key", "XF86AudioPlay"]).wait(), 0)
        self.assertTrue(self.mpris_interface.playback_status_set_event.wait(timeout=10))
        self.mpris_interface.playback_status_set_event.clear()
        self.assertEqual(self.mpris_interface.player_properties["PlaybackStatus"].get_string(), "Playing")
        self.assertEqual(subprocess.Popen(["xdotool", "key", "XF86AudioPlay"]).wait(), 0)
        self.assertTrue(self.mpris_interface.playback_status_set_event.wait(timeout=10))
        self.mpris_interface.playback_status_set_event.clear()
        self.assertEqual(self.mpris_interface.player_properties["PlaybackStatus"].get_string(), "Paused")

    def test_2_next(self) -> None:
        """
        Global shortcut for "Media playback next"
        """
        self.assertEqual(subprocess.Popen(["xdotool", "key", "XF86AudioNext"]).wait(), 0)
        self.assertTrue(self.mpris_interface.metadata_updated_event.wait(10))
        self.mpris_interface.metadata_updated_event.clear()
        self.assertEqual(self.mpris_interface.player_properties["Metadata"]["xesam:title"], "Konqi's Favorite")

        # Press again to test canNext
        self.assertEqual(subprocess.Popen(["xdotool", "key", "XF86AudioNext"]).wait(), 0)
        time.sleep(1)
        self.assertEqual(self.mpris_interface.player_properties["Metadata"]["xesam:title"], "Konqi's Favorite")

    def test_3_previous(self) -> None:
        """
        Global shortcut for "Media playback previous"
        """
        self.assertEqual(subprocess.Popen(["xdotool", "key", "XF86AudioPrev"]).wait(), 0)
        self.assertTrue(self.mpris_interface.metadata_updated_event.wait(10))
        self.mpris_interface.metadata_updated_event.clear()
        self.assertEqual(self.mpris_interface.player_properties["Metadata"]["xesam:title"], "Konqi ❤️️ Katie")

        self.assertEqual(subprocess.Popen(["xdotool", "key", "XF86AudioPrev"]).wait(), 0)
        self.assertTrue(self.mpris_interface.metadata_updated_event.wait(10))
        self.mpris_interface.metadata_updated_event.clear()
        self.assertEqual(self.mpris_interface.player_properties["Metadata"]["xesam:title"], "Katie's Favorite")

        # Press again to test canPrevious
        self.assertEqual(subprocess.Popen(["xdotool", "key", "XF86AudioPrev"]).wait(), 0)
        time.sleep(1)
        self.assertEqual(self.mpris_interface.player_properties["Metadata"]["xesam:title"], "Katie's Favorite")

    def test_4_unload_mprisservice(self) -> None:
        """
        Unload mprisservice to make sure the default keyboard shortcuts don't take effect
        """
        if self.kded is None and "KDECI_BUILD" not in os.environ:
            self.skipTest(f"kded{KDE_VERSION} is not run by this test")

        session_bus: Gio.DBusConnection = Gio.bus_get_sync(Gio.BusType.SESSION)
        kded_reply: GLib.Variant = session_bus.call_sync(f"org.kde.kded{KDE_VERSION}", "/kded", f"org.kde.kded{KDE_VERSION}", "unloadModule",
                                                         GLib.Variant("(s)", ["mprisservice"]), GLib.VariantType("(b)"), Gio.DBusSendMessageFlags.NONE, 1000)
        self.assertTrue(kded_reply.get_child_value(0).get_boolean(), "mprisservice module is not loaded")

        last_playback_status: str = self.mpris_interface.player_properties["PlaybackStatus"].get_string()
        self.assertEqual(subprocess.Popen(["xdotool", "key", "XF86AudioPlay"]).wait(), 0)
        time.sleep(1)
        self.assertEqual(self.mpris_interface.player_properties["PlaybackStatus"].get_string(), last_playback_status)

        last_xesam_title: str = self.mpris_interface.player_properties["Metadata"]["xesam:title"]
        self.assertEqual(subprocess.Popen(["xdotool", "key", "XF86AudioNext"]).wait(), 0)
        time.sleep(1)
        self.assertEqual(self.mpris_interface.player_properties["Metadata"]["xesam:title"], last_xesam_title)


if __name__ == '__main__':
    assert len(sys.argv) >= 2, "kglobalacceld is not provided"
    KGLOBALACCELD_PATH = sys.argv.pop()
    unittest.main()
