/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

Column {
    id: root
    spacing: Kirigami.Units.smallSpacing

    property ButtonGroup group
    property alias checked: radioButton.checked
    property alias preview: previewImage.source
    property alias text: label.text

    signal toggled()
    signal expanded()

    Kirigami.ShadowedRectangle {
        id: delegate
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.View
        implicitWidth: radioButton.implicitWidth + toolButton.implicitWidth
        implicitHeight: radioButton.implicitHeight
        color: Kirigami.Theme.backgroundColor
        radius: Kirigami.Units.cornerRadius
        shadow.xOffset: 0
        shadow.yOffset: 2
        shadow.size: 10
        shadow.color: Qt.rgba(0, 0, 0, 0.3)

        Row {
            anchors.fill: parent

            RadioButton {
                id: radioButton
                ButtonGroup.group: root.group
                implicitWidth: implicitHeight * 1.6
                implicitHeight: Kirigami.Units.gridUnit * 5

                background: Item {}
                indicator: Item {}

                contentItem: Item {
                    Rectangle {
                        anchors.fill: parent
                        topLeftRadius: Kirigami.Units.cornerRadius
                        bottomLeftRadius: Kirigami.Units.cornerRadius
                        topRightRadius: 0
                        bottomRightRadius: 0
                        color: {
                            if (radioButton.checked) {
                                return Kirigami.Theme.highlightColor;
                            } else if (radioButton.hovered) {
                                return Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.5);
                            } else {
                                return Kirigami.Theme.backgroundColor;
                            }
                        }
                    }

                    Image {
                        id: previewImage
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.smallSpacing
                        asynchronous: true
                        sourceSize: Qt.size(width * Screen.devicePixelRatio, height * Screen.devicePixelRatio)
                    }
                }

                onToggled: root.toggled()
            }

            AbstractButton {
                id: toolButton
                implicitWidth: Kirigami.Units.iconSizes.small + leftPadding + rightPadding
                implicitHeight: Kirigami.Units.gridUnit * 5
                leftPadding: Kirigami.Units.smallSpacing
                rightPadding: Kirigami.Units.smallSpacing

                contentItem: Kirigami.Icon {
                    source: "arrow-right"
                }

                background: Rectangle {
                    color: {
                        if (toolButton.pressed) {
                            return Kirigami.Theme.highlightColor;
                        } else if (toolButton.hovered || toolButton.visualFocus) {
                            return Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.5);
                        } else {
                            return Kirigami.Theme.backgroundColor;
                        }
                    }
                    topLeftRadius: 0
                    bottomLeftRadius: 0
                    topRightRadius: Kirigami.Units.cornerRadius
                    bottomRightRadius: Kirigami.Units.cornerRadius
                }

                onClicked: {
                    root.expanded()
                }
            }
        }
    }

    Label {
        id: label
        width: delegate.implicitWidth
        textFormat: Text.PlainText
        horizontalAlignment: Text.AlignHCenter

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            width: parent.paintedWidth
            height: 1
            color: Kirigami.Theme.highlightColor
            opacity: radioButton.visualFocus ? 1 : 0
        }
    }
}
