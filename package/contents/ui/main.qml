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
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0

MouseArea {
    id: root

    property int itemWidth: Math.min(width, units.iconSizes.medium)
    property int itemHeight: Math.min(height, units.iconSizes.medium)
    property bool expanded: false
    property Item activeApplet

    function addApplet(applet, x, y) {
        var component = Qt.createComponent("PlasmoidContainer.qml")
        var plasmoidContainer = component.createObject(tasksRow, {"x": x, "y": y});

        plasmoidContainer.parent = tasksRow;
        plasmoidContainer.applet = applet
        applet.parent = plasmoidContainer
        applet.anchors.fill = plasmoidContainer
        applet.visible = true
        plasmoidContainer.visible = true

    }

    Containment.onAppletAdded: {
        addApplet(applet, x, y);
    }

    Containment.onAppletRemoved: {
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

    PlasmaCore.SortFilterModel {
        id: hiddenTasksModel
        filterRole: "Status"
        filterRegExp: "Passive"
        sourceModel: PlasmaCore.DataModel {
            dataSource: statusNotifierSource
        }
    }

    Row {
        anchors.fill: parent

        Flow {
            id: tasksRow
            spacing: 4
            height: parent.height
            width: parent.width - expander.width
            property string skipItems
            flow: plasmoid.formFactor == PlasmaCore.Types.Vertical ? Flow.LeftToRight : Flow.TopToBottom

            Repeater {
                id: tasksRepeater
                model: PlasmaCore.SortFilterModel {
                    id: filteredStatusNotifiers
                    filterRole: "Status"
                    filterRegExp: "(Active|RequestingAttention)"
                    sourceModel: PlasmaCore.DataModel {
                        dataSource: statusNotifierSource
                    }
                }

                delegate: TaskWidget {}
            }
        }

        ExpanderArrow {
            id: expander
        }
    }

    PlasmaCore.Dialog {
        id: dialog
        visualParent: main
        visible: root.expanded
        mainItem: PlasmoidPopupsContainer {
            activeApplet: root.activeApplet
            Column {
                Layout.minimumWidth: units.gridUnit * 12
                Layout.minimumHeight: units.gridUnit * 12
                Repeater {
                    id: hiddenTasksRepeater
                    model: hiddenTasksModel

                    delegate: TaskWidget {}
                }
            }
        }
    }
}
