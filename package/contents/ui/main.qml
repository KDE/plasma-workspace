/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
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

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0

import "LayoutManager.js" as LayoutManager

MouseArea {
    id: main
    state: height>48?"active":"passive"

    property int itemWidth: main.height*1.4
    property int itemHeight: height

    Component.onCompleted: {
        LayoutManager.tasksRow = tasksRow
        LayoutManager.appletsFlickableParent = tasksRow
        LayoutManager.plasmoid = plasmoid
        LayoutManager.root = main

        LayoutManager.restoreOrder()
    }


    function addApplet(applet, pos)
    {
        var component = Qt.createComponent("PlasmoidContainer.qml")
        var plasmoidContainer = component.createObject(tasksRow, {"x": pos.x, "y": pos.y});
        var index = tasksRow.children.length
        if (pos.x >= 0) {
            //FIXME: this assumes items are square
            index = pos.x/main.height
        }

        tasksRow.insertAt(plasmoidContainer, index)
        plasmoidContainer.anchors.top = tasksRow.top
        plasmoidContainer.anchors.bottom = tasksRow.bottom
        plasmoidContainer.applet = applet
        applet.parent = plasmoidContainer
        applet.anchors.fill = plasmoidContainer
        applet.visible = true
        plasmoidContainer.visible = true

    }

    PlasmaCore.DataSource {
          id: statusNotifierSource
          engine: "statusnotifieritem"
          interval: 0
          onSourceAdded: {
             connectSource(source)
          }
          Component.onCompleted: {
              connectedSources = sources
          }
    }

    Item {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        Flickable {
            id: tasksFlickable
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: (height * 1.4 - height) / 1.5
            interactive:true
            contentWidth: tasksRow.width
            contentHeight: tasksRow.height

            width: Math.min(parent.width, tasksRow.width)

            Row {
                id: tasksRow
                spacing: 4
                height: tasksFlickable.height
                property string skipItems

                //depends on this to be precise to not make a resize loop
                onWidthChanged: {
                    var visibleCount = 0
                    for (var i = 0; i < tasksRow.children.length; ++i) {
                        if (tasksRow.children[i].opacity > 0 && tasksRow.children[i].visible) {
                            ++visibleCount
                        }
                    }
                    main.itemWidth = Math.min(main.height*1.4, centerPanel.x/visibleCount)
                }

                function insertAt(item, index)
                {
                    LayoutManager.insertAt(item, index)
                }

                function remove(item)
                {
                    LayoutManager.remove(item)
                }

                function saveOrder()
                {
                    LayoutManager.saveOrder()
                }

                Repeater {
                    id: tasksRepeater
                    model: PlasmaCore.SortFilterModel {
                        id: filteredStatusNotifiers
                        filterRole: "Title"
                        filterRegExp: tasksRow.skipItems
                        sourceModel: PlasmaCore.DataModel {
                            dataSource: statusNotifierSource
                        }
                    }

                    delegate: TaskWidget {
                    }
                }


                Component.onCompleted: {
                    var items = String(plasmoid.configuration.SkipItems)
                    if (items != "") {
                        skipItems = "^(?!" + items + ")"
                    } else {
                        skipItems = ""
                    }
                }
            }
        }
        Row {
            id: centerPanel
            anchors {
                top: parent.top
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }
        }
        Row {
            id: rightPanel
            anchors {
                top: parent.top
                bottom: parent.bottom
                right: parent.right
                rightMargin: 8
            }
        }
    }
}
