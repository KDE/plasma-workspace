/*
 * Copyright 2020 Carson Black <uhhadd@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.10
import QtQuick.Controls 2.10
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PC3

AbstractButton {
    id: controlRoot

    hoverEnabled: true

    enum State {
        Rest,
        Hover,
        Down
    }

    leftPadding: rest.margins.left
    topPadding: rest.margins.top
    rightPadding: rest.margins.right
    bottomPadding: rest.margins.bottom

    background: Item {
        id: background

        property int state: {
            if (controlRoot.down) {
                return MenuDelegate.State.Down
            } else if (controlRoot.hovered) {
                return MenuDelegate.State.Hover
            }
            return MenuDelegate.State.Rest
        }

        PlasmaCore.FrameSvgItem {
            id: rest
            anchors.fill: parent
            visible: background.state == MenuDelegate.State.Rest
            imagePath: "widgets/menubaritem"
            prefix: "normal"
        }
        PlasmaCore.FrameSvgItem {
            id: hover
            anchors.fill: parent
            visible: background.state == MenuDelegate.State.Hover
            imagePath: "widgets/menubaritem"
            prefix: "hover"
        }
        PlasmaCore.FrameSvgItem {
            id: down
            anchors.fill: parent
            visible: background.state == MenuDelegate.State.Down
            imagePath: "widgets/menubaritem"
            prefix: "pressed"
        }
    }

    contentItem: PC3.Label {
        text: controlRoot.text
    }
}
