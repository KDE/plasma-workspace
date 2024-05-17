/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.plasma.private.mpris as Mpris

Item {
    id: root
    readonly property alias count: repeater.count
    Mpris.Mpris2Model {
        id: mpris
    }
    Repeater {
        id: repeater
        model: mpris
        Item { }
    }

    // make sure PlayerContainer is registered
    readonly property Mpris.PlayerContainer playerData: mpris.currentPlayer
}
