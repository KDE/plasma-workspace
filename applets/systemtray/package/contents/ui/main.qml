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

import QtQuick 2.5
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import "items"

MouseArea {
    id: root

    Layout.minimumWidth: vertical ? units.iconSizes.small : tasksRow.implicitWidth + expander.implicitWidth + units.smallSpacing

    Layout.minimumHeight: vertical ? tasksRow.implicitHeight+ expander.implicitHeight + units.smallSpacing : units.smallSpacing

    property bool vertical: plasmoid.formFactor == PlasmaCore.Types.Vertical
    property int itemSize: Math.min(Math.min(width, height), units.iconSizes.smallMedium)
    property int hiddenItemSize: units.iconSizes.smallMedium
    property alias expanded: dialog.visible
    property Item activeApplet

    property alias visibleLayout: tasksRow
    property alias hiddenLayout: expandedRepresentation.hiddenLayout

    property alias statusNotifierModel: statusNotifierModel

    function addApplet(applet, x, y) {
        print("Applet created:" + applet.title)
        var component = Qt.createComponent("items/PlasmoidItem.qml")
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

        onExtraItemsChanged: plasmoid.nativeInterface.allowedPlasmoids = plasmoid.configuration.extraItems
    }

    Component.onCompleted: {
        //script, don't bind
        plasmoid.nativeInterface.allowedPlasmoids = initializePlasmoidList();
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


    //due to the magic of property bindings this function will be
    //re-executed all the times a setting changes
    property var shownCategories: {
        var array = [];
        if (plasmoid.configuration.applicationStatusShown) {
            array.push("ApplicationStatus");
        }
        if (plasmoid.configuration.communicationsShown) {
            array.push("Communications");
        }
        if (plasmoid.configuration.systemServicesShown) {
            array.push("SystemServices");
        }
        if (plasmoid.configuration.hardwareControlShown) {
            array.push("Hardware");
        }
        if (plasmoid.configuration.miscellaneousShown) {
            array.push("UnknownCategory");
        }

        //nothing? make a regexp that matches nothing
        if (array.length == 0) {
            array.push("$^")
        }
        return array;
    }
    
    PlasmaCore.SortFilterModel {
        id: statusNotifierModel
        filterRole: "Category"
        filterRegExp: "("+shownCategories.join("|")+")"
        sourceModel: PlasmaCore.DataModel {
            dataSource: statusNotifierSource
        }
    }

    //This is a dump for items we don't want to be seen or as an incubation, when they are
    //created as a nursery before going in their final place
    Item {
        id: invisibleEntriesContainer
        visible: false
        Repeater {
            id: tasksRepeater
            model: statusNotifierModel

            delegate: StatusNotifierItem {}
        }
        //NOTE: this exists mostly for not causing reference errors
        property QtObject marginHints: QtObject {
            property int left: 0
            property int top: 0
            property int right: 0
            property int bottom: 0
        }
    }

    CurrentItemHighLight {
        target: root.activeApplet && root.activeApplet.parent.parent == tasksRow ? root.activeApplet.parent : root
        location: plasmoid.location
    }

    //Main Layout
    Flow {
        id: tasksRow
        spacing: units.smallSpacing
        height: parent.height - (vertical ? expander.height : 0)
        width: parent.width  - (vertical ? 0 : expander.width)
        property string skipItems
        flow: vertical ? Flow.LeftToRight : Flow.TopToBottom
        //To make it look centered
        y: vertical ? 0 : (height % root.itemSize) / 2
        x: vertical ? (width % root.itemSize) / 2 : 0
        //make last icon appeared nearer to the tasskbar
        //TODO: Qt 5.6 will have functions for it
        function addItem(item) {
            if (item.parent == tasksRow || item.repositioningInProgress == true || item == marginHints) {
                return;
            }
            var items = [];
            for (var i = 0; i < tasksRow.children.length; ++i) {
                items.push(tasksRow.children[i]);
            }

            for (var i = 0; i < items.length; ++i) {
                items[i].repositioningInProgress = true;
                items[i].parent = invisibleEntriesContainer;
            }

            item.parent = tasksRow;

            for (var i = 0; i < items.length; ++i) {
                items[i].parent = tasksRow;
                items[i].repositioningInProgress = false;
            }
        }

        //NOTE: this exists mostly for not causing reference errors
        property QtObject marginHints: QtObject {
            property int left: 0
            property int top: 0
            property int right: 0
            property int bottom: 0
        }

        /*FIXME: need Qt 5.6 for transitions to look good
         add: Transition {
            NumberAnimation {
                property: "scale"
                from: 0
                to: 1
                easing.type: Easing.InQuad
                duration: units.longDuration
            }
        }
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.InQuad
                duration: units.longDuration
            }
        }*/
    }

    ExpanderArrow {
        id: expander
        anchors {
            fill: parent
            leftMargin: vertical ? 0 : parent.width - implicitWidth
            topMargin: vertical ? parent.height - implicitHeight : 0
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
            if (!visible && root.activeApplet) {
                root.activeApplet.expanded = false;
            }
        }
        mainItem: ExpandedRepresentation {
            id: expandedRepresentation
            activeApplet: root.activeApplet
        }
    }
}
