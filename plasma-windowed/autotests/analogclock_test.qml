/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces appiumtests/applets/analogclocktest.py:
        self.driver.find_element(AppiumBy.NAME, "Analog Clock")

    The same item is then looked up again by By.DESCRIPTION, so the harness'
    other locator strategies are exercised against a real applet tree and not
    just against the By constants (see by_test.qml).
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

PlasmoidTestItem {
    id: plasmoid
    appletId: "org.kde.plasma.analogclock"
    width: 256
    height: 256

    TestCase {
        when: windowShown

        function init() {
            // See PlasmoidTestItem::isReady() for why this wait exists.
            tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
        }

        function test_0_open() {
            const clock = plasmoid.findItem(By.NAME, "Analog Clock")
            if (!clock) {
                console.warn("dumpTree:\n" + plasmoid.dumpTree())
            }
            verify(clock !== null)

            const description = clock.Accessible.description
            verify(description !== "", "analog clock has no accessible description")
            const byDescription = plasmoid.findItem(By.DESCRIPTION, description)
            if (byDescription !== clock) {
                console.warn("dumpTree:\n" + plasmoid.dumpTree())
            }
            verify(byDescription === clock, "By.DESCRIPTION did not find the analog clock")
        }
    }
}
