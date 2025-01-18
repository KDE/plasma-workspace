#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import sys

import KStatusNotifierItem
from PySide6.QtCore import QObject, QPoint, Qt
from PySide6.QtGui import QColor, QIcon, QPixmap
from PySide6.QtWidgets import QApplication, QMenu


class StatusNotifierTest(QObject):

    def __init__(self, parent=None) -> None:
        super().__init__(parent)

        self.system_notifier = KStatusNotifierItem.KStatusNotifierItem("org.kde.statusnotifiertest", self)
        self.system_notifier.setCategory(KStatusNotifierItem.KStatusNotifierItem.ItemCategory.Communications)
        icon = self.create_icon(Qt.red)
        self.system_notifier.setIconByPixmap(icon)

        self.system_notifier.setStatus(KStatusNotifierItem.KStatusNotifierItem.ItemStatus.Active)
        self.system_notifier.setToolTipTitle("StatusNotifierTest")
        self.system_notifier.setTitle("StatusNotifierTest")
        self.system_notifier.setToolTipSubTitle("Some explanation from the beach.")

        menu = QMenu()
        icon = self.create_icon(QColor(0, 255, 0))  # green
        action = menu.addAction(icon, "NeedsAttention")
        action.triggered.connect(self.action_triggered)
        icon = self.create_icon(QColor(255, 85, 255))  # purple
        action = menu.addAction(icon, "Active")
        action.triggered.connect(self.action_triggered)

        sub_menu = QMenu("Sub Menu")
        sub_menu.setIcon(self.create_icon(QColor(85, 0, 255)))
        icon = self.create_icon(QColor(255, 255, 0))  # yellow
        sub_action = sub_menu.addAction(icon, "Passive")
        sub_action.triggered.connect(self.action_triggered)
        menu.addMenu(sub_menu)

        self.system_notifier.setContextMenu(menu)

        self.system_notifier.activateRequested.connect(self.activate_requested)
        self.system_notifier.secondaryActivateRequested.connect(self.secondary_activate_requested)
        self.system_notifier.scrollRequested.connect(self.scroll_requested)

    @staticmethod
    def create_icon(color) -> QIcon:
        pixmap = QPixmap(16, 16)
        pixmap.fill(color)
        return QIcon(pixmap)

    def action_triggered(self) -> None:
        text = self.sender().text()
        print(text, flush=True)

        if text == "NeedsAttention":
            self.system_notifier.setStatus(KStatusNotifierItem.KStatusNotifierItem.ItemStatus.NeedsAttention)
            self.system_notifier.setIconByPixmap(self.create_icon(Qt.blue))
        elif text == "Active":
            self.system_notifier.setStatus(KStatusNotifierItem.KStatusNotifierItem.ItemStatus.Active)
            self.system_notifier.setIconByPixmap(self.create_icon(Qt.red))
        elif text == "Passive":
            self.system_notifier.setStatus(KStatusNotifierItem.KStatusNotifierItem.ItemStatus.Passive)
        else:
            raise RuntimeError("Unexpected action triggered")

    def activate_requested(self, active: bool, pos: QPoint) -> None:
        print("Activated", flush=True)

    def secondary_activate_requested(self, pos: QPoint) -> None:
        print("SecondaryActivated", flush=True)

    def scroll_requested(self, delta: int, orientation: Qt.Orientation) -> None:
        direction = "Horizontally" if orientation == Qt.Horizontal else "Vertically"
        print(f"Scrolled by {delta} {direction}", flush=True)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    test = StatusNotifierTest()
    sys.exit(app.exec())
