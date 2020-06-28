/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright     2014 Sebastian Kügler <sebas@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

import QtQuick 2.0

import org.kde.plasma.components 3.0 as PlasmaComponents3

PlasmaComponents3.Label {
    maximumLineCount: 3
    verticalAlignment: Text.AlignVCenter

    text: {
        var highlightFontTag = "<font color='" + theme.highlightColor + "'>%1</font>"

        var text = DisplayRole.slice(0, 100)

        // first escape any HTML characters to prevent privacy issues
        text = text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")

        // color code leading or trailing whitespace
        // the first regex is basically "trim"
        text = text.replace(/^\s+|\s+$/gm, function(match) {
            // then inside the trimmed characters ("match") we replace each one individually
            match = match.replace(/ /g, "␣") // space
                         .replace(/\t/g, "↹") // tab
                         .replace(/\n/g, "↵") // return
            return highlightFontTag.arg(match)
        })

        // finally turn line breaks into HTML br tags
        text = text.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, "<br>")

        return text
    }
    elide: Text.ElideRight
    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
    textFormat: Text.StyledText
}
