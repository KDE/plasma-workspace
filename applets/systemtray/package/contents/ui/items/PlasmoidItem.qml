/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kirigami as Kirigami

import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.plasmoid

AbstractItem {
    id: plasmoidContainer

    property Item applet: model.applet ?? null
    text: applet?.plasmoid.title ?? ""

    itemId: applet?.plasmoid.pluginName ?? ""
    mainText: inVisibleLayout || (applet && applet.toolTipMainText && applet.toolTipMainText != text) ? applet.toolTipMainText : ""
    subText: applet?.toolTipSubText ?? ""
    mainItem: applet?.toolTipItem ?? null
    textFormat: applet?.toolTipTextFormat ?? 0 /* Text.AutoText, the default value */
    active: inVisibleLayout || (systemTrayState.activeApplet !== applet && (text != mainText || subText.length > 0))

    // FIXME: Use an input type agnostic way to activate whatever the primary
    // action of a plasmoid is supposed to be, even if it's just expanding the
    // Plasmoid. Not all plasmoids are supposed to expand and not all plasmoids
    // do anything with onActivated.
    onActivated: pos => {
        if (applet) {
            applet.plasmoid.activated()
        }
    }

    onClicked: mouse => {
        if (!applet) {
            return
        }
        //forward click event to the applet
        const appletItem = applet.compactRepresentationItem ?? applet.fullRepresentationItem
        const mouseArea = findMouseArea(appletItem)

        if (mouseArea && mouse.button !== Qt.RightButton) {
            mouseArea.clicked(mouse)
        } else if (mouse.button === Qt.LeftButton) {//falback
            activated(null)
        }
    }
    onPressed: mouse => {
        // Only Plasmoids can show context menu on the mouse pressed event.
        // SNI has few problems, for example legacy applications that still use XEmbed require mouse to be released.
        if (mouse.button === Qt.RightButton) {
            contextMenu(mouse);
        } else {
            const appletItem = applet.compactRepresentationItem ?? applet.fullRepresentationItem
            const mouseArea = findMouseArea(appletItem)
            if (mouseArea) {
                // The correct way here would be to invoke the "pressed"
                // signal; however, mouseArea.pressed signal is overridden
                // by its bool value, and our only option is to call the
                // handler directly.
                mouseArea.onPressed(mouse)
            }
        }
    }
    onContextMenu: mouse => {
        if (applet) {
            effectivePressed = false;
            Plasmoid.showPlasmoidMenu(applet, 0, inHiddenLayout ? applet.height : 0);
        }
    }
    onWheel: wheel => {
        if (!applet) {
            return
        }
        //forward wheel event to the applet
        const appletItem = applet.compactRepresentationItem ?? applet.fullRepresentationItem
        const mouseArea = findMouseArea(appletItem)
        if (mouseArea) {
            mouseArea.wheel(wheel)
        }
    }

    function __isSuitableMouseArea(child: Item): bool {
        const item = child.parent;
        return child instanceof MouseArea
            && child.enabled
            // check if MouseArea covers the entire item
            && (child.anchors.fill === item
                || (child.x === 0
                    && child.y === 0
                    && child.width === item.width
                    && child.height === item.height));
    }

    //some heuristics to find MouseArea
    function findMouseArea(item: Item): MouseArea {
        if (!item) {
            return null
        }

        if (item instanceof MouseArea) {
            return item
        }

        return item.children.find(__isSuitableMouseArea) ?? null;
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
            applet.parent = iconContainer
            applet.anchors.fill = applet.parent
            applet.visible = true

            preloadFullRepresentationItem(applet.fullRepresentationItem)
        }
    }

    Connections {
        enabled: plasmoidContainer.applet !== null
        target: findMouseArea(
            plasmoidContainer.applet?.compactRepresentationItem ??
            plasmoidContainer.applet?.fullRepresentationItem ??
            plasmoidContainer.applet
        )

        function onContainsPressChanged() {
            plasmoidContainer.effectivePressed = target.containsPress;
        }

        // TODO For touch/stylus only, since the feature is not desired for mouse users
        function onPressAndHold(mouse) {
            if (mouse.button === Qt.LeftButton) {
                plasmoidContainer.contextMenu(mouse)
            }
        }
    }

    Connections {
        target: plasmoidContainer.applet?.plasmoid ?? null

        //activation using global keyboard shortcut
        function onActivated() {
            plasmoidContainer.effectivePressed = true;
            Qt.callLater(() => {
                plasmoidContainer.effectivePressed = false;
            });
        }
    }

    Connections {
        target: plasmoidContainer.applet

        function onFullRepresentationItemChanged(fullRepresentationItem) {
            preloadFullRepresentationItem(fullRepresentationItem)
        }

        function onExpandedChanged(expanded) {
            if (expanded) {
                effectivePressed = false;
            }
        }
    }

    PlasmaComponents3.BusyIndicator {
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            leftMargin: inHiddenLayout ? Math.round(Kirigami.Units.smallSpacing / 2) : 0
            right: inHiddenLayout ? undefined : parent.right
        }
        z: 999
        running: plasmoidContainer.applet?.plasmoid.busy ?? false
    }

    Binding {
        property: "hideOnWindowDeactivate"
        value: !Plasmoid.configuration.pin
        target: plasmoidContainer.applet
        when: plasmoidContainer.applet !== null
        restoreMode: Binding.RestoreBinding
    }
}
