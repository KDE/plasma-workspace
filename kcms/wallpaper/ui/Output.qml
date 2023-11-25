/*
    SPDX-FileCopyrightText: 2019 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2012 Dan Vratil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
    SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami

Item {
    
    signal screenSelected(string screenName)
    
    id: output

    // readonly property bool isSelected: root.selectedOutput === model.index
    property real relativeFactor
    property var screen
    property int xOffset
    property int yOffset
    property bool isSelected
    
    property size outputSize: Qt.size(screen.geometry.width, screen.geometry.height)
    property point position: Qt.point(screen.geometry.x, screen.geometry.y)
        
    x: position.x / relativeFactor + xOffset
    y:  position.y / relativeFactor + yOffset

    width: outputSize.width / relativeFactor
    height: outputSize.height / relativeFactor

    Rectangle {
        id: outline

        readonly property int orientationPanelWidth: 10
        readonly property real orientationPanelPosition: 1 - (orientationPanelWidth / outline.height)

        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        radius: Kirigami.Units.smallSpacing

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Kirigami.Theme.alternateBackgroundColor
            }
            GradientStop {
                // Create a hard cut. Can't use the same number otherwise it gets confused.
                position: outline.orientationPanelPosition - Number.EPSILON
                color: Kirigami.Theme.alternateBackgroundColor
            }
            GradientStop {
                position: outline.orientationPanelPosition
                color: outline.border.color
            }
            GradientStop {
                position: 1.0
                color: outline.border.color
            }
        }

        border {
            color: isSelected ? Kirigami.Theme.highlightColor : Kirigami.Theme.disabledTextColor
            width: 1

            Behavior on color {
                PropertyAnimation {
                    duration: Kirigami.Units.longDuration
                }
            }
        }
    }

    Item {
        id: labelContainer
        anchors {
            fill: parent
            margins: outline.border.width
        }

        // so the text is drawn above orientationPanelContainer
        z: 1
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 0
            width: parent.width
            Layout.maximumHeight: parent.height

            QQC2.Label {
                Layout.fillWidth: true
                Layout.maximumHeight: labelContainer.height - resolutionLabel.implicitHeight

                text: screen.name
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            QQC2.Label {
                id: resolutionLabel
                Layout.fillWidth: true

                text: "(" + outputSize.width + "x" + outputSize.height +  ")"
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }
    }

    Rectangle {
        id: posLabel

        y: 4
        x: 4
        width: childrenRect.width + 5
        height: childrenRect.height + 2
        radius: 4

        opacity: model.enabled &&
                 (tapHandler.isLongPressed || dragHandler.active) ? 0.9 : 0.0


        color: Kirigami.Theme.disabledTextColor

        Text {
            id: posLabelText

            y: 2
            x: 2

            // text: model.normalizedPosition.x + "," + model.normalizedPosition.y
            color: "white"
        }

        Behavior on opacity {
            PropertyAnimation {
                duration: Kirigami.Units.longDuration
            }
        }
    }
    
    TapHandler {
        id: tapHandler
        property bool isLongPressed: false
        gesturePolicy: TapHandler.WithinBounds

        onPressedChanged: {
            if (pressed) {
                output.screenSelected(screen.name)
            } else {
                isLongPressed = false;
            }
        }
        onLongPressed: isLongPressed = true;
        longPressThreshold: 0.3
    }
}

