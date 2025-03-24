/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import org.kde.plasma.workspace.dbus as DBus
import org.kde.plasma.private.mpris as Mpris

TestCase {
    id: root

    // make sure PlayerContainer is registered
    readonly property Mpris.PlayerContainer playerData: mpris.currentPlayer
    property DBus.DBusPendingReply pendingReply

    Repeater {
        id: repeater
        model: Mpris.Mpris2Model {
            id: mpris
        }
        Item { visible: false }
    }

    function test_count() {
        tryCompare(repeater, "count", 2);
    }

    function test_current() {
        verify(mpris.currentPlayer);
        // Make sure it's a real player container, and make sure PlayerContainer is registered
        compare(mpris.currentPlayer.identity, "Audacious");
        compare(mpris.currentIndex, 0);
    }

    // Make sure an unrelated launcher doesn't have any associated player container.
    // This checks the `pid > 0` condition when matching KDEPidRole
    function test_playerForLauncherUrl() {
        const promise = new Promise((resolve, reject) => {
            DBus.SessionBus.asyncCall({
                "service": "org.kde.mpristest",
                "path": "/mpristest",
                "iface": "org.kde.mpristest",
                "member": "GetPid",
            }, resolve, reject);
        }).then((reply) => {
            root.pendingReply = reply;
        }).catch((reply) => {
            root.fail(reply.error.message);
        });
        tryVerify(() => root.pendingReply);

        const player = mpris.playerForLauncherUrl("applications:invalidlauncherurl.desktop", Number(root.pendingReply.value));
        verify(player);
        compare(mpris.playerForLauncherUrl("applications:audacious.desktop", 0), player);
        compare(player.identity, "Audacious");

        verify(!mpris.playerForLauncherUrl("applications:dolphin.desktop", 0))
    }
}
