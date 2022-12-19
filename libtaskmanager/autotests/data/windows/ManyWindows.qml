/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import QtQuick.Window 2.15

QtObject {
    id: root

    /**
     * A list containing maps of initial properties
     * like [QVariantMap, QVariantMap, ...]
     */
    required property var windowInitialProperties
    property var windowList: []

    property Component windowComponent: Component {
        Window {
            width: 320
            height: 240

            visible: true
        }
    }

    Component.onCompleted: {
        if (!windowInitialProperties.length) {
            console.error("Invalid initial properties!")
            return;
        }

        // Create new windows based on given initial properties
        windowInitialProperties.forEach(props => {
            windowList.push(windowComponent.createObject(root, props));
        });
    }
}
