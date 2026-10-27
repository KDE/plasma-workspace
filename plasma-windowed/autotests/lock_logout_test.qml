/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces appiumtests/applets/lock_logouttest.py:
        self.driver.find_element(AppiumBy.NAME, "Lock")
        self.driver.find_element(AppiumBy.NAME, "Shut down")
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

PlasmoidTestItem {
    id: plasmoid
    appletId: "org.kde.plasma.lock_logout"
    width: 256
    height: 256

    TestCase {
        when: windowShown

        function init() {
            // See PlasmoidTestItem::isReady() for why this wait exists.
            tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
        }

        function test_0_open() {
            verify(plasmoid.findItem(By.NAME, "Lock") !== null)
            verify(plasmoid.findItem(By.NAME, "Shut down") !== null)
        }
    }
}
