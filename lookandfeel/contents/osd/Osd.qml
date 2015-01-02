/*
 * Copyright 2014 Martin Klapetek <mklapetek@kde.org>
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

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtra

PlasmaCore.Dialog {
    id: root
    location: PlasmaCore.Types.Floating
    type: PlasmaCore.Dialog.OnScreenDisplay
    outputOnly: true

    // OSD Timeout in msecs - how long it will stay on the screen
    property int timeout: 1800
    // This is either a text or a number, if showingProgress is set to true,
    // the number will be used as a value for the progress bar
    property var osdValue
    // Icon name to display
    property string icon
    // Set to true if the value is meant for progress bar,
    // false for displaying the value as normal text
    property bool showingProgress: false

    mainItem: Item {
        height: units.gridUnit * 15
        width: height

        //  /--------------------\
        //  |      spacing       |
        //  | /----------------\ |
        //  | |                | |
        //  | |      icon      | |
        //  | |                | |
        //  | |                | |
        //  | \----------------/ |
        //  |      spacing       |
        //  | [progressbar/text] |
        //  |      spacing       |
        //  \--------------------/

        PlasmaCore.IconItem {
            id: icon

            height: parent.height - progressBar.height - ((units.largeSpacing/2) * 3) //it's an svg
            width: parent.width

            source: root.icon
        }

        PlasmaComponents.ProgressBar {
            id: progressBar

            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                margins: Math.floor(units.largeSpacing / 2)
            }

            visible: root.showingProgress
            minimumValue: 0
            maximumValue: 100
            value: visible ? root.osdValue : 0

        }
        PlasmaExtra.Heading {
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                margins: Math.floor(units.largeSpacing / 2)
            }

            visible: !root.showingProgress
            text: root.showingProgress ? "" : (root.osdValue ? root.osdValue : "")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.NoWrap
            elide: Text.ElideLeft
        }
    }
}
