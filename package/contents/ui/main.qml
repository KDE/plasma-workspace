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
    property alias expanded: dialog.visible
    property Item activeApplet

    property alias visibleLayout: tasksRow
    property alias hiddenLayout: expandedRepresentation.hiddenLayout

    function addApplet(applet, x, y) {
        var component = Qt.createComponent("PlasmoidItem.qml")
        var plasmoidContainer = component.createObject((applet.status == PlasmaCore.Types.PassiveStatus) ? hiddenLayout : visibleLayout, {"x": x, "y": y});

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

    property Item oldActiveApplet
    onActiveAppletChanged: {
        if (!activeApplet) {
            dialog.visible = false;
        }
        if (oldActiveApplet) {
            oldActiveApplet.expanded = false;
        }
        oldActiveApplet = activeApplet;
    }
    Connections {
        target: activeApplet
        onExpandedChanged: dialog.visible = activeApplet.expanded
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

                delegate: StatusNotifierItem {}
            }
        }

        ExpanderArrow {
            id: expander
        }
    }

    PlasmaCore.Dialog {
        id: dialog
        visualParent: root
        onVisibleChanged: {
            if (!visible) {
                root.activeApplet = null
            }
        }
        mainItem: ExpandedRepresentation {
            id: expandedRepresentation
            activeApplet: root.activeApplet
        }
    }
}
