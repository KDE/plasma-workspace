/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

import QtQuick
import QtTest
import org.kde.plasma.workspace.dbus as DBus
import org.kde.plasma.private.kicker as Kicker

TestCase {
    id: root
    width: 100
    height: 100
    when: windowShown

    readonly property string service: "org.kde.kickertest"
    readonly property string path: "/test"
    readonly property string iface: service

    function init() {
        compare(favoriteList.count, 0);
    }

    function cleanup() {
        compare(favoriteList.count, 0);
    }

    function test_add_remove_favorite() {
        rootModel.favoritesModel.addFavorite("applications:kickertest.desktop");
        compare(favoriteList.count, 1);
        compare(favoriteList.itemAt(0).text, "DONT TRANSLATE TEST ONLY");
        rootModel.favoritesModel.removeFavorite("applications:kickertest.desktop");
        tryCompare(favoriteList, "count", 0);
    }

    function test_hide_invalid_entries() {
        rootModel.favoritesModel.addFavorite("applications:kickertest.desktop");
        compare(favoriteList.count, 1);
        compare(favoriteList.itemAt(0).text, "DONT TRANSLATE TEST ONLY");

        let pendingReply = DBus.SessionBus.asyncCall({
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": "DeleteAndRebuildDatabase1",
        });
        tryCompare(pendingReply, "isFinished", true);
        console.log("Waiting 3s...")
        wait(3000);
        compare(favoriteList.count, 1);

        DBus.SessionBus.asyncCall({
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": "DeleteAndRebuildDatabase2",
        });
        tryCompare(favoriteList, "count", 0);

        rootModel.favoritesModel.removeFavorite("applications:kickertest.desktop");
    }

    Kicker.RootModel {
        id: rootModel
        autoPopulate: false
        appNameFormat: 0 // NameOnly
        flat: false
        sorted: true
        showSeparators: true
        showAllApps: false
        showAllAppsCategorized: true
        showTopLevelItems: true
        showRecentApps: true
        showRecentDocs: true
        recentOrdering: 0 // RecentFirst
        Component.onCompleted: {
            favoritesModel.initForClient("org.kde.plasma.kicker.favorites.instance-testclient")
        }
    }
    Repeater {
        id: favoriteList
        model: rootModel.favoritesModel
        delegate: Text {
            required property var model
            width: parent.width
            text: model.compactNameWrapped ?? model.compactName ?? model.displayWrapped ?? model.display
            textFormat: Text.PlainText
        }
    }
}
