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
    height: Math.max(brightnessIcon.height, brightnessLabel.height + brightnessSlider.height) + padding.margins.top + padding.margins.bottom

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
            verticalCenter: parent.verticalCenter
            topMargin: padding.margins.top
            bottomMargin: padding.margins.bottom
            left: parent.left
            leftMargin: padding.margins.left
        }
    }

    Components.Label {
        id: brightnessLabel
        anchors {
            top: parent.top
            topMargin: padding.margins.top
            left: brightnessIcon.right
            leftMargin: 6
        }
        height: paintedHeight
    }

    Components.Slider {
        id: brightnessSlider
        anchors {
            bottom: parent.bottom
            bottomMargin: padding.margins.bottom
            left: brightnessIcon.right
            right: brightnessPercent.left
            leftMargin: 6
            rightMargin: 6
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
            rightMargin: padding.margins.right
            verticalCenter: brightnessSlider.verticalCenter
        }
        height: paintedHeight
        text: i18nc("Placeholder is brightness percentage", "%1%", percentage)
    }
}

