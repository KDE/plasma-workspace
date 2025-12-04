/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick

import org.kde.ksvg as KSvg
import org.kde.plasma.private.containmentlayoutmanager as ContainmentLayoutManager
import org.kde.kirigami as Kirigami

ContainmentLayoutManager.ItemContainer {
    enabled: false
    KSvg.FrameSvgItem {
        anchors.fill:parent
        imagePath: "widgets/viewitem"
        prefix: "hover"
        opacity: 0.5
    }
    Behavior on opacity {
        NumberAnimation {
            duration: Kirigami.Units.longDuration
            easing.type: Easing.InOutQuad
        }
    }
}
