/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces appiumtests/applets/manage-inputmethodtest.py:
        self.driver.find_element(AppiumBy.NAME, "Input Method")
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

PlasmoidTestItem {
    id: plasmoid
    appletId: "org.kde.plasma.manage-inputmethod"
    width: 256
    height: 256

    TestCase {
        when: windowShown

        function init() {
            // See PlasmoidTestItem::isReady() for why this wait exists.
            tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
        }

        function test_0_open() {
            const im = plasmoid.findItem(By.NAME, "Input Method")
            if (!im) {
                console.warn("dumpTree:\n" + plasmoid.dumpTree())
            }
            verify(im !== null)
        }
    }
}
