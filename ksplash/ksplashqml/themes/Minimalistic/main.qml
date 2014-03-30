/*   vim:set foldenable foldmethod=marker:
 *
 *   Copyright (C) 2011 Ivan Cukic <ivan.cukic(at)kde.org>
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

import QtQuick 2.0

Item {
    id: main

    width: screenSize.width
    height: screenSize.height
    // width: 300
    // height: 300

    /* property declarations --------------------------{{{ */
    property int stage
    property int iconSize: (screenSize.width <= 1024) ? 64 : 128
    /* }}} */

    /* signal declarations ----------------------------{{{ */

    /* }}} */

    /* JavaScript functions ---------------------------{{{ */
    onStageChanged: {
        if (stage == 1) {
            background.opacity = 1
            gear.opacity = 0.5
        }
        if (stage == 2) {
            gear.opacity = 1
            mask.opacity = 1
            letter.opacity = 1
        }
        if (stage == 3) {
        }
        if (stage == 4) {
        }
        if (stage == 5) {
            logo.opacity = 1
        }
        if (stage == 6) {
        }
    }
    /* }}} */

    /* object properties ------------------------------{{{ */

    /* }}} */

    /* child objects ----------------------------------{{{ */

    Rectangle {
        color: "black"
        anchors.fill: parent
    }

    Rectangle {
        id: background

        gradient: Gradient {
            GradientStop { position: 0.0; color: "#55555f" }
            GradientStop { position: 1.0; color: "#000000" }
        }

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        height: gear.y - 32

        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 1000; easing { type: Easing.InOutQuad } } }
    }

    Image {
        id: gear

        height: iconSize
        width: iconSize
        smooth: true

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        source: "images/kdegear.png"

        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 1000; easing { type: Easing.InOutQuad } } }

        NumberAnimation {
            id: animateRotation
            target: gear
            properties: "rotation"
            from: 0
            to: 360
            duration: 5000

            loops: Animation.Infinite
            running: true
        }

    }

    Image {
        id: mask

        height: iconSize
        width: iconSize
        smooth: true

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        source: "images/kdemask.png"

        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 1000; easing { type: Easing.InOutQuad } } }
    }

    Image {
        id: letter

        height: iconSize
        width: iconSize
        smooth: true

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        source: "images/kdeletter.png"

        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 1000; easing { type: Easing.InOutQuad } } }
    }

    Image {
        id: logo

        height: iconSize
        width: iconSize
        smooth: true

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2 - 1

        source: "images/kdelogo-contrast.png"

        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 1000; easing { type: Easing.InOutQuad } } }
    }

    /* }}} */

    /* stages -----------------------------------------{{{ */

    /* }}} */

    /* transitions ------------------------------------{{{ */

    /* }}} */
}

