/*
 *   Copyright 2011 Marco Martin <notmart@gmail.com>
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
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0

PlasmaComponents.ListItem {
    id: notificationItem
    width: popupFlickable.width

    property int layoutSpacing: 4
    property int toolIconSize: units.iconSizes.smallMedium

    opacity: 1-Math.abs(x)/width

    Timer {
        interval: 10*60*1000
        repeat: false
        running: !idleTimeSource.idle
        onTriggered: {
            if (!notificationsModel.inserting)
                notificationsModel.remove(index)
        }
    }

    MouseArea {
        width: parent.width
        height: childrenRect.height

        drag {
            target: notificationItem
            axis: Drag.XAxis
            //kind of an hack over Column being too smart
            minimumX: -parent.width + 1
            maximumX: parent.width - 1
        }
        onReleased: {
            if (notificationItem.x < -notificationItem.width/2) {
                removeAnimation.exitFromRight = false
                removeAnimation.running = true
            } else if (notificationItem.x > notificationItem.width/2 ) {
                removeAnimation.exitFromRight = true
                removeAnimation.running = true
            } else {
                resetAnimation.running = true
            }
        }

        SequentialAnimation {
            id: removeAnimation
            property bool exitFromRight: true
            NumberAnimation {
                target: notificationItem
                properties: "x"
                to: removeAnimation.exitFromRight ? notificationItem.width-1 : 1-notificationItem.width
                duration: units.longDuration
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                target: notificationItem
                properties: "height"
                to: 0
                duration: units.longDuration
                easing.type: Easing.InOutQuad
            }
            ScriptAction {
                script: notificationsModel.remove(index)
            }
        }

        SequentialAnimation {
            id: resetAnimation
            NumberAnimation {
                target: notificationItem
                properties: "x"
                to: 0
                duration: units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
        Column {
            width: parent.width
            spacing: notificationItem.layoutSpacing

            Item {
                width: parent.width
                height: summaryLabel.height

                PlasmaComponents.Label {
                    id: summaryLabel
                    anchors {
                        left: parent.left
                        right: parent.right
                        leftMargin: closeButton.width
                        rightMargin: settingsButton.visible ? settingsButton.width + closeButton.width : closeButton.width
                    }
                    height: paintedHeight

                    text: summary
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    onLinkActivated: Qt.openUrlExternally(link)
                }

                PlasmaComponents.ToolButton {
                    id: closeButton
                    anchors {
                        top: parent.top
                        right: parent.right
                    }
                    width: notificationItem.toolIconSize
                    height: width

                    iconSource: "window-close"

                    onClicked: {
                        if (notificationsModel.count > 1) {
                            removeAnimation.running = true
                        } else {
                            closeNotification(model.source)
                            notificationsModel.remove(index)
                        }
                    }
                }

                PlasmaComponents.ToolButton {
                    id: settingsButton
                    anchors {
                        top: parent.top
                        right: closeButton.left
                        rightMargin: 5
                    }
                    width: notificationItem.toolIconSize
                    height: width

                    iconSource: "configure"
                    visible: model.configurable

                    onClicked: {
                        plasmoid.hidePopup()
                        configureNotification(model.appRealName)
                    }
                }
            }

            Item {
                height: childrenRect.height
                width: parent.width

                QIconItem {
                    id: appIconItem
                    anchors {
                        left: parent.left
                        verticalCenter: parent.verticalCenter
                    }
                    width: units.iconSizes.large
                    height: units.iconSizes.large

                    icon: appIcon
                    visible: !imageItem.visible
                }

                QImageItem {
                    id: imageItem
                    anchors.fill: appIconItem

                    image: image
                    smooth: true
                    visible: nativeWidth > 0
                }

                PlasmaComponents.ContextMenu {
                    id: contextMenu
                    visualParent: contextMouseArea

                    PlasmaComponents.MenuItem {
                        text: i18n("Copy")
                        onClicked: bodyText.copy()
                    }

                    PlasmaComponents.MenuItem {
                        text: i18n("Select All")
                        onClicked: bodyText.selectAll()
                    }
                }

                MouseArea {
                    id: contextMouseArea
                    anchors {
                        left: appIconItem.right
                        right: actionsColumn.left
                        verticalCenter: parent.verticalCenter
                        leftMargin: 6
                        rightMargin: 6
                    }
                    height: bodyText.paintedHeight

                    acceptedButtons: Qt.RightButton
                    preventStealing: true

                    onPressed: contextMenu.open(mouse.x, mouse.y)

                    TextEdit {
                        id: bodyText
                        anchors.fill: parent

                        text: body
                        color: theme.textColor
                        font.capitalization: theme.defaultFont.capitalization
                        font.family: theme.defaultFont.family
                        font.italic: theme.defaultFont.italic
                        font.letterSpacing: theme.defaultFont.letterSpacing
                        font.pointSize: theme.defaultFont.pointSize
                        font.strikeout: theme.defaultFont.strikeout
                        font.underline: theme.defaultFont.underline
                        font.weight: theme.defaultFont.weight
                        font.wordSpacing: theme.defaultFont.wordSpacing
                        renderType: Text.NativeRendering
                        selectByMouse: true
                        readOnly: true
                        wrapMode: Text.Wrap
                        textFormat: TextEdit.RichText

                        onLinkActivated: Qt.openUrlExternally(link)
                    }
                }

                Column {
                    id: actionsColumn
                    anchors {
                        right: parent.right
                        rightMargin: 6
                        verticalCenter: parent.verticalCenter
                    }

                    spacing: notificationItem.layoutSpacing

                    Repeater {
                        model: actions

                        PlasmaComponents.Button {
                            width: theme.mSize(theme.defaultFont).width * 8
                            height: theme.mSize(theme.defaultFont).width * 2

                            text: model.text
                            onClicked: {
                                executeAction(source, model.id)
                                actionsColumn.visible = false
                            }
                        } // Button
                    } // Repeater
                } // Column
            } // Item
        } // Column
    } //MouseArea

    Component.onCompleted: {
        mainScrollArea.height = mainScrollArea.implicitHeight
    }
    Component.onDestruction: {
        mainScrollArea.height = mainScrollArea.implicitHeight
    }
}
