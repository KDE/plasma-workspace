/*
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Shapes 1.3
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.15 as Kirigami


QQC2.Slider {
    id: control
    implicitHeight: 5

    readonly property bool rtl: (Qt.application as Application).layoutDirection

    property string pointerLabel
    property string handleToolTip
    property bool pointerOnBottom: true
    property bool interactive: true
    property bool overlapping: false

    property real minDrag: 0.0
    property real maxDrag: 1.0

    LayoutMirroring.enabled: rtl

    MouseArea {
        // absorb clicks on slider
        anchors.fill: parent
        onClicked: { return true }
    }

    function changeValue(value) {
        handle.changeValue(value)
    }

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 200
        implicitHeight: 4
        opacity: 0
        width: control.availableWidth
        height: implicitHeight
    }

    signal userChangedValue(value: int)

    function pos(value) {
        return control.rtl ? 1-(value/(to-from)) : (value/(to-from))
    }

    function dragLimit(lim) {
        return control.rtl ? 1-lim : lim
    }

    handle: Item {
        y: (pointerOnBottom ? 10 : -10)

        property alias lblWidth: lbl.width

        x: control.leftPadding + control.visualPosition * (control.availableWidth) - pointer.width/2

        function changeValue(value) {
            x = control.leftPadding + pos(value) * (control.availableWidth) - pointer.width/2
        }

        property real minDragX: control.leftPadding - (pointer.width / 2) + control.minDrag*control.availableWidth
        property real maxDragX: control.leftPadding + control.maxDrag*control.availableWidth - pointer.width/2

        onXChanged: {
            var v = control.valueAt((control.leftPadding + x + pointer.width/2)/(control.leftPadding + control.availableWidth));
            if (control.rtl) {
                v = to - v;
            }
            control.userChangedValue(v)
        }

        Kirigami.Icon {
            id: pointer
            source: pointerOnBottom ? "draw-triangle3" : "draw-triangle4"
            width: 10
            height: 10
            QQC2.ToolTip {
                parent: pointer
                text: handleToolTip
                visible: pointerMouseArea.containsMouse && !pointerMouseArea.drag.active
            }
            MouseArea {
                id: pointerMouseArea
                hoverEnabled: true
                anchors.fill: parent
                drag.target: control.interactive ? handle : undefined
                drag.axis: Drag.XAxis
                drag.minimumX: handle.minDragX
                drag.maximumX: handle.maxDragX
            }
        }

        QQC2.Label {
            id: lbl
            text: pointerLabel
            anchors.top: pointerOnBottom ? pointer.bottom : undefined
            anchors.bottom: pointerOnBottom ? undefined : pointer.top
            x: -width/2 + pointer.width/2
            Binding {
                when: control.overlapping
                target: lbl
                property: pointerOnBottom ? "anchors.topMargin" : "anchors.bottomMargin"
                value: tm.height
                restoreMode: Binding.RestoreBindingOrValue
            }
            Behavior on anchors.bottomMargin {
                NumberAnimation { easing.type: Easing.InOutCubic; duration: Kirigami.Units.shortDuration }
            }
            Behavior on anchors.topMargin {
                NumberAnimation { easing.type: Easing.InOutCubic; duration: Kirigami.Units.shortDuration }
            }
            TextMetrics {
                id: tm
                font: lbl.font
                text: lbl.text
            }
            QQC2.ToolTip {
                parent: lbl
                text: handleToolTip
                visible: lblMouseArea.containsMouse && !lblMouseArea.drag.active
            }
            MouseArea {
                id: lblMouseArea
                hoverEnabled: true
                anchors.fill: parent
                drag.target: control.interactive ? handle : undefined
                drag.axis: Drag.XAxis
                drag.minimumX: handle.minDragX
                drag.maximumX: handle.maxDragX
            }
        }
    }
}
