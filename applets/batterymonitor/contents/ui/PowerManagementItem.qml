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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.kquickcontrolsaddons 2.0

FocusScope {
    id: brightnessItem
    clip: true
    width: parent.width
    height: Math.max(pmCheckBox.height, pmLabel.height) + padding.margins.top + padding.margins.bottom

    property bool enabled: pmCheckBox.checked

    Components.CheckBox {
        id: pmCheckBox
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: padding.margins.left + (units.iconSizes.medium - width)
            topMargin: padding.margins.top
            bottomMargin: padding.margins.bottom
        }
        focus: true
        checked: true
    }

    Components.Label {
        id: pmLabel
        anchors {
            verticalCenter: pmCheckBox.verticalCenter
            left: pmCheckBox.right
            leftMargin: 6
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

