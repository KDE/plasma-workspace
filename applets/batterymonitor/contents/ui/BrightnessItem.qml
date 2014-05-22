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
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

FocusScope {
    id: brightnessItem
    clip: true
    width: parent.width
    height: Math.max(brightnessIcon.height, brightnessLabel.height + brightnessSlider.height) + units.gridUnit

    property alias icon: brightnessIcon.icon
    property alias label: brightnessLabel.text
    property alias value: brightnessSlider.value
    property int percentage: 0

    signal changed(int screenBrightness)

    KQuickControlsAddons.QIconItem {
        id: brightnessIcon
        width: units.iconSizes.medium
        height: width
        anchors {
            top: parent.top
            left: parent.left
        }
    }

    Components.Label {
        id: brightnessLabel
        anchors {
            top: parent.top
            left: brightnessIcon.right
            leftMargin: units.gridUnit / 2
        }
        height: paintedHeight
    }

    Components.Slider {
        id: brightnessSlider
        anchors {
            top: brightnessLabel.bottom
            left: brightnessIcon.right
            right: parent.right
            leftMargin: units.gridUnit / 2
            rightMargin: units.gridUnit * 2.5
        }
        minimumValue: 0
        maximumValue: 100
        stepSize: 10
        focus: true
        onValueChanged: changed(value)
    }

    Components.Label {
        id: brightnessPercent
        anchors {
            right: parent.right
            rightMargin: units.gridUnit / 2
            verticalCenter: brightnessSlider.verticalCenter
        }
        height: paintedHeight
        text: i18nc("Placeholder is brightness percentage", "%1%", percentage)
    }
}

