/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces the open check of appiumtests/applets/appmenutest.py:
        self.driver.find_element(AppiumBy.NAME, "Global Menu")

    Only test_0_open is portable: test_1 needs a real window exporting
    com.canonical.dbusmenu, which is out of scope for this harness.
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

PlasmoidTestItem {
    id: plasmoid
    appletId: "org.kde.plasma.appmenu"
    width: 256
    height: 256

    TestCase {
        when: windowShown

        function init() {
            // See PlasmoidTestItem::isReady() for why this wait exists.
            tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
        }

        function test_0_open() {
            const menu = plasmoid.findItem(By.NAME, "Global Menu")
            if (!menu) {
                console.warn("dumpTree:\n" + plasmoid.dumpTree())
            }
            verify(menu !== null)
        }
    }
}
