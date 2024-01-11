#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os

import gi

gi.require_version('GdkPixbuf', '2.0')
gi.require_version('Gtk', '3.0')
from gi.repository import GdkPixbuf, GLib, Gtk

win = None
current_index = 0
icon_1 = GdkPixbuf.Pixbuf.new_from_file(os.path.join(os.path.dirname(os.path.abspath(__file__)), "none.png"))
icon_2 = GdkPixbuf.Pixbuf.new_from_file(os.path.join(os.path.dirname(os.path.abspath(__file__)), "samplewidgetwindow.png"))


def change_icon():
    global current_index
    if current_index == 0:
        win.set_icon(icon_1)
        current_index = 1
    else:
        win.set_icon(icon_2)
        current_index = 0

    return True


if __name__ == "__main__":
    win = Gtk.Window(title="flash")
    win.show_all()
    GLib.timeout_add(100, change_icon)
    GLib.timeout_add_seconds(60, Gtk.main_quit)
    Gtk.main()
