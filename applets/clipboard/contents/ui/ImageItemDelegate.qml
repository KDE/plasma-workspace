/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    height: childrenRect.height

    KQuickControlsAddons.QPixmapItem {
        id: previewPixmap

        width: Math.min(Math.round(height * nativeWidth/nativeHeight), parent.width)
        height: Math.min(nativeHeight, PlasmaCore.Units.gridUnit * 4 + PlasmaCore.Units.smallSpacing * 2)

        // align left
        // right in RTL
        anchors.left: parent.left

        pixmap: DecorationRole
        smooth: true
        fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectFit
    }
}
