/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: detector

    /**
     * The scope of the detector where mouse movement is tracked
     */
    required property Item target

    /**
     * Intercepted enter position
     */
    property var interceptedPosition: null

    /**
     * Whether mouse is moved since last reset
     */
    property bool mouseMoved: false

    readonly property bool __targetHasFlickable: detector.target.hasOwnProperty("contentItem") && detector.target.contentItem instanceof Flickable

    /**
     * Resets the tracker state to start tracking in the specific scope
     */
    function reset() {
        detector.interceptedPosition = null;
        detector.mouseMoved = false;
    }

    Connections {
        enabled: detector.mouseMoved && detector.__targetHasFlickable
        target: detector.__targetHasFlickable ? detector.target.contentItem : null
        function onContentYChanged() {
            mouseMovementTracker.reset();
        }
    }

    Connections {
        enabled: hoverHandler.enabled && detector.interceptedPosition === null
        target: hoverHandler
        function onPointChanged() {
            detector.interceptedPosition = hoverHandler.point.position
        }
    }

    Connections {
        enabled: hoverHandler.enabled && detector.interceptedPosition && !detector.mouseMoved
        target: hoverHandler
        function onPointChanged() {
            if (hoverHandler.point.position === detector.interceptedPosition) {
                return;
            }
            detector.mouseMoved = true
        }
    }

    HoverHandler {
        id: hoverHandler
        enabled: detector.enabled && (!detector.interceptedPosition || !detector.mouseMoved)
        parent: detector.target
        target: detector.target
    }
}
