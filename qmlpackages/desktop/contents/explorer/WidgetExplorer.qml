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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.qtextracomponents 2.0

Item {
    id: main

    width: 240
    height: 800
    //this is used to perfectly align the filter field and delegates
    property int cellWidth: theme.defaultFont.pixelSize * 20

    PlasmaCore.FrameSvgItem {
        imagePath: "dialogs/background"
        anchors.fill: parent
        anchors.margins: margins
        //color: "orange"
        //opacity: 0.3
    }

    property int minimumWidth: cellWidth + (
        widgetExplorer.orientation == Qt.Horizontal
        ? 0
        : (scrollBar.width + 4 * 2) // 4 * 2 == left and right margins
        )
    property int minimumHeight: topBar.height + list.delegateHeight + (widgetExplorer.orientation == Qt.Horizontal ? scrollBar.height : 0) + 4

    property Item getWidgetsButton
    property Item categoryButton

    PlasmaComponents.ContextMenu {
        id: categoriesDialog
        visualParent: main.categoryButton
    }
    Repeater {
        parent: categoriesDialog
        model: widgetExplorer.filterModel
        delegate: PlasmaComponents.MenuItem {
            text: display
            separator: model["separator"] != undefined ? model["separator"] : false
            onClicked: {
                list.contentX = 0
                list.contentY = 0
                main.categoryButton.text = display
                widgetExplorer.widgetsModel.filterQuery = model["filterData"]
                widgetExplorer.widgetsModel.filterType = model["filterType"]
            }
            Component.onCompleted: {
                parent = categoriesDialog
            }
        }
    }

    PlasmaComponents.ContextMenu {
        id: getWidgetsDialog
        visualParent: main.getWidgetsButton
    }
    Repeater {
        parent: getWidgetsDialog
        model: widgetExplorer.widgetsMenuActions
        delegate: PlasmaComponents.MenuItem {
            icon: modelData.icon
            text: modelData.text
            separator: modelData.separator
            onClicked: modelData.trigger()
            Component.onCompleted: {
                parent = getWidgetsDialog
            }
        }
    }

    PlasmaCore.Dialog {
        id: tooltipDialog
        property Item appletDelegate

        Component.onCompleted: {
            tooltipDialog.setAttribute(Qt.WA_X11NetWmWindowTypeToolTip, true)
            tooltipDialog.windowFlags = Qt.Window|Qt.WindowStaysOnTopHint|Qt.X11BypassWindowManagerHint
        }

        onAppletDelegateChanged: {
            if (!appletDelegate) {
                toolTipHideTimer.restart()
                toolTipShowTimer.running = false
            } else if (tooltipDialog.visible) {
                var point = main.tooltipPosition()
                tooltipDialog.x = point.x
                tooltipDialog.y = point.y
            } else {
                toolTipShowTimer.restart()
                toolTipHideTimer.running = false
            }
        }
        mainItem: Tooltip { id: tooltipWidget }
        Behavior on x {
            enabled: widgetExplorer.orientation == Qt.Horizontal
            NumberAnimation { duration: 250 }
        }
        Behavior on y {
            enabled: widgetExplorer.orientation == Qt.Vertical
            NumberAnimation { duration: 250 }
        }
    }
    Timer {
        id: toolTipShowTimer
        interval: 500
        repeat: false
        onTriggered: {
            var point = main.tooltipPosition()
            tooltipDialog.x = point.x
            tooltipDialog.y = point.y
            tooltipDialog.visible = true
        }
    }
    Timer {
        id: toolTipHideTimer
        interval: 1000
        repeat: false
        onTriggered: tooltipDialog.visible = false
    }
    function tooltipPosition() {
        return widgetExplorer.tooltipPosition(tooltipDialog.appletDelegate, tooltipDialog.width, tooltipDialog.height);
    }

    Loader {
        id: topBar
        property Item categoryButton

        sourceComponent: (widgetExplorer.orientation == Qt.Horizontal) ? horizontalTopBarComponent : verticalTopBarComponent
        height: item.height + 2
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right

            topMargin: widgetExplorer.orientation == Qt.Horizontal ? 4 : 0
            leftMargin: 4
        }
    }

    Component {
        id: horizontalTopBarComponent

        Item {
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
            height: filterField.height
            Row {
                spacing: 5
                anchors {
                    left: parent.left
                    leftMargin: 2
                }
                PlasmaComponents.TextField {
                    id: filterField
                    width: list.width / Math.floor(list.width / cellWidth) - 4
                    clearButtonShown: true
                    placeholderText: i18n("Enter search term...")
                    onTextChanged: {
                        list.contentX = 0
                        list.contentY = 0
                        widgetExplorer.widgetsModel.searchTerm = text
                    }
                    Component.onCompleted: forceActiveFocus()
                }
                PlasmaComponents.Button {
                    id: categoryButton
                    text: i18n("Categories")
                    onClicked: categoriesDialog.open(0, categoryButton.height)
                }
            }
            Row {
                anchors.right: parent.right
                spacing: 5
                PlasmaComponents.Button {
                    id: getWidgetsButton
                    iconSource: "get-hot-new-stuff"
                    text: i18n("Get new widgets")
                    onClicked: getWidgetsDialog.open()
                }

                Repeater {
                    model: widgetExplorer.extraActions.length
                    PlasmaComponents.Button {
                        iconSource: widgetExplorer.extraActions[modelData].icon
                        text: widgetExplorer.extraActions[modelData].text
                        onClicked: {
                            widgetExplorer.extraActions[modelData].trigger()
                        }
                    }
                }
                PlasmaComponents.ToolButton {
                    iconSource: "window-close"
                    onClicked: widgetExplorer.closeClicked()
                }
            }
            Component.onCompleted: {
                main.getWidgetsButton = getWidgetsButton
                main.categoryButton = categoryButton
            }
        }
    }

    Component {
        id: verticalTopBarComponent

        Column {
            anchors.top: parent.top
            anchors.left:parent.left
            anchors.right: parent.right
            spacing: 4

            PlasmaComponents.ToolButton {
                anchors.right: parent.right
                iconSource: "window-close"
                onClicked: widgetExplorer.closeClicked()
            }
            PlasmaComponents.TextField {
                anchors {
                    left: parent.left
                    right: parent.right
                }
                clearButtonShown: true
                placeholderText: i18n("Enter search term...")
                onTextChanged: {
                    list.contentX = 0
                    list.contentY = 0
                    widgetExplorer.widgetsModel.searchTerm = text
                }
                Component.onCompleted: forceActiveFocus()
            }
            PlasmaComponents.Button {
                anchors {
                    left: parent.left
                    right: parent.right
                }
                id: categoryButton
                text: i18n("Categories")
                onClicked: categoriesDialog.open(0, categoryButton.height)
            }
            Component.onCompleted: {
                main.categoryButton = categoryButton
            }
        }
    }

    MouseEventListener {
        id: listParent
        anchors {
            top: topBar.bottom
            left: parent.left
            right: widgetExplorer.orientation == Qt.Horizontal
                ? parent.right
                : (scrollBar.visible ? scrollBar.left : parent.right)
            bottom: widgetExplorer.orientation == Qt.Horizontal ? scrollBar.top : bottomBar.top
            leftMargin: 4
            bottomMargin: 4
        }
        onWheelMoved: {
            //use this only if the wheel orientation is vertical and the list orientation is horizontal, otherwise will be the list itself managing the wheel
            if (wheel.orientation == Qt.Vertical && list.orientation == ListView.Horizontal) {
                var delta = wheel.delta > 0 ? 20 : -20
                list.contentX = Math.min(Math.max(0, list.contentWidth - list.width),
                                         Math.max(0, list.contentX - delta))
            }
        }
        ListView {
            id: list

            property int delegateWidth: (widgetExplorer.orientation == Qt.Horizontal) ? (list.width / Math.floor(list.width / cellWidth)) : list.width
            property int delegateHeight: theme.defaultFont.pixelSize * 7 - 4

            anchors.fill: parent

            orientation: widgetExplorer.orientation == Qt.Horizontal ? ListView.Horizontal : ListView.Vertical
            snapMode: ListView.SnapToItem
            model: widgetExplorer.widgetsModel

            clip: widgetExplorer.orientation == Qt.Vertical

            delegate: AppletDelegate {}
        }

    }
    PlasmaComponents.ScrollBar {
            id: scrollBar
            orientation: widgetExplorer.orientation == Qt.Horizontal ? ListView.Horizontal : ListView.Vertical
            anchors {
                top: widgetExplorer.orientation == Qt.Horizontal ? undefined : listParent.top
                bottom: widgetExplorer.orientation == Qt.Horizontal ? parent.bottom : bottomBar.top
                left: widgetExplorer.orientation == Qt.Horizontal ? parent.left : undefined
                right: parent.right
            }
            flickableItem: list
        }

    Loader {
        id: bottomBar

        sourceComponent: (widgetExplorer.orientation == Qt.Horizontal) ? undefined : verticalBottomBarComponent
        //height: item.height
        height: 48 // FIXME
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            leftMargin: 4
        }
    }

    Component {
        id: verticalBottomBarComponent
        Column {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            spacing: 4

            PlasmaComponents.Button {
                anchors {
                    left: parent.left
                    right: parent.right
                }
                id: getWidgetsButton
                iconSource: "get-hot-new-stuff"
                text: i18n("Get new widgets")
                onClicked: getWidgetsDialog.open()
            }

            Repeater {
                model: widgetExplorer.extraActions.length
                PlasmaComponents.Button {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    iconSource: widgetExplorer.extraActions[modelData].icon
                    text: widgetExplorer.extraActions[modelData].text
                    onClicked: {
                        widgetExplorer.extraActions[modelData].trigger()
                    }
                }
            }

            Component.onCompleted: {
                main.getWidgetsButton = getWidgetsButton
            }
        }
    }
}
