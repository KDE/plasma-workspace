/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces appiumtests/applets/icontest.py:
        self.driver.find_element(AppiumBy.NAME, "Icon")
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

PlasmoidTestItem {
    id: plasmoid
    appletId: "org.kde.plasma.icon"
    width: 256
    height: 256

    TestCase {
        when: windowShown

        function init() {
            // See PlasmoidTestItem::isReady() for why this wait exists.
            tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
        }

        function test_0_open() {
            const icon = plasmoid.findItem(By.NAME, "Icon")
            if (!icon) {
                console.warn("dumpTree:\n" + plasmoid.dumpTree())
            }
            verify(icon !== null)
        }
    }
}
