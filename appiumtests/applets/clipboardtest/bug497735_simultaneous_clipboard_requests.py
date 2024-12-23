#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import sys

from PySide6.QtCore import QCommandLineOption, QCommandLineParser, QMimeData
from PySide6.QtGui import QImage, QLinearGradient, QPainter
from PySide6.QtWidgets import QApplication, QMainWindow, QPushButton


class MainWindow(QMainWindow):

    def __init__(self, width: str, height: str):
        super().__init__()

        self.setWindowTitle("Gradient Generator")
        self.setGeometry(0, 0, 400, 300)

        self.button = QPushButton(text="Copy To Clipboard", parent=self)
        self.button.clicked.connect(self.generate_image)
        self.setCentralWidget(self.button)

        self.presets = list(QLinearGradient.Preset)
        self.count = -1
        self.width = int(width)
        self.height = int(height)

    def resizeEvent(self, event) -> None:
        print("window resized", file=sys.stderr)

    def generate_image(self):
        image = QImage(self.width, self.height, QImage.Format_ARGB32_Premultiplied)
        painter = QPainter(image)
        painter.fillRect(0, 0, self.width, self.height, self.random_gradient_preset())
        painter.end()

        clipboard = QApplication.clipboard()
        mime_data = QMimeData()
        mime_data.setImageData(image)
        clipboard.setMimeData(mime_data)
        print("copied")

    def random_gradient_preset(self):
        self.count += 1
        if self.count == len(self.presets):
            self.count = 0
        return self.presets[self.count]


if __name__ == "__main__":
    app = QApplication(sys.argv)

    parser = QCommandLineParser()
    width_option = QCommandLineOption(['w', "width"], "", 'Image width', "1000")
    parser.addOption(width_option)
    height_option = QCommandLineOption(['h', "height"], "", 'Image height', "1000")
    parser.addOption(height_option)
    parser.process(app)

    window = MainWindow(parser.value(width_option), parser.value(height_option))
    window.showFullScreen()
    sys.exit(app.exec())
