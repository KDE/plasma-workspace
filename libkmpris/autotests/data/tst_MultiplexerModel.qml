/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import org.kde.plasma.private.mpris as Mpris

TestCase {
    id: root

    Repeater {
        id: repeater
        model: Mpris.MultiplexerModel {
            id: mpris
        }
        Item {
            required property bool isMultiplexer
        }
    }

    function test_count() {
        tryCompare(repeater, "count", 1);
    }

    function test_isMultiplexer() {
        const item = repeater.itemAt(0);
        verify(item.isMultiplexer);
    }
}
