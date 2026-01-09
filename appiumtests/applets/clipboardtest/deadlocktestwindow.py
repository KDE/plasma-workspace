#!/usr/bin/env python3
"""
SPDX-FileCopyrightText: 2025 David Edmundson <davidedmundson@kde.org>
SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
SPDX-License-Identifier: LGPL-2.0-or-later
"""

import sys

from KGuiAddons import KSystemClipboard
from PySide6.QtCore import QMimeData, Slot
from PySide6.QtGui import QClipboard
from PySide6.QtWidgets import (QApplication, QGroupBox, QHBoxLayout, QPushButton, QTextEdit, QVBoxLayout, QWidget)


class ClipboardTestWindow(QWidget):
    """
    This is a small GUI app which constantly syncs the normal and data control
    clipboards. This is useful for confirming all deadlocks are gone.

    @see https://invent.kde.org/frameworks/kguiaddons/-/merge_requests/191
    """

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Clipboard Test")
        self.__setup_ui()

    def __setup_ui(self) -> None:
        self.root_layout = QHBoxLayout(self)
        self.normal_clipboard_view = QTextEdit()
        self.normal_text_edit = QTextEdit()
        self.__setup_regular_clipboard(self.root_layout)

        self.system_clipboard_view = QTextEdit()
        self.system_text_edit = QTextEdit()
        self.__setup_system_clipboard(self.root_layout)

        self.resize(600, 500)

    def __setup_regular_clipboard(self, parent_layout) -> None:
        clipboard = QApplication.clipboard()

        group = QGroupBox("Normal Clipboard")
        parent_layout.addWidget(group)

        group_layout = QVBoxLayout(group)

        self.normal_clipboard_view.setAccessibleName("Normal Clipboard View")
        self.normal_clipboard_view.setReadOnly(True)
        self.normal_clipboard_view.setMinimumHeight(200)

        clipboard.changed.connect(self.on_normal_clipboard_changed)

        self.normal_clipboard_view.setText(clipboard.text())

        group_layout.addWidget(self.normal_clipboard_view)

        self.normal_text_edit.setAccessibleName("Normal Text Edit")
        self.normal_text_edit.setPlaceholderText("Enter clipboard content here…")
        self.normal_text_edit.setMinimumHeight(120)
        group_layout.addWidget(self.normal_text_edit)

        btn_row = QHBoxLayout()
        btn_row.addStretch()

        submit_btn = QPushButton("Submit1")
        submit_btn.clicked.connect(self.submit_normal_clipboard)
        btn_row.addWidget(submit_btn)

        group_layout.addLayout(btn_row)

    def __setup_system_clipboard(self, parent_layout) -> None:
        clipboard = KSystemClipboard.instance()

        group = QGroupBox("System Clipboard (QClipboard)")
        parent_layout.addWidget(group)

        group_layout = QVBoxLayout(group)

        self.system_clipboard_view.setAccessibleName("System Clipboard View")
        self.system_clipboard_view.setReadOnly(True)
        self.system_clipboard_view.setMinimumHeight(200)

        clipboard.changed.connect(self.on_system_clipboard_changed)

        self.system_clipboard_view.setText(clipboard.text(QClipboard.Mode.Clipboard))

        group_layout.addWidget(self.system_clipboard_view)

        self.system_text_edit.setAccessibleName("System Text Edit")
        self.system_text_edit.setPlaceholderText("Enter clipboard content here…")
        self.system_text_edit.setMinimumHeight(120)
        group_layout.addWidget(self.system_text_edit)

        btn_row = QHBoxLayout()
        btn_row.addStretch()

        submit_btn = QPushButton("Submit2")
        submit_btn.clicked.connect(self.submit_system_clipboard)
        btn_row.addWidget(submit_btn)

        group_layout.addLayout(btn_row)

    @Slot(QClipboard.Mode)
    def on_normal_clipboard_changed(self, mode) -> None:
        if mode != QClipboard.Mode.Clipboard:
            return
        clipboard = QApplication.clipboard()
        self.normal_clipboard_view.setText(clipboard.text(mode))

    @Slot(QClipboard.Mode)
    def on_system_clipboard_changed(self, mode) -> None:
        if mode != QClipboard.Mode.Clipboard:
            return
        clipboard = KSystemClipboard.instance()
        self.system_clipboard_view.setText(clipboard.text(mode))

    @Slot()
    def submit_normal_clipboard(self) -> None:
        clipboard = QApplication.clipboard()
        mime_data = QMimeData()
        mime_data.setText(self.normal_text_edit.toPlainText())
        clipboard.setMimeData(mime_data, QClipboard.Mode.Clipboard)

    @Slot()
    def submit_system_clipboard(self) -> None:
        clipboard = KSystemClipboard.instance()
        mime_data = QMimeData()
        mime_data.setText(self.system_text_edit.toPlainText())
        clipboard.setMimeData(mime_data, QClipboard.Mode.Clipboard)


if __name__ == "__main__":
    app = QApplication(sys.argv)

    window = ClipboardTestWindow()
    window.show()

    sys.exit(app.exec())
