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
import QtQuick.Controls.Private 1.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0

PlasmaComponents.ListItem {
    id: notificationItem
    width: popupFlickable.width

    property int layoutSpacing: units.smallSpacing
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

        NotificationItem {
            id: notification
            width: parent.width

            compact: true
            icon: appIcon
            image: model.image
            summary: model.summary
            configurable: model.configurable && !Settings.isMobile
            // model.actions JS array is implicitly turned into a ListModel which we can assign directly
            actions: model.actions
            created: model.created

            textItem: MouseArea {
                id: contextMouseArea
                height: bodyText.paintedHeight

                acceptedButtons: Qt.RightButton
                preventStealing: true

                onPressed: contextMenu.open(mouse.x, mouse.y)

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

                TextEdit {
                    id: bodyText
                    anchors.fill: parent

                    text: body
                    color: PlasmaCore.ColorScope.textColor
                    selectedTextColor: theme.viewBackgroundColor
                    selectionColor: theme.viewFocusColor
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
                    enabled: !Settings.isMobile
                    selectByMouse: true
                    readOnly: true
                    wrapMode: Text.Wrap
                    textFormat: TextEdit.RichText

                    onLinkActivated: Qt.openUrlExternally(link)
                }
            }

            onClose: {
                if (notificationsModel.count > 1) {
                    removeAnimation.running = true
                } else {
                    closeNotification(model.source)
                    notificationsModel.remove(index)
                }
            }
            onConfigure: {
                plasmoid.expanded = false
                configureNotification(model.appRealName)
            }
            onAction: {
                executeAction(source, actionId)
                actions.clear()
            }
        }

    } //MouseArea

    Component.onCompleted: {
        mainScrollArea.height = mainScrollArea.implicitHeight
    }
    Component.onDestruction: {
        mainScrollArea.height = mainScrollArea.implicitHeight
    }
}
