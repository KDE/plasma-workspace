/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
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

import QtQuick 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.draganddrop 2.0
import org.kde.qtextracomponents 2.0

PlasmaCore.FrameSvgItem {
    id: background
    width: list.delegateWidth
    height: list.delegateHeight

    property variant icon: decoration
    property string title: name
    property string description: model.description
    property string author: model.author
    property string email: model.email
    property string license: model.license
    property string pluginName: model.pluginName
    property bool local: model.local

    ListView.onRemove: SequentialAnimation {
        PropertyAction {
            target: background
            property: "ListView.delayRemove"
            value: true
        }
        NumberAnimation {
            target: background
            property: widgetExplorer.orientation == Qt.Horizontal ? "y" : "x"
            to: widgetExplorer.orientation == Qt.Horizontal ? list.delegateHeight : list.delegateWidth
            duration: 150
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: background
            property: widgetExplorer.orientation == Qt.Horizontal ? "y" : "x"
            to: widgetExplorer.orientation == Qt.Horizontal ? list.delegateHeight : list.delegateWidth
            duration: 150
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: background
            property: widgetExplorer.orientation == Qt.Horizontal ? "width" : "height"
            from: widgetExplorer.orientation == Qt.Horizontal ? list.delegateWidth : list.delegateHeight
            to: 0
            duration: 150
            easing.type: Easing.InOutQuad
        }
        PropertyAction {
            target: background
            property: "ListView.delayRemove"
            value: false
        }
    }

    ListView.onAdd: SequentialAnimation {
        PropertyAction {
            target: background
            property: "y"
            value: widgetExplorer.orientation == Qt.Horizontal ? -list.delegateHeight : -list.delegateWidth
        }
        NumberAnimation {
            target: background
            property: widgetExplorer.orientation == Qt.Horizontal ? "width" : "height"
            from: 0
            to: widgetExplorer.orientation == Qt.Horizontal ? list.delegateWidth : list.delegateHeight
            duration: 150
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: background
            property: widgetExplorer.orientation == Qt.Horizontal ? "y" : "x"
            to: 0
            duration: 150
            easing.type: Easing.InOutQuad
        }
    }


    imagePath: "widgets/viewitem"
    prefix: "normal"

    DragArea {
        anchors.fill: parent
        supportedActions: Qt.MoveAction | Qt.LinkAction
        onDragStarted: tooltipDialog.visible = false
        delegateImage: background.icon
        mimeData {
            source: parent
        }
        Component.onCompleted: mimeData.setData("text/x-plasmoidservicename", pluginName)

        QIconItem {
                id: iconWidget
                anchors.verticalCenter: parent.verticalCenter
                x: y
                width: theme.hugeIconSize
                height: width
                icon: background.icon
            }
        Column {
            anchors {
                left: iconWidget.right
                right: parent.right
                verticalCenter: parent.verticalCenter

                leftMargin: background.margins.left
                rightMargin: background.margins.right
            }
            spacing: 4
            PlasmaExtras.Heading {
                id: titleText
                level: 4
                text: title
//                 font {
//                     weight: Font.Bold
//                     pointSize: theme.smallestFont.pointSize
//                 }
                anchors {
                    left: parent.left
                    right: parent.right
                }
                height: paintedHeight
                wrapMode: Text.WordWrap
                //go with nowrap only if there is a single word too long
                onPaintedWidthChanged: {
                    wrapTimer.restart()
                }
                Timer {
                    id: wrapTimer
                    interval: 200
                    onTriggered: {
                        //give it some pixels of tolerance
                        if (titleText.paintedWidth > titleText.width + 3) {
                            titleText.wrapMode = Text.NoWrap
                            titleText.elide = Text.ElideRight
                        } else {
                            titleText.wrapMode = Text.WordWrap
                            titleText.elide = Text.ElideNone
                        }
                    }
                }
            }
            PlasmaComponents.Label {
                text: description
                font.pointSize: theme.smallestFont.pointSize
                anchors {
                    left: parent.left
                    right: parent.right
                }
                //elide: Text.ElideRight
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignTop
                maximumLineCount: 3
            }
        }
        QIconItem {
            icon: running ? "dialog-ok-apply" : undefined
            visible: running
            width: theme.smallIconSize
            height: width
            anchors {
                right: parent.right
                bottom: parent.bottom
                rightMargin: background.margins.right
                bottomMargin: background.margins.bottom
            }
        }
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onDoubleClicked: widgetExplorer.addApplet(pluginName)
            onEntered: tooltipDialog.appletDelegate = background
            onExited: tooltipDialog.appletDelegate = null
        }
    }
}
