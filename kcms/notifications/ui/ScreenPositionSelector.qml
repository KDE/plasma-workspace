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
        border.color: Kirigami.Theme.highlightColor
        border.width: 1
        color: Kirigami.Theme.highlightColor
        opacity: 0.5
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

        Repeater {
            id: buttonRepeater
            model: [NotificationManager.Settings.TopLeft, NotificationManager.Settings.TopCenter, NotificationManager.Settings.TopRight, NotificationManager.Settings.BottomLeft, NotificationManager.Settings.BottomCenter, NotificationManager.Settings.BottomRight]
            QtControls.RadioButton {
                required property int modelData
                readonly property int position: modelData
                Layout.margins: Kirigami.Units.smallSpacing
                Layout.alignment: {
                    switch (position) {
                    case NotificationManager.Settings.TopLeft:
                        return Qt.AlignLeft | Qt.AlignTop;
                    case NotificationManager.Settings.TopCenter:
                        return Qt.AlignHCenter | Qt.AlignTop;
                    case NotificationManager.Settings.TopRight:
                        return Qt.AlignRight | Qt.AlignTop;
                    case NotificationManager.Settings.BottomLeft:
                        return Qt.AlignLeft | Qt.AlignBottom;
                    case NotificationManager.Settings.BottomCenter:
                        return Qt.AlignHCenter | Qt.AlignBottom;
                    case NotificationManager.Settings.BottomRight:
                        return Qt.AlignRight | Qt.AlignBottom;
                    }
                }
                contentItem: Item {}
                checked: monitorPanel.selectedPosition == position
                QtControls.ButtonGroup.group: positionRadios
            }
        }
    }
}
