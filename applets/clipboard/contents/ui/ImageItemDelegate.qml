/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.20 as Kirigami

Item {
    height: childrenRect.height

    Drag.active: dragHandler.active
    Drag.dragType: Drag.Automatic
    Drag.supportedActions: Qt.CopyAction
    Drag.mimeData: {
        "image/png": DecorationRole,
    }

    KQuickControlsAddons.QImageItem {
        id: previewImage

        width: Math.min(Math.round(height * nativeWidth/nativeHeight), parent.width)
        height: Math.min(nativeHeight, Kirigami.Units.gridUnit * 4 + Kirigami.Units.smallSpacing * 2)

        // align left
        // right in RTL
        anchors.left: parent.left

        image: DecorationRole
        smooth: true
        fillMode: Image.PreserveAspectFit
    }
}
