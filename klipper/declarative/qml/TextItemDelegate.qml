/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick 2.15

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

ClipboardItemDelegate {
    id: menuItem
    Accessible.name: mainLabel.text
    mainItem: PlasmaComponents3.Label {
        id: mainLabel
        maximumLineCount: 3
        verticalAlignment: Text.AlignVCenter

        text: {
            if (!menuItem.model?.display) {
                return "";
            }
            let highlightFontTag = "<font color='" + Kirigami.Theme.highlightColor + "'>%1</font>"
            let text = menuItem.model.display.slice(0, 100)

            // first escape any HTML characters to prevent privacy issues
            text = text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")

            // color code leading or trailing whitespace
            // the first regex is basically "trim"
            text = text.replace(/^\s+|\s+$/gm, function(match) {
                // then inside the trimmed characters ("match") we replace each one individually
                match = match.replace(/ /g, "␣") // space
                            .replace(/\t/g, "⇥") // tab
                            .replace(/\n/g, "↵") // return
                return highlightFontTag.arg(match)
            })

            // finally turn line breaks into HTML br tags
            text = text.replace(/\r\n|\r|\n/g, "<br>")

            return text
        }
        elide: Text.ElideRight
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        textFormat: Text.StyledText

        Accessible.ignored: true

        Drag.active: menuItem.dragHandler.active
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction
        Drag.mimeData: {
            "text/plain": menuItem.model?.display ?? "",
        }
    }
}


