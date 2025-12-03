/*
    SPDX-FileCopyrightText: 2020 Konrad Materka <materka@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick

Loader {
    required property int index
    required property var model

    z: x + 1 // always be above what it's on top of, even for x==0

    readonly property url __url: {
        if (model.itemType === "Plasmoid" && model.hasApplet) {
            return Qt.resolvedUrl("PlasmoidItem.qml")
        } else if (model.itemType === "StatusNotifier") {
            return Qt.resolvedUrl("StatusNotifierItem.qml")
        }
        console.warn("SystemTray ItemLoader: Invalid state, cannot determine source!")
        return ""
    }

    // Avoid relying on context properties using initialProperties with bindings.
    // See https://bugreports.qt.io/browse/QTBUG-125070
    on__UrlChanged: {
        setSource(__url, {
            index: Qt.binding(() => index),
            model: Qt.binding(() => model),
        });
    }
}
