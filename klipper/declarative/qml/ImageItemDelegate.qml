/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick 2.15

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.kirigami 2.20 as Kirigami

ClipboardItemDelegate {
    id: menuItem
    mainItem: Item {
        implicitHeight: childrenRect.height

        Drag.active: menuItem.dragHandler.active
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction
        Drag.mimeData: {
            "image/png": menuItem.decoration,
        }

        KQuickControlsAddons.QImageItem {
            id: previewImage

            width: Math.min(Math.round(height * nativeWidth/nativeHeight), parent.width)
            height: Math.min(nativeHeight, Kirigami.Units.gridUnit * 4 + Kirigami.Units.smallSpacing * 2)

            // align left
            // right in RTL
            anchors.left: parent.left

            image: menuItem.decoration
            smooth: true
            fillMode: Image.PreserveAspectFit
        }
    }
}
