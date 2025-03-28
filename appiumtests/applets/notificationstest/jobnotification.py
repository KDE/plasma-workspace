#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import sys

from jobnotificationinterface import JobNotificationControlInterface
from KCoreAddons import KJob
from KJobWidgets import KUiServerV2JobTracker
from PySide6.QtCore import QCoreApplication, QMetaObject, Qt, QTimer
from PySide6.QtGui import QGuiApplication


class TestJob(KJob):

    def __init__(self) -> None:
        super().__init__()

        self.setProperty("immediateProgressReporting", True)
        self.setCapabilities(KJob.Capability.Killable | KJob.Capability.Suspendable)

    def start(self) -> None:
        # Delay reporting total bytes to test BUG 501991
        self.startElapsedTimer()
        QTimer.singleShot(500, self, self.init_progress)

    def doKill(self) -> bool:
        QMetaObject.invokeMethod(QCoreApplication.instance(), "quit", Qt.QueuedConnection)
        return True

    def doSuspend(self) -> bool:
        return True

    def doResume(self) -> bool:
        return True

    def init_progress(self) -> None:
        self.setProperty("destUrl", "file:///home/foobar/test.txt")
        self.setTotalAmount(KJob.Unit.Bytes, 1024 * 1024 * 10)
        self.setProcessedAmount(KJob.Unit.Bytes, 1024 * 1024 * 1)
        self.setTotalAmount(KJob.Unit.Files, 1)

    def update_progress(self, new_bytes: int) -> None:
        if new_bytes > self.processedAmount(KJob.Unit.Bytes):
            self.emitSpeed(new_bytes - self.processedAmount(KJob.Unit.Bytes))
        self.setProcessedAmount(KJob.Unit.Bytes, new_bytes)
        if self.processedAmount(KJob.Unit.Bytes) == self.totalAmount(KJob.Unit.Bytes):
            self.emitResult()


if __name__ == "__main__":
    app = QGuiApplication(sys.argv)
    app.setDesktopFileName("org.kde.plasmashell")

    tracker = KUiServerV2JobTracker(app)
    job = TestJob()
    job.finished.connect(app.quit)
    tracker.registerJob(job)
    job.start()

    control_interface = JobNotificationControlInterface(job)

    sys.exit(app.exec())
