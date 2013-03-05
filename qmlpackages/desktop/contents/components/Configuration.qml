/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.configuration 0.1


//TODO: all of this will be done with desktop components
Rectangle {
    id: root

//BEGIN properties
    color: "lightgray"
    width: 640
    height: 480
//END properties

//BEGIN model
    property ConfigModel globalConfigModel: plasmoid.containmentType !== undefined ? globalContainmentConfigModel : globalAppletConfigModel
    ConfigModel {
        id: globalAppletConfigModel
        ConfigCategory {
            name: "Keyboard shortcuts"
            icon: "preferences-desktop-keyboard"
            source: "ConfigurationShortcuts.qml"
        }
    }
    ConfigModel {
        id: globalContainmentConfigModel
        ConfigCategory {
            name: "Appearance"
            icon: "preferences-desktop-wallpaper"
            source: "ConfigurationContainmentAppearance.qml"
        }
        ConfigCategory {
            name: "Mouse Actions"
            icon: "preferences-desktop-mouse"
            source: "ConfigurationContainmentActions.qml"
        }
    }
//END model

//BEGIN functions
    function saveConfig() {
        for (var key in plasmoid.configuration) {
            if (main.currentPage["cfg_"+key] !== undefined) {
                plasmoid.configuration[key] = main.currentPage["cfg_"+key]
            }
        }
    }

    function restoreConfig() {
        for (var key in plasmoid.configuration) {
            if (main.currentPage["cfg_"+key] !== undefined) {
                main.currentPage["cfg_"+key] = plasmoid.configuration[key]
            }
        }
    }
//END functions


//BEGIN connections
    Component.onCompleted: {
        if (configDialog.configModel && configDialog.configModel.count > 0) {
            main.sourceFile = configDialog.configModel.get(0).source
        } else {
            main.sourceFile = globalConfigModel.get(0).source
        }
        root.restoreConfig()
        root.width = mainColumn.implicitWidth
        root.height = mainColumn.implicitHeight
    }
//END connections

//BEGIN UI components
    Column {
        id: mainColumn
        anchors.fill: parent
        property int implicitWidth: Math.max(contentRow.implicitWidth, buttonsRow.implicitWidth) + 8
        property int implicitHeight: contentRow.implicitHeight + buttonsRow.implicitHeight + 8

        Row {
            id: contentRow
            anchors {
                left: parent.left
                right: parent.right
            }
            spacing: 4
            height: parent.height - buttonsRow.height
            property int implicitWidth: categoriesScroll.implicitWidth + pageScroll.implicitWidth
            property int implicitHeight: Math.max(categoriesScroll.implicitHeight, pageScroll.implicitHeight)

            PlasmaExtras.ScrollArea {
                id: categoriesScroll
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                }
                visible: (configDialog.configModel ? configDialog.configModel.count : 0) + globalConfigModel.count > 1
                width: visible ? 100 : 0
                implicitWidth: width
                implicitHeight: theme.defaultFont.mSize.height * 12
                Flickable {
                    id: categoriesView
                    contentWidth: width
                    contentHeight: childrenRect.height
                    anchors.fill: parent

                    property Item currentItem

                    Rectangle {
                        id: categories
                        width: parent.width
                        height: Math.max(categoriesView.height, categoriesColumn.height)
                        color: "white"

                        Rectangle {
                            color: theme.highlightColor
                            width: parent.width
                            height:  categoriesView.currentItem.height
                            y: categoriesView.currentItem.y
                            Behavior on y {
                                NumberAnimation {
                                    duration: 250
                                    easing.type: "InOutQuad"
                                }
                            }
                        }
                        Column {
                            id: categoriesColumn
                            width: parent.width
                            Repeater {
                                model: configDialog.configModel
                                delegate: ConfigCategoryDelegate {}
                            }
                            Repeater {
                                model: globalConfigModel
                                delegate: ConfigCategoryDelegate {}
                            }
                        }
                    }
                }
            }
            PlasmaExtras.ScrollArea {
                id: pageScroll
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    margins: 4
                }
                width: parent.width - categoriesScroll.width - 8
                implicitWidth: main.currentPage ? main.currentPage.implicitWidth : 0
                implicitHeight: main.currentPage ? main.currentPage.implicitHeight : 0
                Flickable {
                    contentWidth: width
                    contentHeight: main.height
                    Item {
                        width: parent.width
                        height: childrenRect.height
                        PlasmaComponents.PageStack {
                            id: main
                            anchors.fill: parent
                            property string sourceFile
                            onSourceFileChanged: {
                                replace(Qt.resolvedUrl(sourceFile))
                                root.width = mainColumn.implicitWidth
                                root.height = mainColumn.implicitHeight
                                /*
                                 * This is not needed on a desktop shell that has ok/apply/cancel buttons, i'll leave it here only for future reference until we have a prototype for the active shell.
                                 * root.pageChanged will start a timer, that in turn will call saveConfig() when triggered

                                for (var prop in currentPage) {
                                    if (prop.indexOf("cfg_") === 0) {
                                        currentPage[prop+"Changed"].connect(root.pageChanged)
                                    }
                                }*/
                            }
                        }
                    }
                }
            }
        }
        Row {
            id: buttonsRow
            spacing: 4
            anchors {
                right: parent.right
                rightMargin: spacing
            }
            PlasmaComponents.Button {
                iconSource: "dialog-ok"
                text: "Ok"
                onClicked: {
                    root.saveConfig()
                    configDialog.close()
                }
            }
            PlasmaComponents.Button {
                iconSource: "dialog-ok-apply"
                text: "Apply"
                onClicked: root.saveConfig()
            }
            PlasmaComponents.Button {
                iconSource: "dialog-cancel"
                text: "Cancel"
                onClicked: configDialog.close()
            }
        }
    }
//END UI components
}
