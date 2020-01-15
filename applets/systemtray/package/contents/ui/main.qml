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
import org.kde.draganddrop 2.0 as DnD
import org.kde.kirigami 2.5 as Kirigami

import "items"

MouseArea {
    id: root

    Layout.minimumWidth: vertical ? units.iconSizes.small : tasksRow.implicitWidth + (expander.visible ? expander.implicitWidth : 0) + units.smallSpacing

    Layout.minimumHeight: vertical ? tasksRow.implicitHeight + (expander.visible ? expander.implicitHeight : 0) + units.smallSpacing : units.smallSpacing

    Layout.preferredHeight: Layout.minimumHeight
    LayoutMirroring.enabled: !vertical && Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property var iconSizes: ["small", "smallMedium", "medium", "large", "huge", "enormous"];
    property int iconSize: plasmoid.configuration.iconSize + (Kirigami.Settings.tabletMode ? 1 : 0)

    property bool vertical: plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property int itemSize: units.roundToIconSize(Math.min(Math.min(width, height), units.iconSizes[iconSizes[Math.min(iconSizes.length-1, iconSize)]]))
    property int hiddenItemSize: units.iconSizes.smallMedium
    property alias expanded: dialog.visible
    property Item activeApplet
    property int status: dialog.visible ? PlasmaCore.Types.RequiresAttentionStatus : PlasmaCore.Types.PassiveStatus

    property alias visibleLayout: tasksRow
    property alias hiddenLayout: expandedRepresentation.hiddenLayout

    property alias statusNotifierModel: statusNotifierModel

    // workaround https://bugreports.qt.io/browse/QTBUG-71238 / https://bugreports.qt.io/browse/QTBUG-72004
    property Component plasmoidItemComponent: Qt.createComponent("items/PlasmoidItem.qml")

    property int creationIdCounter: 0

    Plasmoid.onExpandedChanged: {
        if (!plasmoid.expanded) {
            dialog.visible = plasmoid.expanded;
        }
    }

    // temporary hack to fix known broken categories
    // should go away as soon as fixes are merged
    readonly property var categoryOverride: {
        "org.kde.discovernotifier": "SystemServices",
        "org.kde.plasma.networkmanagement": "Hardware",
        "org.kde.kdeconnect": "Hardware",
        "org.kde.plasma.keyboardindicator": "Hardware",
        "touchpad": "Hardware"
    }

    readonly property var categoryOrder: [
        "UnknownCategory", "ApplicationStatus", "Communications",
        "SystemServices", "Hardware"
    ]
    function indexForItemCategory(item) {
        if (item.itemId == "org.kde.plasma.notifications") {
            return -1
        }
        var i = categoryOrder.indexOf(categoryOverride[item.itemId] || item.category)
        return i == -1 ? categoryOrder.indexOf("UnknownCategory") : i
    }

    // return negative integer if a < b, 0 if a === b, and positive otherwise
    function compareItems(a, b) {
        var categoryDiff = indexForItemCategory(a) - indexForItemCategory(b)
        var textDiff = (categoryDiff != 0 ? categoryDiff : a.text.localeCompare(b.text))
        return textDiff != 0 ? textDiff : b.creationId - a.creationId
    }

    function moveItemAt(item, container, index) {
        if (container.children.length == 0) {
            item.parent = container
        } else {
            if (index == container.children.length) {
                var other = container.children[index - 1]
                if (item != other) {
                    plasmoid.nativeInterface.reorderItemAfter(item, other)
                }
            } else {
                var other = container.children[index]
                if (item != other) {
                    plasmoid.nativeInterface.reorderItemBefore(item, other)
                }
            }
        }
    }

    function reorderItem(item, container) {
        var i = 0;
        while (i < container.children.length &&
               compareItems(container.children[i], item) <= 0) {
            i++
        }
        moveItemAt(item, container, i)
    }

    function updateItemVisibility(item) {
        switch (item.effectiveStatus) {
        case PlasmaCore.Types.HiddenStatus:
            if (item.parent != invisibleEntriesContainer) {
                item.parent = invisibleEntriesContainer;
            }
            break;

        case PlasmaCore.Types.ActiveStatus:
            reorderItem(item, visibleLayout)
            break;

        case PlasmaCore.Types.PassiveStatus:
            reorderItem(item, hiddenLayout)
            item.x = 0;
            break;
        }
    }

    onWheel: {
        // Don't propagate unhandled wheel events
        wheel.accepted = true;
    }

    Containment.onAppletAdded: {
        //Allow the plasmoid expander to know in what window it will be
        var plasmoidContainer = plasmoidItemComponent.createObject(invisibleEntriesContainer, {"x": x, "y": y, "applet": applet});

        applet.parent = plasmoidContainer
        applet.anchors.left = plasmoidContainer.left
        applet.anchors.top = plasmoidContainer.top
        applet.anchors.bottom = plasmoidContainer.bottom
        applet.width = plasmoidContainer.height
        applet.visible = true
        plasmoidContainer.visible = true
        
        //This is to make preloading effective, minimizes the scene changes
        if (applet.fullRepresentationItem) {
            applet.fullRepresentationItem.width = expandedRepresentation.width
            applet.fullRepresentationItem.width = expandedRepresentation.height
            applet.fullRepresentationItem.parent = preloadedStorage;
        } else {
            applet.fullRepresentationItemChanged.connect(function() {
                applet.fullRepresentationItem.width = expandedRepresentation.width
                applet.fullRepresentationItem.width = expandedRepresentation.height
                applet.fullRepresentationItem.parent = preloadedStorage;
            });
        }
    }

    //being there forces the items to fully load, and they will be reparented in the popup one by one, this item is *never* visible
    Item {
        id: preloadedStorage
        visible: false
    }

    Containment.onAppletRemoved: {
    }

    Connections {
        target: plasmoid
        onUserConfiguringChanged: {
            if (plasmoid.userConfiguring) {
                dialog.visible = false
            }
        }
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

    PlasmaCore.SortFilterModel {
        id: statusNotifierModel
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
        visualParent: tasksRow
        target: root.activeApplet && root.activeApplet.parent.parent == tasksRow ? root.activeApplet.parent : root
        location: plasmoid.location
    }

    DnD.DropArea {
        anchors.fill: parent

        preventStealing: true;

        /** Extracts the name of the system tray applet in the drag data if present
         * otherwise returns null*/
        function systemTrayAppletName(event) {
            if (event.mimeData.formats.indexOf("text/x-plasmoidservicename") < 0) {
                return null;
            }
            var plasmoidId = event.mimeData.getDataAsByteArray("text/x-plasmoidservicename");

            if (!plasmoid.nativeInterface.isSystemTrayApplet(plasmoidId)) {
                return null;
            }
            return plasmoidId;
        }

        onDragEnter: {
            if (!systemTrayAppletName(event)) {
                event.ignore();
            }
        }

        onDrop: {
            var plasmoidId = systemTrayAppletName(event);
            if (!plasmoidId) {
                event.ignore();
                return;
            }

            if (plasmoid.configuration.extraItems.indexOf(plasmoidId) < 0) {
                var extraItems = plasmoid.configuration.extraItems;
                extraItems.push(plasmoidId);
                plasmoid.configuration.extraItems = extraItems;
            }
        }
    }

    //Main Layout
    Flow {
        id: tasksRow
        spacing: 0
        height: parent.height - (vertical && expander.visible ? expander.height : 0)
        width: parent.width - (vertical || !expander.visible ? 0 : expander.width)
        property string skipItems
        flow: vertical ? Flow.LeftToRight : Flow.TopToBottom
        //To make it look centered
        y: Math.round(height/2 - childrenRect.height/2)
        x: (expander.visible && LayoutMirroring.enabled ? expander.width : 0) + Math.round(width/2 - childrenRect.width/2)


        //Do spacing with margins, to correctly compute the number of lines
        property QtObject marginHints: QtObject {
            property int left: Math.round(units.smallSpacing / 2)
            property int top: Math.round(units.smallSpacing / 2)
            property int right: Math.round(units.smallSpacing / 2)
            property int bottom: Math.round(units.smallSpacing / 2)
        }

        //add doesn't seem to work used in conjunction with stackBefore/stackAfter
        /*add: Transition {
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
        hideOnWindowDeactivate: !plasmoid.configuration.pin

        onVisibleChanged: {
            if (!visible) {
                plasmoid.status = PlasmaCore.Types.PassiveStatus;
                if (root.activeApplet) {
                    root.activeApplet.expanded = false;
                }
            } else {
                plasmoid.status = PlasmaCore.Types.RequiresAttentionStatus;
            }
            plasmoid.expanded = visible;
        }
        mainItem: ExpandedRepresentation {
            id: expandedRepresentation

            Keys.onEscapePressed: {
                root.expanded = false;
            }

            activeApplet: root.activeApplet

            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
        }
    }
}
