/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

import QtQuick
import org.kde.plasma.plasma5support as P5Support

Text {
    id: root
    text: "No command is running"
    P5Support.DataSource {
        engine: "executable"
        connectedSources: ["echo hi"]
        onNewData: (sourceName, data) => {
            root.text = data["stdout"]
            disconnectSource(sourceName)
        }
    }
}