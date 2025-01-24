/*
    SPDX-FileCopyrightText: 2024 Kuneho Cottonears <k-cottonears@posteo.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import QtQuick.Effects

import org.kde.kirigami as Kirigami
import org.kde.plasma.kcm.users 1.0 as UsersKCM

Kirigami.Dialog {
    id: cropSheet

    property alias imageUrl: image.source
    property bool isMousePressed: false
    property int handleToCursor: 0
    property var cursor: Qt.OpenHandCursor
    required property Kirigami.OverlaySheet picturesSheet
    required property Kirigami.Page usersDetailPage

    parent: usersDetailPage.QQC2.Overlay.overlay
    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    title: i18nc("@title:window", "Select Picture Area")
    width: Math.max(Math.round(parent.width * 0.6), Kirigami.Units.gridUnit * 20) - Kirigami.Units.gridUnit * 4

    onAccepted: {
        if (image.sourceSize.height !== image.height) {
            const multiplier = image.sourceSize.width / image.width;
            const calcX = crop.x * multiplier;
            const calcY = crop.y * multiplier;
            const calcW = crop.width * multiplier;
            const calcH = crop.height * multiplier;
            usersDetailPage.user.faceCrop = Qt.rect(calcX, calcY, calcW, calcH);
        } else {
            usersDetailPage.user.faceCrop = Qt.rect(crop.x, crop.y, crop.width, crop.height);
        }

        usersDetailPage.oldImage = usersDetailPage.user.face;
        usersDetailPage.user.face = imageUrl
        usersDetailPage.overrideImage = true;

        cropSheet.close();
        picturesSheet.close();
    }

    function keepMinimumSize(): void {
        if (crop.width < 50) {
            crop.width = 50;
            crop.height = 50;
        }
    }

    function keepSizeLessThanImage(): void {
        if (crop.width > image.width) {
            crop.width = image.width;
            crop.height = image.width;
        }

        if (crop.height > image.height) {
            crop.width = image.height;
            crop.height = image.height;
        }
    }

    function keepCropperInBounds(): void {
        // Left side
        if (crop.x < 0) {
            centerCage.x = Math.round(crop.width / 2 - centerCage.width / 2);
        }

        // Top side
        if (crop.y < 0) {
            centerCage.y = Math.round(crop.height / 2 - centerCage.height / 2);
        }

        // Right side
        if (crop.x + crop.width > image.width) {
            centerCage.x = Math.round(centerCage.width / 2 - crop.width / 2);
        }

        // Bottom side
        if (crop.y + crop.height > image.height) {
            centerCage.y = Math.round(centerCage.height / 2 - crop.height / 2);
        }
    }

    function resizeCrop(mouse: int): void {
        crop.width += mouse;
        crop.height += mouse;

        keepMinimumSize();
        keepSizeLessThanImage();
        keepCropperInBounds();
    }

    function dragHandle(mouseDir: int, resizeAmount: int): void {
        if (cropSheet.isMousePressed) {
            resizeCrop(resizeAmount); // Keep the resize amount small, and not exponential.
        } else {
            handleToCursor = mouseDir;
        }
    }

    function lockHandleToCursor(buttonType: int): void {
        isMousePressed = buttonType === Qt.LeftButton;
    }

    function unlockHandleToCursor(): void {
        isMousePressed = false;
    }

    function getCursorShape(alternateShape: int): int {
        return isMousePressed ? Qt.ClosedHandCursor : alternateShape
    }

    GridLayout {
        Image {
            id: image

            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: Math.round(sourceSize.height / (sourceSize.width / Layout.preferredWidth))
            Layout.preferredWidth: cropSheet.width - Kirigami.Units.largeSpacing * 2
            fillMode: Image.PreserveAspectFit

            Rectangle {
                id: cover
                anchors.fill: image
                opacity: 0.0
                color: "#70000000"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: getCursorShape(Qt.ArrowCursor)
                }
            }

            MultiEffect {
                source: cover
                anchors.fill: cover

                maskEnabled: true
                maskInverted: true
                maskSource: mask
            }

            Item {
                id: mask
                anchors.fill: image
                visible: false
                layer.enabled: true

                Rectangle {
                    border {
                        color: "white"
                        width: 1
                    }
                    height: crop.height
                    radius: width / 2
                    width: crop.width
                    x: crop.x
                    y: crop.y
                }
            }

            // This item surrounds the cropping circle, and keeps it centered and unmoving while performing resizes.
            Item {
                id: centerCage
                visible: false
                width: image.width
                height: image.height
            }

            Rectangle {
                id: crop

                border {
                    color: "white"
                    width: 1
                }
                color: "transparent"
                height: 100
                radius: width / 2
                width: 100

                anchors {
                    verticalCenter: centerCage.verticalCenter
                    horizontalCenter: centerCage.horizontalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: getCursorShape(Qt.OpenHandCursor)

                    drag {
                        target: centerCage
                        maximumX: Math.round(centerCage.width / 2 - crop.width / 2)
                        maximumY: Math.round(centerCage.height / 2 - crop.height / 2)
                        minimumX: Math.round(crop.width / 2 - centerCage.width / 2)
                        minimumY: Math.round(crop.height / 2 - centerCage.height / 2)
                    }

                    onPressed: { cropSheet.isMousePressed = true }
                    onReleased: { cropSheet.isMousePressed = false }
                }
            }

            component Handle : Rectangle {
                color: Kirigami.Theme.highlightColor
                height: 20; width: 20
                radius: width / 2
            }

            component HandleMouseArea : MouseArea {
                property var mouse
                property bool invertResizeAmount: false

                function calcResizeAmount(mousePoint, invert: bool): int {
                    return invert ? handleToCursor - mousePoint : mousePoint - handleToCursor;
                }

                anchors.fill: parent; drag.target: parent; hoverEnabled: true
                cursorShape: getCursorShape(Qt.OpenHandCursor)

                onPositionChanged: (_, m = mouse, i = invertResizeAmount) => { dragHandle(m, calcResizeAmount(m, i)); }
                onPressed: (e) => { lockHandleToCursor(e.button); }
                onReleased: { unlockHandleToCursor(); }
            }

            Handle {
                anchors.left: crop.right
                anchors.verticalCenter: crop.verticalCenter
                transform: Translate { x: -10 }

                HandleMouseArea { mouse: mouseX }
            }

            Handle {
                anchors.right: crop.left
                anchors.verticalCenter: crop.verticalCenter
                transform: Translate { x: 10 }

                HandleMouseArea { mouse: mouseX; invertResizeAmount: true }
            }

            Handle {
                anchors.bottom: crop.top
                anchors.horizontalCenter: crop.horizontalCenter
                transform: Translate { y: 10 }

                HandleMouseArea { mouse: mouseY; invertResizeAmount: true }
            }

            Handle {
                anchors.top: crop.bottom
                anchors.horizontalCenter: crop.horizontalCenter
                transform: Translate { y: -10 }

                HandleMouseArea { mouse: mouseY }
            }
        }
    }

    Component.onCompleted: {
        open()
    }
}
