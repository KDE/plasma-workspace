/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

PlasmaComponents3.Label {
    maximumLineCount: 3
    verticalAlignment: Text.AlignVCenter

    text: {
        var highlightFontTag = "<font color='" + PlasmaCore.Theme.highlightColor + "'>%1</font>"

        var text = DisplayRole.slice(0, 100)

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
}
