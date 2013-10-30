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

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.qtextracomponents 2.0

PlasmaCore.FrameSvgItem {
    id: background

    width: Math.max((delegateStack.currentPage ? delegateStack.currentPage.implicitWidth : 0) + margins.left + margins.right, list.delegateWidth)
    height: Math.max((delegateStack.currentPage ? delegateStack.currentPage.implicitHeight : 0) + margins.top + margins.bottom, list.delegateHeight)

    imagePath: "widgets/viewitem"
    prefix: Current ? (mainMouseArea.containsMouse ? "selected+hover" : "selected") : (mainMouseArea.containsMouse ? "hover" : "normal")

    Behavior on width {
        NumberAnimation { duration: 250 }
    }
    Behavior on height {
        NumberAnimation { duration: 250 }
    }

    PlasmaComponents.PageStack {
        id: delegateStack
        anchors {
            fill: parent
            leftMargin: background.margins.left
            topMargin: background.margins.top
            rightMargin: background.margins.right
            bottomMargin: background.margins.bottom
        }
        clip: true
        initialPage: iconComponent
    }

    property Item mainMouseArea
    Component {
        id: iconComponent
        Item {
            anchors.fill: parent

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    var activityId = model["DataEngineSource"]
                    var service = activitySource.serviceForSource(activityId)
                    var operation = service.operationDescription("setCurrent")
                    service.startOperationCall(operation)
                }

                Component.onCompleted: mainMouseArea = mouseArea

                PlasmaCore.IconItem {
                    id: iconWidget
                    anchors.verticalCenter: parent.verticalCenter
                    x: y
                    width: theme.hugeIconSize
                    height: width
                    source: Icon
                }
                QPixmapItem {
                    anchors.fill: iconWidget
                    pixmap: Icon ? undefined : activityManager.pixmapForActivity(model["DataEngineSource"])
                    visible: Icon == ""
                }
                PlasmaCore.IconItem {
                    width: theme.mediumIconSize
                    height: width
                    anchors.centerIn: iconWidget
                    source: "media-playback-start"
                    visible: model["State"] != "Running"
                }
                Column {
                    anchors {
                        left: iconWidget.right
                        right: parent.right
                        verticalCenter: parent.verticalCenter

                        leftMargin: background.margins.left
                    }
                    PlasmaComponents.Label {
                        id: titleText
                        text: Name
                        anchors.left: parent.left
                        anchors.right: parent.right
                        horizontalAlignment: Text.AlignHCenter
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
                    Row {
                        id: buttonsRow
                        visible: model["State"] == "Running"
                        anchors.horizontalCenter: parent.horizontalCenter

                        PlasmaComponents.ToolButton {
                            id: configureButton
                            flat: false
                            iconSource: "configure"
                            onClicked: delegateStack.push(configurationComponent)
                        }
                        PlasmaComponents.ToolButton {
                            visible: !model["Current"]
                            iconSource: "media-playback-stop"
                            flat: false
                            onClicked: {
                                var activityId = model["DataEngineSource"]
                                var service = activitySource.serviceForSource(activityId)
                                var operation = service.operationDescription("stop")
                                service.startOperationCall(operation)
                            }
                        }
                    }
                    PlasmaComponents.ToolButton {
                        visible: model["State"] != "Running"
                        iconSource: "edit-delete"
                        text: i18n("Delete")
                        width: Math.min(implicitWidth, parent.width)
                        anchors.horizontalCenter: parent.horizontalCenter
                        onClicked: delegateStack.push(confirmationComponent)
                    }
                }
            }
        }
    }

    Component {
        id: confirmationComponent
        MouseArea {
            anchors.fill: parent

            //20 is just a number arbitrarly low, won't be followed
            implicitWidth: (activityManager.orientation == Qt.Horizontal) ? confirmationLabel.paintedWidth : 20
            implicitHeight: (activityManager.orientation == Qt.Horizontal) ? 20 : confirmationColumn.childrenRect.height

            onClicked: delegateStack.pop()
            Column {
                id: confirmationColumn
                anchors.fill: parent
                spacing: 4
                PlasmaComponents.Label {
                    id: confirmationLabel
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n("Remove activity %1?", Name)
                    wrapMode: (activityManager.orientation == Qt.Horizontal) ? Text.NoWrap : Text.Wrap
                }

                PlasmaComponents.Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: i18n("Remove")
                    onClicked: {
                        var activityId = model["DataEngineSource"]
                        var service = activitySource.serviceForSource(activityId)
                        var operation = service.operationDescription("remove")
                        operation["Id"] = activityId
                        service.startOperationCall(operation)
                    }
                }
                PlasmaComponents.Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: i18n("Cancel")
                    onClicked: delegateStack.pop()
                }
            }
        }
    }

    Component {
        id: configurationComponent
        MouseArea {
            anchors.fill: parent

            //20 is just a number arbitrarly low, won't be followed
            implicitWidth: (activityManager.orientation == Qt.Horizontal) ? (iconButton.x*3 + iconButton.width + theme.mSize(theme.defaultFont).width * 12) : 20
            // set the implicit height to a meaningful value, otherwise the layouting goes a little crazy :)
            implicitHeight: iconButton.y*3

            onClicked: delegateStack.pop()
            PlasmaComponents.Button {
                id: iconButton
                iconSource: Icon
                anchors {
                    top: configurationLayout.top
                    bottom: configurationLayout.bottom
                }
                x: y
                width: height
                QPixmapItem {
                    anchors.centerIn: parent
                    width: theme.largeIconSize
                    height: width
                    smooth: true
                    visible: iconButton.iconSource == ""
                    pixmap: visible ? undefined : activityManager.pixmapForActivity(model["DataEngineSource"])
                }
                onClicked: iconSource = activityManager.chooseIcon()
            }
            Column {
                id: configurationLayout
                anchors {
                    left: iconButton.right
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    leftMargin: iconButton.x
                    rightMargin: iconButton.x
                }
                spacing: 4
                PlasmaComponents.TextField {
                    id: activityNameField
                    text: Name
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                PlasmaComponents.Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: i18n("Apply")
                    onClicked: {
                        var activityId = model["DataEngineSource"]
                        var service = activitySource.serviceForSource(activityId)

                        var operation = service.operationDescription("setName")
                        operation["Name"] = activityNameField.text
                        service.startOperationCall(operation)

                        var operation = service.operationDescription("setIcon")
                        operation["Icon"] = iconButton.iconSource
                        service.startOperationCall(operation)

                        delegateStack.pop()
                    }
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                PlasmaComponents.Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: i18n("Cancel")
                    onClicked: delegateStack.pop()
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
            }
        }
    }
}
