#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import gi

gi.require_version('Gtk', '4.0')
from gi.repository import GLib, Gtk


class TestWindow(Gtk.ApplicationWindow):

    def __init__(self, _app: Gtk.Application) -> None:
        super().__init__(application=_app, title="Test Window")

        builder = Gtk.Builder.new_from_string("""
<interface>
    <menu id='menubar'>
        <submenu>
            <attribute name='label'>_foo</attribute>
        </submenu>
        <submenu>
            <attribute name='label'>_bar</attribute>
        </submenu>
    </menu>
</interface>
        """, -1)
        menubar = builder.get_object("menubar")
        _app.set_menubar(menubar)
        self.set_show_menubar(False)
        GLib.timeout_add_seconds(60, self.close)


def on_activate(_app: Gtk.Application) -> None:
    win = TestWindow(_app)
    win.present()


if __name__ == "__main__":
    app = Gtk.Application(application_id='org.kde.testwindow')
    app.connect('activate', on_activate)
    app.run(None)
