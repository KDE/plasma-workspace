/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Window
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

PlasmaExtras.Representation {
    id: root
    readonly property var appletInterface: devicenotifier

    property bool spontaneousOpen: false

    property alias model: notifierDialog.model

    Layout.minimumWidth: Kirigami.Units.gridUnit * 18
    Layout.minimumHeight: Kirigami.Units.gridUnit * 18
    Layout.maximumWidth: Kirigami.Units.gridUnit * 80
    Layout.maximumHeight: Kirigami.Units.gridUnit * 40

    focus: true
    collapseMarginsHint: true

    header: PlasmaExtras.PlasmoidHeading {
        visible: !(Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading) && devicenotifier.mountedRemovables > 1
        PlasmaComponents3.ToolButton {
            id: unmountAll
            anchors.right: parent.right
            visible: devicenotifier.mountedRemovables > 1;

            icon.name: "media-eject"
            text: i18n("Remove All")
            Accessible.description: i18n("Click to safely remove all devices")

            PlasmaComponents3.ToolTip {
                text: parent.Accessible.description
            }
        }
    }

    MouseArea {
        id: fullRepMouseArea
        hoverEnabled: true
    }

    // this item is reparented to a delegate that is showing a message to draw focus to it
    PlasmaExtras.Highlight {
        id: messageHighlight
        visible: false

        OpacityAnimator {
            id: messageHighlightAnimator
            target: messageHighlight
            from: 1
            to: 0
            duration: Kirigami.Units.veryLongDuration * 8
            easing.type: Easing.InOutQuad
            Component.onCompleted: devicenotifier.isMessageHighlightAnimatorRunning = Qt.binding(() => running);
        }

        Connections {
            target: filterModel
            function onLastUdiChanged() {
                if (filterModel.lastUdi === "") {
                    messageHighlightAnimator.stop()
                    messageHighlight.visible = false
                }
            }
        }

        function highlight(item) {
            parent = item
            width = Qt.binding(function() { return item.width })
            height = Qt.binding(function() { return item.height })
            opacity = 1 // Animator is threaded so the old opacity might be visible for a frame or two
            visible = true
            messageHighlightAnimator.start()
        }
    }

    PlasmaComponents3.ScrollView {
        id: scrollView

        anchors.fill: parent
        contentWidth: availableWidth - contentItem.leftMargin - contentItem.rightMargin

        focus: true

        contentItem: ListView {
            id: notifierDialog
            focus: true

            model: filterModel

            delegate: DeviceItem {
            }

            highlight: PlasmaExtras.Highlight
            {
            }
            highlightMoveDuration: Kirigami.Units.shortDuration
            highlightResizeDuration: Kirigami.Units.shortDuration

            topMargin: Kirigami.Units.largeSpacing
            bottomMargin: Kirigami.Units.largeSpacing
            leftMargin: Kirigami.Units.largeSpacing
            rightMargin: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            currentIndex: devicenotifier.currentIndex

            //this is needed to make SectionScroller actually work
            //acceptable since one doesn't have a billion of devices
            cacheBuffer: 1000

            KeyNavigation.backtab: root.KeyNavigation.backtab
            KeyNavigation.up: root.KeyNavigation.up

            section {
                property: "deviceType"
                delegate: Item {
                    height: Math.floor(childrenRect.height)
                    width: notifierDialog.width - (scrollView.PlasmaComponents3.ScrollBar.vertical.visible ? Kirigami.Units.largeSpacing * 2 : 0)
                    Kirigami.Heading {
                        level: 3
                        opacity: 0.6
                        text: section
                        textFormat: Text.PlainText
                    }
                }
            }

            Loader {
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.gridUnit * 4)

                active: notifierDialog.count === 0 && !messageHighlightAnimator.running
                visible: active
                asynchronous: true

                sourceComponent: PlasmaExtras.PlaceholderMessage
                {
                    width: parent.width
                    iconName: "drive-removable-media-symbolic"
                    text: Plasmoid.configuration.removableDevices ? i18n("No removable devices attached") : i18n("No disks available")
                }
            }
        }
    }
}
