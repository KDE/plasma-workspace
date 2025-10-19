#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import sys

from PySide6.QtCore import QCommandLineOption, QCommandLineParser, QMimeData, QTimer
from PySide6.QtGui import QColor, QImage, QPainter
from PySide6.QtWidgets import QApplication, QMainWindow, QPushButton


class MainWindow(QMainWindow):

    def __init__(self, width: str, height: str):
        super().__init__()

        self.setWindowTitle("Gradient Generator")
        self.setGeometry(0, 0, 400, 300)

        self.button = QPushButton(text="Copy To Clipboard", parent=self)
        self.button.clicked.connect(self.on_button_clicked)
        self.setCentralWidget(self.button)

        self.width = int(width)
        self.height = int(height)

        self.images = []
        for color in (QColor(255, 0, 0), ):
            image = QImage(self.width, self.height, QImage.Format_ARGB32_Premultiplied)
            painter = QPainter(image)
            painter.fillRect(0, 0, self.width, self.height, color)
            painter.end()
            self.images.append(image)
        self.pressed_count = 0

    def resizeEvent(self, event) -> None:
        print("window resized", file=sys.stderr)

    def on_button_clicked(self) -> None:
        self.generate_image()
        self.pressed_count += 1
        if self.pressed_count == len(self.images):
            self.hide()
            QTimer.singleShot(1000, self, QApplication.instance().quit)

    def generate_image(self):
        mime_data = QMimeData()
        mime_data.setImageData(self.images[self.pressed_count])
        QApplication.clipboard().setMimeData(mime_data)
        print("copied", file=sys.stderr)


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
