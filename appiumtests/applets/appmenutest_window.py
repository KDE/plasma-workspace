#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

from PySide6.QtWidgets import QApplication, QMainWindow, QMenu
from PySide6.QtCore import QTimer


class TestWindow(QMainWindow):

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setWindowTitle("Test Window")

        menu_bar = self.menuBar()
        foo_menu = QMenu("&foo", self)
        bar_menu = QMenu("&bar", self)
        menu_bar.addMenu(foo_menu)
        menu_bar.addMenu(bar_menu)
        menu_bar.setVisible(False)

        QTimer.singleShot(60000, self.close)


if __name__ == "__main__":
    app = QApplication([])
    app.setDesktopFileName("org.kde.testwindow")
    win = TestWindow()
    win.show()
    app.exec()
