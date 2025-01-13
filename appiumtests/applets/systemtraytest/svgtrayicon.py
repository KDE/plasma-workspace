#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import sys

from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QApplication, QSystemTrayIcon


class TestApplication(QApplication):

    def __init__(self, argv) -> None:
        super().__init__(argv)

        self.setApplicationName("SvgTrayIconTest")
        self.tray_icon = QSystemTrayIcon(QIcon(os.path.join(os.path.join(os.path.dirname(os.path.abspath(__file__))), "svgtrayicon.svg")), self)
        self.tray_icon.activated.connect(self.on_tray_icon_activated)
        self.tray_icon.show()

    def on_tray_icon_activated(self, reason) -> None:
        if reason is QSystemTrayIcon.Trigger:
            self.quit()


if __name__ == "__main__":
    app = TestApplication(sys.argv)
    sys.exit(app.exec())
