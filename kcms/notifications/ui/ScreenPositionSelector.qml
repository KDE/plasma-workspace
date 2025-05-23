/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2025 Akseli Lahtinen <akselmo@akselmo.dev>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Window
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import QtQuick.Layouts

import org.kde.notificationmanager as NotificationManager

Item {
    id: monitorPanel
    implicitWidth: baseUnit * 13 + baseUnit * 2
    implicitHeight: (screenRatio * baseUnit * 13) + (baseUnit * 2)

    property int baseUnit: Kirigami.Units.gridUnit
    property int selectedPosition
    property var disabledPositions: []
    property real screenRatio: Screen.height / Screen.width

    Rectangle {
        id: monitorRectangle
        anchors.centerIn: monitorPanel
        border.color: Kirigami.Theme.textColor
        border.width: 1
        color: Kirigami.Theme.alternateBackgroundColor
        width: monitorPanel.width - (Kirigami.Units.smallSpacing * 2)
        height: monitorPanel.height - (Kirigami.Units.smallSpacing * 2)
        radius: Kirigami.Units.cornerRadius
    }

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

    QtControls.ButtonGroup {
        id: positionRadios
        onCheckedButtonChanged: monitorPanel.selectedPosition = checkedButton.position
    }

    // TODO increase hit area for radio buttons
    GridLayout {
        id: radioButtonLayout
        anchors.centerIn: monitorRectangle
        anchors.margins: Kirigami.Units.smallSpacing
        columns: 3
        rowSpacing: 0
        width: monitorRectangle.width
        height: monitorRectangle.height

        QtControls.RadioButton {
            Layout.margins: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            contentItem: Item{}
            readonly property int position: NotificationManager.Settings.TopLeft
            checked: monitorPanel.selectedPosition == position
            visible: monitorPanel.disabledPositions.indexOf(position) == -1
            QtControls.ButtonGroup.group: positionRadios
        }
        QtControls.RadioButton {
            Layout.margins: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            contentItem: Item{}
            readonly property int position: NotificationManager.Settings.TopCenter
            checked: monitorPanel.selectedPosition == position
            visible: monitorPanel.disabledPositions.indexOf(position) == -1
            QtControls.ButtonGroup.group: positionRadios
        }
        QtControls.RadioButton {
            Layout.margins: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            contentItem: Item{}
            readonly property int position: NotificationManager.Settings.TopRight
            checked: monitorPanel.selectedPosition == position
            visible: monitorPanel.disabledPositions.indexOf(position) == -1
            QtControls.ButtonGroup.group: positionRadios
        }
        QtControls.RadioButton {
            Layout.margins: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
            contentItem: Item{}
            readonly property int position: NotificationManager.Settings.BottomLeft
            checked: monitorPanel.selectedPosition == position
            visible: monitorPanel.disabledPositions.indexOf(position) == -1
            QtControls.ButtonGroup.group: positionRadios
        }
        QtControls.RadioButton {
            Layout.margins: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            contentItem: Item{}
            readonly property int position: NotificationManager.Settings.BottomCenter
            checked: monitorPanel.selectedPosition == position
            visible: monitorPanel.disabledPositions.indexOf(position) == -1
            QtControls.ButtonGroup.group: positionRadios
        }
        QtControls.RadioButton {
            Layout.margins: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            contentItem: Item{}
            readonly property int position: NotificationManager.Settings.BottomRight
            checked: monitorPanel.selectedPosition == position
            visible: monitorPanel.disabledPositions.indexOf(position) == -1
            QtControls.ButtonGroup.group: positionRadios
        }
    }
}
