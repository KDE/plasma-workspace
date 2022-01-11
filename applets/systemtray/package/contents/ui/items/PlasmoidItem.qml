/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.1
import QtQml 2.15

import org.kde.plasma.core 2.0 as PlasmaCore

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
    // plasmoid. Not all plasmoids are supposed to expand and not all plasmoids
    // do anything with onActivated.
    onActivated: if (applet) {
        let fullRep = applet.fullRepresentationItem
        /* HACK: Plasmoids can have an empty but not null fullRepresentationItem,
         * even if fullRepresentation is not explicitly defined or is explicitly null.
         *
         * If fullRep is a plain Item and there are no children, assume it is empty.
         * There will be uncommon situations where this assumption is wrong.
         *
         * `typeof fullRep` only returns "object", which is useless.
         * We aren't using `fullRep instanceof Item` because it would always
         * return true if fullRep is not null.
         * If fullRep.toString() starts with "QQuickItem_QML",
         * then it really is just a plain Item.
         *
         * We really need to refactor system tray someday.
         */
        if (fullRep && (!fullRep.toString().startsWith("QQuickItem_QML")
                        || fullRep.children.length > 0)
        ) {
            // Assume that an applet with a fullRepresentationItem that
            // fits the criteria will want to expand the applet when activated.
            applet.expanded = !applet.expanded
        }
        // If there is no fullRepresentationItem, hopefully the applet is using
        // the onActivated signal handler for something useful.
        applet.activated()
    }

    onClicked: {
        if (!applet) {
            return
        }
        //forward click event to the applet
        const mouseArea = findMouseArea(applet.compactRepresentationItem)
        if (mouseArea) {
            mouseArea.clicked(mouse)
        } else if (mouse.button === Qt.LeftButton) {//falback
            plasmoidContainer.activated()
        }
    }
    onContextMenu: if (applet) {
        plasmoid.nativeInterface.showPlasmoidMenu(applet, 0,
                                                  plasmoidContainer.inHiddenLayout ? applet.height : 0);
    }
    onWheel: {
        if (!applet) {
            return
        }
        //forward wheel event to the applet
        const mouseArea = findMouseArea(applet.compactRepresentationItem)
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
            fullRepresentationItem.width = expandedRepresentation.height
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
        target: applet

        //activation using global keyboard shortcut
        function onActivated() {
            plasmoidContainer.startActivatedAnimation()
        }

        function onExpandedChanged(expanded) {
            if (expanded) {
                systemTrayState.setActiveApplet(applet, model.row)
                plasmoidContainer.startActivatedAnimation()
            }
        }

        function onFullRepresentationItemChanged(fullRepresentationItem) {
            preloadFullRepresentationItem(fullRepresentationItem)
        }
    }

    Binding {
        property: "hideOnWindowDeactivate"
        value: !plasmoid.configuration.pin
        target: plasmoidContainer.applet
        when: null !== plasmoidContainer.applet
        restoreMode: Binding.RestoreBinding
    }
}
