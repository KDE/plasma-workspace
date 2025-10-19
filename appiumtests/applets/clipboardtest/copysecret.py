#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import sys

from PySide6.QtCore import QMimeData
from PySide6.QtWidgets import QApplication, QMainWindow, QPushButton


class ClipboardHelperWindow(QMainWindow):
    """
    A window to copy text with the secret hint
    """

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("ClipboardHelper")
        self.setGeometry(0, 0, 100, 100)

        self.button = QPushButton("Copy Secret", self)
        self.button.clicked.connect(self.copy_secret)
        self.setCentralWidget(self.button)

    def resizeEvent(self, event) -> None:
        print("button resized", file=sys.stderr)

    def copy_secret(self) -> None:
        mimeData = QMimeData()
        mimeData.setText("123456789test")
        mimeData.setData("x-kde-passwordManagerHint", b"secret")
        QApplication.clipboard().setMimeData(mimeData)

        mimeData = QMimeData()
        mimeData.setText("123456789")
        mimeData.setData("x-kde-passwordManagerHint", b"public")
        QApplication.clipboard().setMimeData(mimeData)
        self.hide()  # Move focus to plasmawindowed


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = ClipboardHelperWindow()
    window.showFullScreen()
    sys.exit(app.exec())
