/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.0
import QtQuick.Window 2.1
import QtQuick.Controls 2.2 as QtControls
import org.kde.kirigami 2.4 as Kirigami
import org.kde.ksvg 1.0 as KSvg

import org.kde.notificationmanager 1.0 as NotificationManager

Item {
    id: monitorPanel

    property int baseUnit: Kirigami.Units.gridUnit

    implicitWidth: baseUnit * 13 + baseUnit * 2
    implicitHeight: (screenRatio * baseUnit * 13) + (baseUnit * 2) + basePart.height

    property int selectedPosition
    property var disabledPositions: []
    property real screenRatio: Screen.height / Screen.width

    onSelectedPositionChanged: {
        var buttons = positionRadios.buttons.length;
        for (var i = 0; i < buttons.length; ++i) {
            var button = buttons[i];
            if (button.position === selectedPosition) {
                button.checked = true;
                break;
            }
        }
    }

    KSvg.SvgItem {
        id: topleftPart
        anchors {
            left: parent.left
            top: parent.top
        }
        imagePath: "widgets/monitor"
        elementId: "topleft"
        width: baseUnit
        height: baseUnit
    }

    KSvg.SvgItem {
        id: topPart
        anchors {
            top: parent.top
            left: topleftPart.right
            right: toprightPart.left
        }
        imagePath: "widgets/monitor"
        elementId: "top"
        height: baseUnit
    }

    KSvg.SvgItem {
        id: toprightPart
        anchors {
            right: parent.right
            top: parent.top
        }
        imagePath: "widgets/monitor"
        elementId: "topright"
        width: baseUnit
        height: baseUnit
    }

    KSvg.SvgItem {
        id: leftPart
        anchors {
            left: parent.left
            top: topleftPart.bottom
            bottom: bottomleftPart.top
        }
        imagePath: "widgets/monitor"
        elementId: "left"
        width: baseUnit
    }

    KSvg.SvgItem {
        id: rightPart
        anchors {
            right: parent.right
            top: toprightPart.bottom
            bottom: bottomrightPart.top
        }
        imagePath: "widgets/monitor"
        elementId: "right"
        width: baseUnit
    }

    KSvg.SvgItem {
        id: bottomleftPart
        anchors {
            left: parent.left
            bottom: basePart.top
        }
        imagePath: "widgets/monitor"
        elementId: "bottomleft"
        width: baseUnit
        height: baseUnit
    }

    KSvg.SvgItem {
        id: bottomPart
        anchors {
            bottom: basePart.top
            left: bottomleftPart.right
            right: bottomrightPart.left
        }
        imagePath: "widgets/monitor"
        elementId: "bottom"
        height: baseUnit
    }

    KSvg.SvgItem {
        id: bottomrightPart
        anchors {
            right: parent.right
            bottom: basePart.top
        }
        imagePath: "widgets/monitor"
        elementId: "bottomright"
        width: baseUnit
        height: baseUnit
    }

    KSvg.SvgItem {
        id: basePart
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        width: 120
        height: 60
        imagePath: "widgets/monitor"
        elementId: "base"
    }

    QtControls.ButtonGroup {
        id: positionRadios
        onCheckedButtonChanged: monitorPanel.selectedPosition = checkedButton.position
    }

    // TODO increase hit area for radio buttons

    QtControls.RadioButton {
        anchors {
            top: topPart.bottom
            left: leftPart.right
            margins: Kirigami.Units.smallSpacing
        }
        readonly property int position: NotificationManager.Settings.TopLeft
        checked: monitorPanel.selectedPosition == position
        visible: monitorPanel.disabledPositions.indexOf(position) == -1
        QtControls.ButtonGroup.group: positionRadios
    }
    QtControls.RadioButton {
        anchors {
            top: topPart.bottom
            horizontalCenter: topPart.horizontalCenter
            margins: Kirigami.Units.smallSpacing
        }
        readonly property int position: NotificationManager.Settings.TopCenter
        checked: monitorPanel.selectedPosition == position
        visible: monitorPanel.disabledPositions.indexOf(position) == -1
        QtControls.ButtonGroup.group: positionRadios
    }
    QtControls.RadioButton {
        anchors {
            top: topPart.bottom
            right: rightPart.left
            margins: Kirigami.Units.smallSpacing
        }
        readonly property int position: NotificationManager.Settings.TopRight
        checked: monitorPanel.selectedPosition == position
        visible: monitorPanel.disabledPositions.indexOf(position) == -1
        QtControls.ButtonGroup.group: positionRadios
    }
    QtControls.RadioButton {
        anchors {
            bottom: bottomPart.top
            left: leftPart.right
            margins: Kirigami.Units.smallSpacing
        }
        readonly property int position: NotificationManager.Settings.BottomLeft
        checked: monitorPanel.selectedPosition == position
        visible: monitorPanel.disabledPositions.indexOf(position) == -1
        QtControls.ButtonGroup.group: positionRadios
    }
    QtControls.RadioButton {
        anchors {
            bottom: bottomPart.top
            horizontalCenter: bottomPart.horizontalCenter
            margins: Kirigami.Units.smallSpacing
        }
        readonly property int position: NotificationManager.Settings.BottomCenter
        checked: monitorPanel.selectedPosition == position
        visible: monitorPanel.disabledPositions.indexOf(position) == -1
        QtControls.ButtonGroup.group: positionRadios
    }
    QtControls.RadioButton {
        anchors {
            bottom: bottomPart.top
            right: rightPart.left
            margins: Kirigami.Units.smallSpacing
        }
        readonly property int position: NotificationManager.Settings.BottomRight
        checked: monitorPanel.selectedPosition == position
        visible: monitorPanel.disabledPositions.indexOf(position) == -1
        QtControls.ButtonGroup.group: positionRadios
    }
}
