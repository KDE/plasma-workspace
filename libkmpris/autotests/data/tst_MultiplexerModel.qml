/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.plasma.private.mpris as MPRIS

Item {
    id: root
    readonly property alias count: repeater.count
    property QtObject modelData
    MPRIS.MultiplexerModel {
        id: mpris
    }
    Repeater {
        id: repeater
        model: mpris
        Item {
            Component.onCompleted: {
                root.modelData = model;
            }
        }
    }
}