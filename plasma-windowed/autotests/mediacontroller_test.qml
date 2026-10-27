/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Replaces appiumtests/applets/mediacontrollertest/test_1_basic_playback.py:
        presence_of_element_located((AppiumBy.NAME, "Katie's Favorite"))
        presence_of_element_located((AppiumBy.NAME, "KDE Community"))

    The MPRIS player is not started here: qmltestrunner cannot spawn services,
    so the CTest entry wraps this file in run_qmltest_with_bus.py, which runs
    mediaplayer.py on a private session bus before the test starts.
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

PlasmoidTestItem {
    id: plasmoid
    appletId: "org.kde.plasma.mediacontroller"
    width: 512
    height: 512

    TestCase {
        when: windowShown

        function init() {
            // See PlasmoidTestItem::isReady() for why this wait exists.
            tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
        }

        function test_0_single_player_with_metadata() {
            // The media controller picks the player up asynchronously through
            // NameOwnerChanged, so wait for the metadata instead of reading it
            // immediately after the applet became ready.
            tryVerify(() => plasmoid.findItem(By.NAME, "Katie's Favorite") !== null, 10000,
                      "track title of the MPRIS player did not appear")
            tryVerify(() => plasmoid.findItem(By.NAME, "KDE Community") !== null, 10000,
                      "track artist of the MPRIS player did not appear")
        }
    }
}
