#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# For FreeBSD CI which only has python 3.9
from __future__ import annotations

import threading
import time

from gi.repository import GLib


class GLibMainLoopThread(threading.Thread):
    """
    A GLib main loop in another thread.
    @note It's redundant to create a loop when QCoreApplication is used in the same application.
    """

    loop: GLib.MainLoop | None = None

    def __init__(self) -> None:
        # Set up D-Bus loop
        self.loop = GLib.MainLoop()
        self.failSafeTimer = threading.Timer(60, self.loop.quit)

        # Create the thread
        super(GLibMainLoopThread, self).__init__()

    def run(self) -> None:
        """
        Method to run the DBus main loop (on a thread)
        """
        self.failSafeTimer.start()
        self.loop.run()

    def quit(self) -> None:
        self.failSafeTimer.cancel()
        self.loop.quit()

    @classmethod
    def process_events(cls) -> None:
        """
        Processes some pending events in the main loop
        """
        if cls.loop is not None:
            context = cls.loop.get_context()
        else:
            context = GLib.MainContext.default()

        for _ in range(4):
            if not context.pending():
                time.sleep(0.1)
                continue
            context.iteration(may_block=False)
            break
