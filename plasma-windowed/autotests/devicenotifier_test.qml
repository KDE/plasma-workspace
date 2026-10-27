/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces the open check of appiumtests/applets/devicenotifiertest.py:
        self.driver.find_element(AppiumBy.NAME, "No removable devices attached")

    Only the empty state is portable: the device listing tests drive a mocked
    udisks2 on a private system bus, which this harness does not provide. This
    also means the empty state is not guaranteed here: on a machine with a
    mounted removable device the applet lists it instead and this test fails.
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
