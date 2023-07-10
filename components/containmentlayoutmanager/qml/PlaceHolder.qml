/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.12

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.ksvg 1.0 as KSvg
import org.kde.plasma.private.containmentlayoutmanager 1.0 as ContainmentLayoutManager
import org.kde.kirigami 2.20 as Kirigami

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
