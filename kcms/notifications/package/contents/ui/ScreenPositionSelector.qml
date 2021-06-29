/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.0
import QtQuick.Window 2.1
import QtQuick.Controls 2.2 as QtControls
import org.kde.kirigami 2.4 as Kirigami
import org.kde.plasma.core 2.0 as PlasmaCore

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

    PlasmaCore.Svg {
        id: monitorSvg
        imagePath: "widgets/monitor"
    }

    PlasmaCore.SvgItem {
        id: topleftPart
        anchors {
            left: parent.left
            top: parent.top
        }
        svg: monitorSvg
        elementId: "topleft"
        width: baseUnit
        height: baseUnit
    }

    PlasmaCore.SvgItem {
        id: topPart
        anchors {
            top: parent.top
            left: topleftPart.right
            right: toprightPart.left
        }
        svg: monitorSvg
        elementId: "top"
        height: baseUnit
    }

    PlasmaCore.SvgItem {
        id: toprightPart
        anchors {
            right: parent.right
            top: parent.top
        }
        svg: monitorSvg
        elementId: "topright"
        width: baseUnit
        height: baseUnit
    }

    PlasmaCore.SvgItem {
        id: leftPart
        anchors {
            left: parent.left
            top: topleftPart.bottom
            bottom: bottomleftPart.top
        }
        svg: monitorSvg
        elementId: "left"
        width: baseUnit
    }

    PlasmaCore.SvgItem {
        id: rightPart
        anchors {
            right: parent.right
            top: toprightPart.bottom
            bottom: bottomrightPart.top
        }
        svg: monitorSvg
        elementId: "right"
        width: baseUnit
    }

    PlasmaCore.SvgItem {
        id: bottomleftPart
        anchors {
            left: parent.left
            bottom: basePart.top
        }
        svg: monitorSvg
        elementId: "bottomleft"
        width: baseUnit
        height: baseUnit
    }

    PlasmaCore.SvgItem {
        id: bottomPart
        anchors {
            bottom: basePart.top
            left: bottomleftPart.right
            right: bottomrightPart.left
        }
        svg: monitorSvg
        elementId: "bottom"
        height: baseUnit
    }

    PlasmaCore.SvgItem {
        id: bottomrightPart
        anchors {
            right: parent.right
            bottom: basePart.top
        }
        svg: monitorSvg
        elementId: "bottomright"
        width: baseUnit
        height: baseUnit
    }

    PlasmaCore.SvgItem {
        id: basePart
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        width: 120
        height: 60
        svg: monitorSvg
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
