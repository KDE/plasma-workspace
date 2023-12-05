/*
    SPDX-FileCopyrightText: 2019 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2012 Dan Vratil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
    SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: output

    // readonly property bool isSelected: root.selectedOutput === model.index
    property real relativeFactor
    property var screen
    property int xOffset
    property int yOffset
    property bool isSelected
    
    readonly property size outputSize: Qt.size(screen.geometry.width, screen.geometry.height)
    readonly property point position: Qt.point(screen.geometry.x, screen.geometry.y)

    signal screenSelected(string screenName)

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

    states: [
        State {
            name: "transposed"
            PropertyChanges {
                target: outline
                width: output.height
                height: output.width
            }
        },

        State {
            name: "rot90"
            extend: "transposed"
            when: screen.orientation === 1 // Qt::PortraitOrientation
            PropertyChanges {
                target: outline
                rotation: 90
            }
            PropertyChanges {
                target: labelContainer
                anchors.leftMargin: outline.orientationPanelWidth + outline.border.width
            }
        },
        State {
            name: "rot0"
            when: screen.orientation === 2 // Qt::LandscapeOrientation
            PropertyChanges {
                target: labelContainer
                anchors.bottomMargin: outline.orientationPanelWidth + outline.border.width
            }
        },
        State {
            name: "rot270"
            extend: "transposed"
            when: screen.orientation === 4 // Qt::InvertedPortraitOrientation
            PropertyChanges {
                target: outline
                rotation: 270
            }
            PropertyChanges {
                target: labelContainer
                anchors.rightMargin: outline.orientationPanelWidth + outline.border.width
            }
        },
        State {
            name: "rot180"
            when: screen.orientation === 8 // Qt::InvertedLandscapeOrientation
            PropertyChanges {
                target: outline
                rotation: 180
            }
            PropertyChanges {
                target: labelContainer
                anchors.topMargin: outline.orientationPanelWidth + outline.border.width
            }
        }
    ]

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
    
    TapHandler {
        id: tapHandler
        gesturePolicy: TapHandler.WithinBounds

        onPressedChanged: {
            if (pressed) {
                output.screenSelected(screen.name)
            }
        }
    }
}

