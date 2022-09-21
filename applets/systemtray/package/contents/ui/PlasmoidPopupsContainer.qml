/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
//needed for units
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.plasmoid 2.0

StackView {
    id: mainStack
    focus: true

    Layout.minimumWidth: PlasmaCore.Units.gridUnit * 12
    Layout.minimumHeight: PlasmaCore.Units.gridUnit * 12

    readonly property Item activeApplet: systemTrayState.activeApplet

    /* Heading */
    property bool appletHasHeading: false
    property bool mergeHeadings: appletHasHeading && activeApplet.fullRepresentationItem.header.visible
    property int headingHeight: mergeHeadings ? activeApplet.fullRepresentationItem.header.height : 0
    /* Footer */
    property bool appletHasFooter: false
    property bool mergeFooters: appletHasFooter && activeApplet.fullRepresentationItem.footer.visible
    property int footerHeight: mergeFooters ? activeApplet.fullRepresentationItem.footer.height : 0

    onActiveAppletChanged: {
        mainStack.appletHasHeading = false
        mainStack.appletHasFooter = false
        if (activeApplet != null) {
            //reset any potential anchor
            activeApplet.fullRepresentationItem.anchors.left = undefined;
            activeApplet.fullRepresentationItem.anchors.top = undefined;
            activeApplet.fullRepresentationItem.anchors.right = undefined;
            activeApplet.fullRepresentationItem.anchors.bottom = undefined;
            activeApplet.fullRepresentationItem.anchors.centerIn = undefined;
            activeApplet.fullRepresentationItem.anchors.fill = undefined;

            if (activeApplet.fullRepresentationItem instanceof PlasmaComponents3.Page ||
                activeApplet.fullRepresentationItem instanceof PlasmaExtras.Representation) {
                if (activeApplet.fullRepresentationItem.header && activeApplet.fullRepresentationItem.header instanceof PlasmaExtras.PlasmoidHeading) {
                    mainStack.appletHasHeading = true
                    activeApplet.fullRepresentationItem.header.background.visible = false
                }
                if (activeApplet.fullRepresentationItem.footer && activeApplet.fullRepresentationItem.footer instanceof PlasmaExtras.PlasmoidHeading) {
                    mainStack.appletHasFooter = true
                    activeApplet.fullRepresentationItem.footer.background.visible = false
                }
            }

            let unFlipped = systemTrayState.oldVisualIndex < systemTrayState.newVisualIndex;
            if (Qt.application.layoutDirection !== Qt.LeftToRight) {
                unFlipped = !unFlipped;
            }

            const isTransitionEnabled = systemTrayState.expanded;
            (mainStack.empty ? mainStack.push : mainStack.replace)(activeApplet.fullRepresentationItem, {
                "width": Qt.binding(() => mainStack.width),
                "height": Qt.binding(() => mainStack.height),
                "x": 0,
                "focus": true,
                "opacity": 1,
                "KeyNavigation.up": mainStack.KeyNavigation.up,
                "KeyNavigation.backtab": mainStack.KeyNavigation.backtab,
            }, isTransitionEnabled ? (unFlipped ? StackView.PushTransition : StackView.PopTransition) : StackView.Immediate);
        } else {
            mainStack.clear();
        }
    }

    onCurrentItemChanged: {
        if (currentItem !== null && Plasmoid.expanded) {
            currentItem.forceActiveFocus();
        }
    }

    Connections {
        target: Plasmoid.self
        function onAppletRemoved(applet) {
            if (applet === systemTrayState.activeApplet) {
                mainStack.clear();
            }
        }
    }
}
