/*
 *   Copyright 2014 Martin Klapetek <mklapetek@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0

PlasmaCore.Dialog {
    id: notificationPopup

    location: PlasmaCore.Types.Floating
    type: PlasmaCore.Dialog.Dock
    flags: Qt.WindowStaysOnTopHint

    property var notificationProperties

    onVisibleChanged: {
        notificationTimer.running = visible;
    }

    function populatePopup(notification)
    {
        notificationProperties = notification
        actionsRepeater.model = notification.actions
        notificationTimer.interval = notification.expireTimeout
        notificationTimer.restart()
        titleLabel.text = notification.summary
        bodyLabel.text = notification.body
        appIconItem.icon = notification.appIcon
        imageItem.image = notification.image
    }

    Behavior on y {
        NumberAnimation {
            duration: units.longDuration
            easing.type: Easing.OutQuad
        }
    }

    mainItem: MouseEventListener {
        id: root
        height: Math.max(actionsColumn.height + closeButton.height + (3 * units.smallSpacing), (5 * units.gridUnit)) //5 * units.gridUnit
        width: Math.round(23 * units.gridUnit)

        state: "controlsHidden"
        hoverEnabled: true

        onContainsMouseChanged: {
            if (containsMouse) {
                root.state = "controlsShown"
                notificationTimer.running = false
            } else {
                root.state = "controlsHidden"
                notificationTimer.restart()
            }
        }

        onClicked: {
            closeNotification(notificationProperties.source)
            notificationPopup.hide()
        }

        QIconItem {
            id: appIconItem
            height: units.iconSizes.large
            width: height
            visible: !imageItem.visible
            anchors {
                left: parent.left
                top: parent.top
                margins: units.smallSpacing * 2
            }
        }

        QImageItem {
            id: imageItem
            anchors.fill: appIconItem
            smooth: true
            fillMode: Image.PreserveAspectFit
            visible: nativeWidth > 0
        }

        PlasmaExtras.Heading {
            id: titleLabel
            level: 4
            height: paintedHeight
            elide: Text.ElideRight
            anchors {
                left: appIconItem.right
                //right: closeButton.left
                top: parent.top
                right: actionsRepeater.count < 3 ? closeButton.left : actionsColumn.left
                margins: units.smallSpacing * 2
            }
            onLinkActivated: Qt.openUrlExternally(link)
        }

        /*
         * this extra item is for clip the overflowed body text
         * maximumLineCount cannot control the behavior of rich text,
         * so manual clip is required.
         */
//         Item {
//             id: bodyLabelClip
//             clip: true
//             height: Math.min(parent.height - (titleLabel.height+titleLabel.y), bodyLabel.height)
//             property bool tallText: bodyLabelClip.height >= (bodyLabelClip.parent.height - (titleLabel.height+titleLabel.y)*2)
//             anchors {
//                 top: tallText ? titleLabel.bottom : undefined
//                 verticalCenter: tallText ? undefined : parent.verticalCenter
//                 left: appIconItem.right
// //                 right: actionsColumn.left
//                 leftMargin: 6
//                 rightMargin: 6
//             }
            PlasmaComponents.Label {
                id: bodyLabel
                color: theme.textColor
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                maximumLineCount: 2
                verticalAlignment: Text.AlignTop
                onLinkActivated: Qt.openUrlExternally(link)
                anchors {
                    left: appIconItem.right
                    right: (!notificationProperties || notificationProperties.actions.length == 0) ? parent.right : actionsColumn.left
                    top: titleLabel.bottom
                    bottom: parent.bottom
                    margins: units.smallSpacing * 2
                }
            }
//         }

            PlasmaComponents.ToolButton {
                id: closeButton
//                 opacity: 0
                iconSource: "window-close"
                width: units.iconSizes.smallMedium
                height: width
                flat: false
                anchors {
                    right: parent.right
                    top: parent.top
                    topMargin: units.smallSpacing
                    rightMargin: units.smallSpacing
                }
                onClicked: {
                    closeNotification(notificationProperties.source)
                    notificationPopup.hide()
                }
            }

            Column {
                id: actionsColumn
                spacing: units.smallSpacing
                anchors {
                    bottom: parent.bottom
                    right: parent.right
                    rightMargin: units.smallSpacing
                    bottomMargin: units.smallSpacing
                }
                Repeater {
                    id: actionsRepeater
                    model: []
                    PlasmaComponents.Button {
                        text: modelData.text
                        width: theme.mSize(theme.defaultFont).width * 12
                        onClicked: {
                            executeAction(notificationProperties.source, modelData.id)
                            actionsColumn.visible = false
                            notificationPopup.hide()
                        }
                    }
                }
            }

        Timer {
            id: notificationTimer
            repeat: false
            running: false
            onTriggered: {
                if (!notificationProperties.isPersistent) {
                    closeNotification(notificationProperties.source)
                }
                notificationPopup.hide()
            }
        }

        states: [
        State {
            name: "controlsShown"
//             PropertyChanges {
//                 target: closeButton
//                 opacity: 1
//             }
//             PropertyChanges {
//                 target: settingsButton
//                 opacity: 1
//             }
        },
        State {
            name: "controlsHidden"
//             PropertyChanges {
//                 target: closeButton
//                 opacity: 0
//             }
//             PropertyChanges {
//                 target: settingsButton
//                 opacity: 0
//             }
        }
        ]
        transitions: [
        Transition {
            NumberAnimation {
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: units.longDuration
            }
        }
        ]

    }

}
