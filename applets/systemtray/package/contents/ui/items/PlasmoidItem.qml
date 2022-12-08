/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQml 2.15

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

AbstractItem {
    id: plasmoidContainer

    property Item applet: model.applet || null
    text: applet ? applet.title : ""

    itemId: applet ? applet.pluginName : ""
    mainText: applet ? applet.toolTipMainText : ""
    subText: applet ? applet.toolTipSubText : ""
    mainItem: applet && applet.toolTipItem ? applet.toolTipItem : null
    textFormat: applet ? applet.toolTipTextFormat : ""
    active: systemTrayState.activeApplet !== applet

    // FIXME: Use an input type agnostic way to activate whatever the primary
    // action of a plasmoid is supposed to be, even if it's just expanding the
    // Plasmoid. Not all plasmoids are supposed to expand and not all plasmoids
    // do anything with onActivated.
    onActivated: {
        if (applet) {
            applet.nativeInterface.activated()
        }
    }

    onClicked: {
        if (!applet) {
            return
        }
        //forward click event to the applet
        var appletItem = applet.compactRepresentationItem ? applet.compactRepresentationItem : applet.fullRepresentationItem
        const mouseArea = findMouseArea(appletItem)
        if (mouseArea && mouse.button !== Qt.RightButton) {
            mouseArea.clicked(mouse)
        } else if (mouse.button === Qt.LeftButton) {//falback
            plasmoidContainer.activated(null)
        }
    }
    onPressed: {
        // Only Plasmoids can show context menu on the mouse pressed event.
        // SNI has few problems, for example legacy applications that still use XEmbed require mouse to be released.
        if (mouse.button === Qt.RightButton) {
            plasmoidContainer.contextMenu(mouse);
        } else {
            const appletItem = applet.compactRepresentationItem ? applet.compactRepresentationItem : applet.fullRepresentationItem
            const mouseArea = findMouseArea(appletItem)
            if (mouseArea) {
                // HACK QML only sees the "mouseArea.pressed" property, not the signal.
                Plasmoid.nativeInterface.emitPressed(mouseArea, mouse);
            }
        }
    }
    onContextMenu: if (applet) {
        Plasmoid.nativeInterface.showPlasmoidMenu(applet, 0,
                                                  plasmoidContainer.inHiddenLayout ? applet.height : 0);
    }
    onWheel: {
        if (!applet) {
            return
        }
        //forward wheel event to the applet
        var appletItem = applet.compactRepresentationItem ? applet.compactRepresentationItem : applet.fullRepresentationItem
        const mouseArea = findMouseArea(appletItem)
        if (mouseArea) {
            mouseArea.wheel(wheel)
        }
    }

    //some heuristics to find MouseArea
    function findMouseArea(item) {
        if (!item) {
            return null
        }

        if (item instanceof MouseArea) {
            return item
        }
        for (var i = 0; i < item.children.length; i++) {
            const child = item.children[i]
            if (child instanceof MouseArea && child.enabled) {
                //check if MouseArea covers the entire item
                if (child.anchors.fill === item || (child.x === 0 && child.y === 0 && child.height === item.height && child.width === item.width)) {
                    return child
                }
            }
        }

        return null
    }

    //This is to make preloading effective, minimizes the scene changes
    function preloadFullRepresentationItem(fullRepresentationItem) {
        if (fullRepresentationItem && fullRepresentationItem.parent === null) {
            fullRepresentationItem.width = expandedRepresentation.width
            fullRepresentationItem.height = expandedRepresentation.height
            fullRepresentationItem.parent = preloadedStorage;
        }
    }

    onAppletChanged: {
        if (applet) {
            applet.parent = plasmoidContainer.iconContainer
            applet.anchors.fill = applet.parent
            applet.visible = true

            preloadFullRepresentationItem(applet.fullRepresentationItem)
        }
    }

    Connections {
        target: plasmoidContainer.applet

        //activation using global keyboard shortcut
        function onActivated() {
            plasmoidContainer.startActivatedAnimation()
        }

        function onExpandedChanged(expanded) {
            if (expanded) {
                systemTrayState.setActiveApplet(plasmoidContainer.applet, model.row)
                plasmoidContainer.startActivatedAnimation()
            }
        }

        function onFullRepresentationItemChanged(fullRepresentationItem) {
            preloadFullRepresentationItem(fullRepresentationItem)
        }
    }

    PlasmaComponents3.BusyIndicator {
        anchors.fill: parent
        z: 999
        running: plasmoidContainer.applet && plasmoidContainer.applet.busy
    }

    Binding {
        property: "hideOnWindowDeactivate"
        value: !Plasmoid.configuration.pin
        target: plasmoidContainer.applet
        when: null !== plasmoidContainer.applet
        restoreMode: Binding.RestoreBinding
    }
}
