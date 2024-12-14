/*
    SPDX-FileCopyrightText: 2020 Konrad Materka <materka@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

//This object contains state of the SystemTray, mainly related to the 'expanded' state
QtObject {
    //true if System Tray is 'expanded'. It may be when:
    // - there is an active applet or
    // - 'Status and Notification' with hidden items is shown
    property bool expanded: false
    //set when there is an applet selected
    property Item activeApplet

    //allow expanded change only when activated at least once
    //this is to suppress expanded state change during Plasma startup
    property bool acceptExpandedChange: false

    // These properties allow us to keep track of where the expanded applet
    // was and is on the panel, allowing PlasmoidPopupContainer.qml to animate
    // depending on their locations.
    property int oldVisualIndex: -1
    property int newVisualIndex: -1

    function setActiveApplet(applet, visualIndex) {

        // Applets which prefer to always show their full
        // representation will always be expanded, there's
        // no need to activate them.
        if (applet && applet.preferredRepresentation == applet.fullRepresentation) return;

        if (visualIndex === undefined) {
            oldVisualIndex = -1
            newVisualIndex = -1
        } else {
            oldVisualIndex = (activeApplet && activeApplet.status === PlasmaCore.Types.PassiveStatus) ? 9999 : newVisualIndex
            newVisualIndex = visualIndex
        }

        const oldApplet = activeApplet
        if (applet && !applet.preferredRepresentation) {
            applet.expanded = true;
        }
        if (!applet || !applet.preferredRepresentation) {
            activeApplet = applet;
        }

        if (oldApplet && oldApplet !== applet) {
            oldApplet.expanded = false
        }

        if (applet && !applet.preferredRepresentation) {
            expanded = true
        }
    }

    onExpandedChanged: {
        if (expanded) {
            Plasmoid.status = PlasmaCore.Types.RequiresAttentionStatus
        } else {
            Plasmoid.status = PlasmaCore.Types.PassiveStatus;
            if (activeApplet) {
                // if not expanded we don't have an active applet anymore
                activeApplet.expanded = false
                activeApplet = null
            }
        }
        acceptExpandedChange = false
        root.expanded = expanded
    }

    //listen on SystemTray AppletInterface signals
    readonly property Connections plasmoidConnections: Connections {
        target: Plasmoid
        //emitted when activation is requested, for example by using a global keyboard shortcut
        function onActivated() {
            acceptExpandedChange = true
        }
    }

    readonly property Connections rootConnections: Connections {
        function onExpandedChanged() {
            if (acceptExpandedChange) {
                expanded = root.expanded
            } else {
                root.expanded = expanded
            }
        }
    }

    readonly property Connections activeAppletConnections: Connections {
        target: activeApplet && activeApplet

        function onExpandedChanged() {
            if (activeApplet && !activeApplet.expanded) {
                expanded = false
            }
        }
    }
}
