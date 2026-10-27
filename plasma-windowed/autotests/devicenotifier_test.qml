/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces the open check of appiumtests/applets/devicenotifiertest.py:
        self.driver.find_element(AppiumBy.NAME, "No removable devices attached")

    Only the empty state is portable: the device listing tests add mock devices
    to a udisks2 mock (python3-dbusmock), which is out of scope here. Instead
    this test runs on a private system bus with no udisks2 at all, started by
    run_qmltest_with_bus.py, so the empty state holds no matter what is plugged
    into the host.
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

PlasmoidTestItem {
    id: plasmoid
    appletId: "org.kde.plasma.devicenotifier"
    width: 512
    height: 512

    TestCase {
        when: windowShown

        function init() {
            // See PlasmoidTestItem::isReady() for why this wait exists.
            tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
        }

        function test_0_open() {
            // The placeholder lives behind an asynchronous Loader
            // (applets/devicenotifier/qml/FullRepresentation.qml), so it can
            // appear a few event loop iterations after the applet is ready.
            // The appium original waited for it with an implicit 10 s timeout.
            tryVerify(() => plasmoid.findItem(By.NAME, "No removable devices attached") !== null, 10000,
                      "the \"no removable devices\" placeholder did not appear")
        }
    }
}
