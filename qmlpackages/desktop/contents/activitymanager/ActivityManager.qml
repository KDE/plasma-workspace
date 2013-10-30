/*
 *   Copyright 2013 Marco Martin <mart@kde.org>
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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.qtextracomponents 2.0

Item {
    id: main
    signal closed()

    //this is used to perfectly align the filter field and delegates
    property int cellWidth: theme.mSize(theme.defaultFont).width * 20

    QtObject {
        id: activityManager
        property int orientation: Qt.Vertical
    }
    property int minimumWidth: cellWidth + (
        activityManager.orientation == Qt.Horizontal
        ? 0
        : (scrollBar.width + 4 * 2) // 4 * 2 == left and right margins
        )
    property int minimumHeight: topBar.height + list.delegateHeight + (activityManager.orientation == Qt.Horizontal ? scrollBar.height : 0) + 4


    PlasmaCore.DataSource {
        id: activitySource
        engine: "org.kde.activities"
        onSourceAdded: {
            if (source != "Status") {
                connectSource(source)
            }
        }
        Component.onCompleted: {
            connectedSources = sources.filter(function(val) {
                return val != "Status";
            })
        }
    }

    PlasmaComponents.ContextMenu {
        id: newActivityMenu
        visualParent: topBar.newActivityButton
        PlasmaComponents.MenuItem {
            id: templatesItem
            text: i18n("Templates")
            onClicked: activityTemplatesMenu.open()
        }
        PlasmaComponents.MenuItem {
            icon: "user-desktop"
            text: i18n("Empty Desktop")
            onClicked: activityManager.createActivity("desktop")
        }
        PlasmaComponents.MenuItem {
            icon: "edit-copy"
            text: i18n("Clone current activity")
            onClicked: activityManager.cloneCurrentActivity()
        }
    }


    PlasmaComponents.ContextMenu {
        id: activityTemplatesMenu
        visualParent: templatesItem
    }
    Repeater {
        parent: activityTemplatesMenu
        model: activityManager.activityTypeActions
        delegate: PlasmaComponents.MenuItem {
            icon: modelData.icon
            text: modelData.text
            separator: modelData.separator
            onClicked: {
                //is a plugin?
                if (modelData.pluginName) {
                    activityManager.createActivity(modelData.pluginName)
                //is a script?
                } else if (modelData.scriptFile) {
                    activityManager.createActivityFromScript(modelData.scriptFile,  modelData.text, modelData.icon, modelData.startupApps)
                //invoke ghns
                } else {
                    activityManager.downloadActivityScripts()
                }
            }
            Component.onCompleted: {
                parent = activityTemplatesMenu
            }
        }
    }


    Loader {
        id: topBar
        property string query
        property Item newActivityButton

        sourceComponent: (activityManager.orientation == Qt.Horizontal) ? horizontalTopBarComponent : verticalTopBarComponent
        height: item.height + 2
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right

            topMargin: activityManager.orientation == Qt.Horizontal ? 4 : 0
            leftMargin: 4
        }
    }
    Component {
        id: horizontalTopBarComponent
        Item {
            anchors {
                top: parent.top
                left:parent.left
                right: parent.right
            }
            height: filterField.height

            PlasmaComponents.TextField {
                id: filterField
                anchors {
                    left: parent.left
                    leftMargin: 2
                }
                width: list.width / Math.floor(list.width / cellWidth) - 4
                clearButtonShown: true
                onTextChanged: topBar.query = text
                placeholderText: i18n("Enter search term...")
                Component.onCompleted: forceActiveFocus()
            }

            Row {
                anchors.right: parent.right
                spacing: 4
                PlasmaComponents.Button {
                    id: newActivityButton
                    iconSource: "list-add"
                    text: i18n("Create activity...")
                    onClicked: newActivityMenu.open()
                }
                PlasmaComponents.Button {
                    iconSource: "plasma"
                    text: i18n("Add widgets")
                    onClicked: activityManager.addWidgetsRequested()
                }
                PlasmaComponents.ToolButton {
                    iconSource: "window-close"
                    onClicked: main.closed()
                }
            }
            Component.onCompleted: {
                topBar.newActivityButton = newActivityButton
            }
        }
    }
    Component {
        id: verticalTopBarComponent
        Column {
            spacing: 4
            anchors {
                top: parent.top
                left:parent.left
                right: parent.right
            }

            PlasmaComponents.ToolButton {
                anchors.right: parent.right
                iconSource: "window-close"
                onClicked: main.closed()
            }

            PlasmaComponents.TextField {
                id: filterField
                anchors {
                    left: parent.left
                    right: parent.right
                }
                clearButtonShown: true
                onTextChanged: topBar.query = text
                placeholderText: i18n("Enter search term...")
                Component.onCompleted: forceActiveFocus()
            }

            PlasmaComponents.Button {
                id: newActivityButton
                anchors {
                    left: parent.left
                    right: parent.right
                }
                iconSource: "list-add"
                text: i18n("Create activity...")
                onClicked: newActivityMenu.open()
            }
            Component.onCompleted: {
                topBar.newActivityButton = newActivityButton
            }
        }
    }

    MouseEventListener {
        id: listParent
        anchors {
            top: topBar.bottom
            left: parent.left
            right: activityManager.orientation == Qt.Horizontal
                ? parent.right
                : (scrollBar.visible ? scrollBar.left : parent.right)
            bottom: activityManager.orientation == Qt.Horizontal ? scrollBar.top : bottomBar.top
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

            property int delegateWidth: (activityManager.orientation == Qt.Horizontal) ? (list.width / Math.floor(list.width / cellWidth)) : list.width
            property int delegateHeight: theme.mSize(theme.defaultFont).height * 7 - 4


            anchors.fill: parent

            orientation: activityManager.orientation == Qt.Horizontal ? ListView.Horizontal : ListView.vertical
            snapMode: ListView.SnapToItem
            model: PlasmaCore.SortFilterModel {
                sourceModel: PlasmaCore.DataModel {
                    dataSource: activitySource
                }
                filterRole: "Name"
                filterRegExp: ".*"+topBar.query+".*"
            }
            clip: activityManager.orientation == Qt.Vertical

            delegate: ActivityDelegate {}
        }
    }
    PlasmaComponents.ScrollBar {
        id: scrollBar
        orientation: activityManager.orientation == Qt.Horizontal ? ListView.Horizontal : ListView.Vertical
        anchors {
            top: activityManager.orientation == Qt.Horizontal ? undefined : listParent.top
            bottom: activityManager.orientation == Qt.Horizontal ? parent.bottom : bottomBar.top
            left: activityManager.orientation == Qt.Horizontal ? parent.left : undefined
            right: parent.right
        }
        flickableItem: list
    }

    Loader {
        id: bottomBar

        sourceComponent: (activityManager.orientation == Qt.Horizontal) ? undefined : verticalBottomBarComponent
        height: item.height
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
                iconSource: "plasma"
                text: i18n("Add widgets")
                onClicked: activityManager.addWidgetsRequested()
            }
        }
    }
}

