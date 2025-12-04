/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kirigami as Kirigami

ClipboardItemDelegate {
    id: menuItem
    mainItem: Item {
        implicitHeight: childrenRect.height

        Drag.active: menuItem.dragHandler.active
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction
        Drag.mimeData: {
            "text/uri-list": [previewImage.source],
        }

        Image {
            id: previewImage

            width: Math.min(Math.round(height * menuItem.imageSize.width / menuItem.imageSize.height), parent.width)
            height: sourceSize.height

            // align left
            // right in RTL
            anchors.left: parent.left

            cache: false
            source: menuItem.decoration
            sourceSize.height: Math.min(menuItem.imageSize.height, Kirigami.Units.gridUnit * 4 + Kirigami.Units.smallSpacing * 2)
            smooth: true
            fillMode: Image.PreserveAspectFit
        }
    }
}
