/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces the open check of appiumtests/applets/cameraindicatortest.py, whose
    assertion depends on whether pipewire was already running:
        if pipewire was running: AppiumBy.NAME, "No camera is in use"
        otherwise:               AppiumBy.NAME, "Camera indicator is unavailable"

    Which of the two applies is an environment detail (this harness does not
    start pipewire), so both are accepted. The tests that stream from mock
    cameras need pipewire + pipewiremockcamera and stay in the appium suite.
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

PlasmoidTestItem {
    id: plasmoid
    appletId: "org.kde.plasma.cameraindicator"
    width: 512
    height: 512

    TestCase {
        when: windowShown

        function init() {
            // See PlasmoidTestItem::isReady() for why this wait exists.
            tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
        }

        function test_0_open() {
            const unavailable = plasmoid.findItem(By.NAME, "Camera indicator is unavailable")
            const inUse = plasmoid.findItem(By.NAME, "No camera is in use")
            if (!unavailable && !inUse) {
                console.warn("dumpTree:\n" + plasmoid.dumpTree())
            }
            verify(unavailable !== null || inUse !== null, "camera indicator reports no camera status")
        }
    }
}
