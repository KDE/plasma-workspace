/*
 *   Copyright 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
 *   Copyright 2013 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.components 2.0 as Components
import org.kde.kquickcontrolsaddons 2.0

FocusScope {
    id: brightnessItem
    clip: true
    width: parent.width
    height: Math.max(pmCheckBox.height, pmLabel.height) + dialog.anchors.topMargin + dialog.anchors.bottomMargin + units.gridUnit

    property bool enabled: pmCheckBox.checked

    Components.CheckBox {
        id: pmCheckBox
        anchors {
            top: parent.top
            left: parent.left
            leftMargin: units.iconSizes.medium - pmCheckBox.width
        }
        focus: true
        checked: true
    }

    Components.Label {
        id: pmLabel
        anchors {
            verticalCenter: pmCheckBox.verticalCenter
            left: pmCheckBox.right
            leftMargin: units.gridUnit / 2
        }
        height: paintedHeight
        text: i18n("Enable Power Management")
    }

    MouseArea {
        anchors.fill: parent

        onReleased: pmCheckBox.released();
        onPressed: pmCheckBox.forceActiveFocus();
    }
}

