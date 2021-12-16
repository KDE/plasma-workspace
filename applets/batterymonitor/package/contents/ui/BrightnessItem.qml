/*
    SPDX-FileCopyrightText: 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
    SPDX-FileCopyrightText: 2013, 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore

RowLayout {
    id: root

    property alias icon: brightnessIcon.source
    property alias label: brightnessLabel.text
    property alias slider: brightnessSlider
    property alias value: brightnessSlider.value
    property alias maximumValue: brightnessSlider.to
    property alias stepSize: brightnessSlider.stepSize
    property alias showPercentage: brightnessPercent.visible

    readonly property real percentage: Math.round(100 * value / maximumValue)

    signal moved()

    spacing: PlasmaCore.Units.gridUnit

    PlasmaCore.IconItem {
        id: brightnessIcon
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: PlasmaCore.Units.iconSizes.medium
        Layout.preferredHeight: width
    }

    Column {
        id: brightnessColumn
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        spacing: 0

        RowLayout {
            id: infoRow
            width: parent.width
            spacing: PlasmaCore.Units.smallSpacing

            PlasmaComponents3.Label {
                id: brightnessLabel
                Layout.fillWidth: true
            }

            PlasmaComponents3.Label {
                id: brightnessPercent
                horizontalAlignment: Text.AlignRight
                text: i18nc("Placeholder is brightness percentage", "%1%", root.percentage)
            }
        }

        PlasmaComponents3.Slider {
            id: brightnessSlider
            width: parent.width
            // Don't allow the slider to turn off the screen
            // Please see https://git.reviewboard.kde.org/r/122505/ for more information
            from: to > 100 ? 1 : 0
            stepSize: 1
            onMoved: root.moved()
        }
    }
}
