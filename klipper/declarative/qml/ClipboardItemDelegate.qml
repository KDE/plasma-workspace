/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts 1.1
import Qt5Compat.GraphicalEffects

import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.20 as Kirigami
import org.kde.ksvg 1.0 as KSvg

PlasmaComponents.ItemDelegate {
    id: menuItem

    width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin

    required property var listMargins

    required property var model
    required property int index
    required property var decoration
    required property var uuid
    required property int type

    readonly property alias dragHandler: dragHandler
    property alias mainItem: label.contentItem

    property int maximumNumberOfPreviews: Math.floor(width / (Kirigami.Units.gridUnit * 4 + Kirigami.Units.smallSpacing))
    readonly property real gradientThreshold: (label.width - toolButtonsLoader.width) / label.width
    // Consider tall to be > about 1.5x the default height for purposes of top-aligning
    // the buttons to preserve Fitts' Law when deleting multiple items in a row,
    // or else the top-alignment doesn't look deliberate enough and people will think
    // it's a bug
    readonly property bool isTall: height > Math.round(Kirigami.Units.gridUnit * 2.5)
    readonly property bool shouldUseOverflowButton: Kirigami.Settings.tabletMode || Kirigami.Settings.hasTransientTouchInput

    signal itemSelected()
    signal remove()
    signal edit()
    signal barcode()
    signal triggerAction()

    // the 1.6 comes from ToolButton's default height
    height: Math.max(label.height, Math.round(Kirigami.Units.gridUnit * 1.6)) + 2 * Kirigami.Units.smallSpacing

    enabled: true

    onClicked: menuItem.itemSelected()

    onItemSelected: (menuItem.ListView.view.parent as ClipboardMenu).itemSelected(uuid)
    onRemove: (menuItem.ListView.view.parent as ClipboardMenu).remove(uuid)
    onEdit: (menuItem.ListView.view.parent as ClipboardMenu).edit(model)
    onBarcode: (menuItem.ListView.view.parent as ClipboardMenu).barcode(model.display)
    onTriggerAction: (menuItem.ListView.view.parent as ClipboardMenu).triggerAction(uuid)

    Keys.onEnterPressed: event => Keys.returnPressed(event)
    Keys.onReturnPressed: menuItem.clicked()
    Keys.onDeletePressed: menuItem.remove()
    KeyNavigation.right: toolButtonsLoader.active ? toolButtonsLoader.item.defaultButton : toolButtonsLoader

    ListView.onIsCurrentItemChanged: {
        if (ListView.isCurrentItem) {
            labelMask.source = label // calculate on demand
        }
    }

    Binding {
        target: menuItem.ListView.view
        // don't change currentIndex if it would make listview scroll
        // see https://bugs.kde.org/show_bug.cgi?id=387797
        // this is a workaround till https://bugreports.qt.io/browse/QTBUG-114574 gets fixed
        // which would allow a proper solution
        when: menuItem.hovered && (menuItem.y - menuItem.ListView.view.contentY + menuItem.height + 1 /* border */ < menuItem.ListView.view.height) && (menuItem.y - menuItem.ListView.view.contentY >= 0)
        property: "currentIndex"
        value: menuItem.index
        restoreMode: Binding.RestoreBinding
    }

    DragHandler {
        id: dragHandler
        enabled: !toolButtonsLoader.item?.hovered
        target: null
    }

    // this stuff here is used so we can fade out the text behind the tool buttons
    Item {
        id: labelMaskSource
        anchors.fill: label
        visible: false

        Rectangle {
            anchors.centerIn: parent
            rotation: LayoutMirroring.enabled ? 90 : -90 // you cannot even rotate gradients without Qt5Compat.GraphicalEffects
            width: parent.height
            height: parent.width

            gradient: Gradient {
                GradientStop { position: 0.0; color: "white" }
                GradientStop { position: gradientThreshold - 0.25; color: "white"}
                GradientStop { position: gradientThreshold; color: "transparent"}
                GradientStop { position: 1; color: "transparent"}
            }
        }
    }

    OpacityMask {
        id: labelMask
        anchors.fill: label
        cached: true
        maskSource: labelMaskSource
        visible: !!source && menuItem.ListView.isCurrentItem

        TapHandler {
            enabled: !toolButtonsLoader.item?.hovered // https://bugreports.qt.io/browse/QTBUG-108821
            onTapped: {
                menuItem.clicked() // https://bugreports.qt.io/browse/QTBUG-63395
            }
        }
    }

    QQC.Control {
        id: label
        height: implicitHeight
        visible: !menuItem.ListView.isCurrentItem
        anchors {
            left: parent.left
            leftMargin: Math.ceil(Kirigami.Units.gridUnit / 2) - menuItem.listMargins.left
            right: parent.right
            rightMargin: (menuItem.shouldUseOverflowButton ? toolButtonsLoader.implicitWidth : 0) + toolButtonsLoader.anchors.rightMargin
            verticalCenter: parent.verticalCenter
        }
    }

    Loader {
        id: toolButtonsLoader

        anchors {
            right: parent.right
            rightMargin: Math.ceil(Kirigami.Units.gridUnit / 2) - menuItem.listMargins.right
            verticalCenter: parent.verticalCenter
            // This is here because you can't assign to it in AnchorChanges below
            topMargin: Math.ceil(Kirigami.Units.gridUnit / 2) - menuItem.listMargins.top
        }
        source: "DelegateToolButtons.qml"
        active: menuItem.ListView.view.clipboardMenu.expanded && (menuItem.ListView.isCurrentItem || menuItem.shouldUseOverflowButton)

        // It's not recommended to change anchors via conditional bindings, use AnchorChanges instead.
        // See https://doc.qt.io/qt-5/qtquick-positioning-anchors.html#changing-anchors
        states: [
            State {
                when: menuItem.isTall

                AnchorChanges {
                    target: toolButtonsLoader
                    anchors.top: toolButtonsLoader.parent.top
                    anchors.verticalCenter: undefined
                }
            }
        ]
    }
}
