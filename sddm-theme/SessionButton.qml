/*
*   Copyright 2016 David Edmundson <davidedmundson@kde.org>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License as
*   published by the Free Software Foundation; either version 2 or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details
*
*   You should have received a copy of the GNU Library General Public
*   License along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

import QtQuick 2.6

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents

import QtQuick.Controls 2.5 as QQC2
import QtGraphicalEffects 1.0

PlasmaComponents.ToolButton {
    id: sessionButton
    property int currentIndex: -1

    visible: sessionMenu.count > 1

    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Desktop Session: %1", instantiator.objectAt(currentIndex).name || "Error")

    font.pointSize: config.fontSize

    Component.onCompleted: {
        currentIndex = sessionModel.lastIndex
    }

    onClicked: {
        sessionMenu.popup(x, y)
    }

    QQC2.Menu {
        id: sessionMenu

        property int largestWidth: 9999

        Component.onCompleted: {
            var trueWidth = 0;
            for (var i = 0; i < sessionMenu.count; i++) {
                trueWidth = Math.max(trueWidth, sessionMenu.itemAt(i).textWidth)
            }
            sessionMenu.largestWidth = trueWidth
        }

        background: Rectangle {
            implicitHeight: 40
            implicitWidth: sessionMenu.largestWidth > sessionButton.implicitWidth ? sessionMenu.largestWidth : sessionButton.implicitWidth
            color: PlasmaCore.ColorScope.backgroundColor
            property color borderColor: PlasmaCore.Colorscope.textColor
            border.color: Qt.rgba(borderColor.r, borderColor.g, borderColor.b, 0.3)
            border.width: 1
            layer.enabled: true
            layer.effect: DropShadow {
                transparentBorder: true
                radius: 8
                samples: 8
                horizontalOffset: 0
                verticalOffset: 2
                color: Qt.rgba(0, 0, 0, 0.3)
            }
        }

        Instantiator {
            id: instantiator
            model: sessionModel
            onObjectAdded: sessionMenu.addItem( object )
            onObjectRemoved: sessionMenu.removeItem( object )
            delegate: QQC2.MenuItem {
                id: menuItem
                property string name: model.name

                property real textWidth: text.contentWidth + 20
                implicitWidth: text.contentWidth + 20
                implicitHeight: Math.round(text.contentHeight * 1.6)

                contentItem: QQC2.Label {
                    id: text
                    font.pointSize: config.fontSize
                    text: model.name
                    color: menuItem.highlighted ? PlasmaCore.ColorScope.highlightedTextColor : PlasmaCore.ColorScope.textColor
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: PlasmaCore.ColorScope.highlightColor
                    opacity: menuItem.highlighted ? 1 : 0
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onContainsMouseChanged: menuItem.highlighted = containsMouse
                        onClicked: {
                            sessionMenu.dismiss()
                            sessionButton.currentIndex = model.index
                        }
                    }
                }
            }
        }

        enter: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 150
            }
        }
        exit: Transition {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
            }
        }
    }
}
