/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces appiumtests/applets/calendartest.py:
        self.driver.find_element(AppiumBy.NAME, "Today")
        self.driver.find_element(AppiumBy.NAME, "Days")
        self.driver.find_element(AppiumBy.NAME, "Months")
        self.driver.find_element(AppiumBy.NAME, "Years")
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

PlasmoidTestItem {
    id: plasmoid
    appletId: "org.kde.plasma.calendar"
    width: 512
    height: 512

    TestCase {
        when: windowShown

        function init() {
            // See PlasmoidTestItem::isReady() for why this wait exists.
            tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
        }

        function test_0_open() {
            const missing = []
            for (const name of ["Today", "Days", "Months", "Years"]) {
                if (!plasmoid.findItem(By.NAME, name)) {
                    missing.push(name)
                }
            }
            if (missing.length > 0) {
                console.warn("missing: " + missing.join(", ") + "\ndumpTree:\n" + plasmoid.dumpTree())
            }
            compare(missing.length, 0)
        }
    }
}
