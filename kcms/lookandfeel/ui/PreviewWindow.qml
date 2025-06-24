/*
    SPDX-FileCopyrightText: 2018 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Controls as QtControls
import QtQuick.Window

Window {
    id: previewWindow
    color: Qt.rgba(0, 0, 0, 0.7)
    MouseArea {
        anchors.fill: parent
        Image {
            id: previewImage
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            width: Math.min(parent.width, sourceSize.width)
            height: Math.min(parent.height, sourceSize.height)
        }
        onClicked: previewWindow.close()
        QtControls.ToolButton {
            anchors {
                top: parent.top
                right: parent.right
            }
            icon.name: "window-close"
            onClicked: previewWindow.close()
        }
        Shortcut {
            onActivated: previewWindow.close()
            sequence: "Esc"
        }
    }
    function show(path: string): void {
        previewImage.source = "file:/" + path;
        showFullScreen();
    }
}
