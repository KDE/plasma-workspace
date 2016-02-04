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

    Layout.minimumWidth: tasksRow.implicitWidth + expander.implicitWidth+30
    property int itemSize: Math.min(Math.min(width, height), units.iconSizes.medium)
    property int hiddenItemSize: units.iconSizes.smallMedium
    property alias expanded: dialog.visible
    property Item activeApplet

    property alias visibleLayout: tasksRow
    property alias hiddenLayout: expandedRepresentation.hiddenLayout

    function addApplet(applet, x, y) {
        print("Applet created:" + applet.title)
        var component = Qt.createComponent("PlasmoidItem.qml")
        var plasmoidContainer = component.createObject((applet.status == PlasmaCore.Types.PassiveStatus) ? hiddenLayout : visibleLayout, {"x": x, "y": y});

        plasmoidContainer.applet = applet
        applet.parent = plasmoidContainer
        applet.anchors.left = plasmoidContainer.left
        applet.anchors.top = plasmoidContainer.top
        applet.anchors.bottom = plasmoidContainer.bottom
        applet.width = plasmoidContainer.height
        applet.visible = true
        plasmoidContainer.visible = true
    }

    Containment.onAppletAdded: {
        addApplet(applet, x, y);
    }

    Containment.onAppletRemoved: {
    }

     Connections {
        target: plasmoid.configuration
        onApplicationStatusShownChanged: plasmoid.nativeInterface.setCategoryShown(SystemTray.Task.ApplicationStatus, plasmoid.configuration.applicationStatusShown);

        onCommunicationsShownChanged: plasmoid.nativeInterface.setCategoryShown(SystemTray.Task.Communications, plasmoid.configuration.communicationsShown);

        onSystemServicesShownChanged: plasmoid.nativeInterface.setCategoryShown(SystemTray.Task.SystemServices, plasmoid.configuration.systemServicesShown);

        onHardwareControlShownChanged: plasmoid.nativeInterface.setCategoryShown(SystemTray.Task.Hardware, plasmoid.configuration.hardwareControlShown);

        onMiscellaneousShownChanged: plasmoid.nativeInterface.setCategoryShown(SystemTray.Task.Unknown, plasmoid.configuration.miscellaneousShown);

        onExtraItemsChanged: plasmoid.nativeInterface.allowedPlasmoids = plasmoid.configuration.extraItems
    }

    Component.onCompleted: {
        //script, don't bind
        plasmoid.nativeInterface.allowedPlasmoids = initializePlasmoidList();
        //not care about applets order
        for (var i = 0; i < plasmoid.applets.length; ++i) {
            addApplet(plasmoid.applets[i], 0 ,0);
        }
    }

    function initializePlasmoidList() {
        var newKnownItems = [];
        var newExtraItems = [];

        //NOTE:why this? otherwise the interpreter will execute plasmoid.nativeInterface.defaultPlasmoids() on
        //every access of defaults[], resulting in a very slow iteration
        var defaults = [];
        //defaults = defaults.concat(plasmoid.nativeInterface.defaultPlasmoids);
        defaults = plasmoid.nativeInterface.defaultPlasmoids.slice()
        var candidate;

        //Add every plasmoid that is both not enabled explicitly and not already known
        for (var i = 0; i < defaults.length; ++i) {
            candidate = defaults[i];
            if (plasmoid.configuration.knownItems.indexOf(candidate) === -1) {
                newKnownItems.push(candidate);
                if (plasmoid.configuration.extraItems.indexOf(candidate) === -1) {
                    newExtraItems.push(candidate);
                }
            }
        }

        if (newExtraItems.length > 0) {
            plasmoid.configuration.extraItems = plasmoid.configuration.extraItems.slice().concat(newExtraItems);
        }
        if (newKnownItems.length > 0) {
            plasmoid.configuration.knownItems = plasmoid.configuration.knownItems.slice().concat(newKnownItems);
        }

        return plasmoid.configuration.extraItems;
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

    CurrentItemHighLight {
        target: root.activeApplet && root.activeApplet.parent.parent == tasksRow ? root.activeApplet.parent : root
        location: plasmoid.location
    }

    //Main Layout
    Row {
        id: mainLayout
        anchors.fill: parent

        Flow {
            id: tasksRow
            spacing: 0
            height: parent.height
            width: parent.width - expander.width
            property string skipItems
            flow: plasmoid.formFactor == PlasmaCore.Types.Vertical ? Flow.LeftToRight : Flow.TopToBottom
            //NOTE: this exists mostly for not causing reference errors
            property QtObject marginHints: QtObject {
                property int left: 0
                property int top: 0
                property int right: 0
                property int bottom: 0
            }

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

    //Main popup
    PlasmaCore.Dialog {
        id: dialog
        visualParent: root
        flags: Qt.WindowStaysOnTopHint
        location: plasmoid.location
        hideOnWindowDeactivate: expandedRepresentation.hideOnWindowDeactivate
        onVisibleChanged: {
            if (!visible) {
                root.activeApplet.expanded = false;
            }
        }
        mainItem: ExpandedRepresentation {
            id: expandedRepresentation
            activeApplet: root.activeApplet
        }
    }
}
