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
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.kirigami 2.20 as Kirigami
import org.kde.ksvg 1.0 as KSvg

PlasmaComponents.ItemDelegate {
    id: menuItem

    width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin

    required property var listMargins

    required property var model
    required property int index
    required property var decoration
    required property size imageSize
    required property var uuid
    required property int type

    readonly property alias dragHandler: dragHandler
    property alias mainItem: label.contentItem

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
    signal triggerAction(bool automaticallyInvoked)

    // the 1.6 comes from ToolButton's default height
    height: Math.max(label.height, Math.round(Kirigami.Units.gridUnit * 1.6)) + 2 * Kirigami.Units.smallSpacing + (!!(expandButtonLoader.item as PlasmaComponents.ToolButton)?.checked ? toolButtonsLoader.implicitHeight : 0)
    Behavior on height {
        enabled: menuItem.shouldUseOverflowButton
        SmoothedAnimation { // to match the highlight
            id: heightAnimation
            duration: Kirigami.Units.longDuration
            velocity: Kirigami.Units.longDuration
            easing.type: Easing.InOutCubic
        }
    }

    enabled: true

    onClicked: menuItem.itemSelected()

    onItemSelected: (menuItem.ListView.view.parent as ClipboardMenu).itemSelected(uuid)
    onRemove: (menuItem.ListView.view.parent as ClipboardMenu).remove(uuid)
    onEdit: (menuItem.ListView.view.parent as ClipboardMenu).edit(model)
    onBarcode: (menuItem.ListView.view.parent as ClipboardMenu).barcode(model.display)
    onTriggerAction: automaticallyInvoked => (menuItem.ListView.view.parent as ClipboardMenu).triggerAction(uuid, model.display, automaticallyInvoked)

    Accessible.onPressAction: menuItem.itemSelected()
    Keys.onEnterPressed: event => Keys.returnPressed(event)
    Keys.onReturnPressed: menuItem.clicked()
    Keys.onDeletePressed: menuItem.remove()
    KeyNavigation.right: toolButtonsLoader.active ? (toolButtonsLoader.item as DelegateToolButtons).defaultButton : toolButtonsLoader
    KeyNavigation.left: null

    ListView.onIsCurrentItemChanged: {
        if (ListView.isCurrentItem) {
            labelMask.source = label // calculate on demand
        }
    }
    ListView.onPooled: if (expandButtonLoader.active) {
        expandButtonLoader.item.checked = false;
    }

    onHoveredChanged: if (hovered) ListView.view.currentIndex = index

    // The highlight at a listview level is disabled as by default Listview then scrolls to keep that index in view
    // see https://bugs.kde.org/show_bug.cgi?id=387797
    // Instead place highlight in each delegate and we don't keep the highlight in view
    background: PlasmaExtras.Highlight {
        anchors.fill: parent
        hovered: menuItem.ListView.isCurrentItem
    }

    DragHandler {
        id: dragHandler
        enabled: !(toolButtonsLoader.item as DelegateToolButtons)?.hovered
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
                GradientStop { position: menuItem.gradientThreshold - 0.25; color: "white"}
                GradientStop { position: menuItem.gradientThreshold; color: "transparent"}
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
            enabled: !(toolButtonsLoader.item as DelegateToolButtons)?.hovered // https://bugreports.qt.io/browse/QTBUG-108821
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
            rightMargin: expandButtonLoader.implicitWidth + expandButtonLoader.anchors.rightMargin
                          + ((menuItem.model?.starred ?? false) ? starSlot.width + Kirigami.Units.smallSpacing : 0)
            verticalCenter: parent.verticalCenter
        }
        states: [
            State {
                when: toolButtonsLoader.active
                AnchorChanges {
                    target: label
                    anchors.verticalCenter: undefined
                }
            }
        ]
    }


    // Reserve the same space and alignment as the real star ToolButton
    Item {
        id: starSlot
        anchors.right: parent.right
        anchors.rightMargin: expandButtonLoader.implicitWidth + expandButtonLoader.anchors.rightMargin
        anchors.verticalCenter: parent.verticalCenter
        width: menuItem.ListView.view.clipboardMenu.starMetricsImplicitWidth
        height: menuItem.ListView.view.clipboardMenu.starMetricsImplicitHeight
        visible: (menuItem.model?.starred ?? false) && !toolButtonsLoader.active

        Kirigami.Icon {
            id: passiveStar
            anchors.centerIn: parent
            // Match toolbutton icon extent
            width: Kirigami.Units.iconSizes.smallMedium
            height: width
            source: "starred-symbolic"
        }
    }

    Loader {
        id: expandButtonLoader

        anchors {
            right: parent.right
            rightMargin: Math.ceil(Kirigami.Units.gridUnit / 2) - menuItem.listMargins.right
            verticalCenter: parent.verticalCenter
            // This is here because you can't assign to it in AnchorChanges below
            topMargin: Math.ceil(Kirigami.Units.gridUnit / 2) - menuItem.listMargins.top
        }
        active: menuItem.shouldUseOverflowButton
        sourceComponent: PlasmaComponents.ToolButton {
            id: expandButton
            visible: menuItem.shouldUseOverflowButton
            checkable: true
            display: PlasmaComponents.AbstractButton.IconOnly
            text: checked ? i18ndc("libplasma6", "@action:button", "Collapse") : i18ndc("libplasma6", "@action:button", "Expand")
            icon.name: checked ? "collapse" : "expand"
            PlasmaComponents.ToolTip.text: text
            PlasmaComponents.ToolTip.delay: Kirigami.Units.toolTipDelay
            PlasmaComponents.ToolTip.visible: pressed

            Connections {
                target: menuItem.ListView.view.clipboardMenu
                function onExpandedChanged() {
                    expandButton.checked = false;
                }
            }
        }
        states: [
            State {
                when: toolButtonsLoader.active
                AnchorChanges {
                    target: expandButtonLoader
                    anchors.verticalCenter: undefined
                }
            }
        ]
    }

    Loader {
        id: toolButtonsLoader

        anchors {
            right: parent.right
            rightMargin: expandButtonLoader.anchors.rightMargin
            verticalCenter: parent.verticalCenter
            // This is here because you can't assign to it in AnchorChanges below
            topMargin: expandButtonLoader.anchors.topMargin
        }
        width: menuItem.shouldUseOverflowButton ? label.width + label.anchors.rightMargin - label.anchors.leftMargin : implicitWidth
        sourceComponent: DelegateToolButtons {
            menuItem: menuItem
            shouldUseOverflowButton: menuItem.shouldUseOverflowButton
        }
        active: (menuItem.ListView.isCurrentItem && !menuItem.shouldUseOverflowButton)
            || (menuItem.shouldUseOverflowButton && (!!(expandButtonLoader.item as PlasmaComponents.ToolButton)?.checked || opacity > 0))
        opacity: !expandButtonLoader.active || (expandButtonLoader.item as PlasmaComponents.ToolButton).checked ? 1 : 0

        // It's not recommended to change anchors via conditional bindings, use AnchorChanges instead.
        // See https://doc.qt.io/qt-5/qtquick-positioning-anchors.html#changing-anchors
        states: [
            State {
                when: menuItem.shouldUseOverflowButton
                AnchorChanges {
                    target: toolButtonsLoader
                    anchors.top: label.bottom
                    anchors.verticalCenter: undefined
                }
            }
        ]

        Behavior on opacity {
            enabled: menuItem.shouldUseOverflowButton
            SmoothedAnimation { // to match the highlight
                id: expandedItemOpacityFade
                duration: heightAnimation.duration
                // velocity is divided by the default speed, as we're in the range 0-1
                velocity: heightAnimation.velocity / 200
                easing.type: Easing.InOutCubic
            }
        }
    }
}
