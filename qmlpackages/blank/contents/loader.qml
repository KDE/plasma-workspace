/*   vim:set foldmethod=marker:
 *
 *   Copyright (C) 2013 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2
import org.kde.plasma.hardware 0.1 as Hardware
import org.kde.plasma.platformcomponents 1.0 as Platform
import QtQuick.Window 2.0

Item {
    id: main

    property string shell  : "org.kde.blank"
    property bool willing  : hardware.touchscreenPresent
    property int  priority : 0

    // This is not needed, but allows the
    // handler to know whether its shell is loaded
    property bool loaded   : false

    Hardware.Interface { id: hardware }

    Platform.Application {
        application: "xterm"
        running: main.loaded
    }

    // TODO: This is not needed - just for testing purposes
    Window {
        id: blankDialog

        visible: main.loaded

        width: 500
        height: 500

        Rectangle {
            anchors.fill: parent
            color: "white"
        }

        Text {
            anchors.fill: parent
            font.pointSize: 32
            text: "Blank"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:   Text.AlignVCenter
        }
    }
}

