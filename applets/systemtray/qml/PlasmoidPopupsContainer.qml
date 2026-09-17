/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid

QQC2.StackView {
    id: mainStack
    focus: true

    Layout.minimumWidth: Kirigami.Units.gridUnit * 12
    Layout.minimumHeight: Kirigami.Units.gridUnit * 12

    readonly property Item activeApplet: systemTrayState.activeApplet

    /* Heading */
    property bool appletHasHeading: false
    property bool mergeHeadings: appletHasHeading && activeApplet.fullRepresentationItem.header.visible
    property int headingHeight: mergeHeadings ? activeApplet.fullRepresentationItem.header.height : 0
    /* Footer */
    property bool appletHasFooter: false
    property bool mergeFooters: appletHasFooter && activeApplet.fullRepresentationItem.footer.visible
    property int footerHeight: mergeFooters ? activeApplet.fullRepresentationItem.footer.height : 0

    function _detectAndConfigureHeaderAndFooter() {
        // FIXME: this is fairly meh. We explicitly set a visibility here, breaking bindings, and doing this from the
        //        outside, imperatively, is impossible to discover from the other side, making it hard to debug.
        //        What should happen is that the PlasmoidHeading adapts to its surrounding, not the surrounding forcing
        //        things on the PlasmoidHeading.

        let findFirstChildOfType = function(root, type, depth = 0) {
            if (depth > 5) { // Arbitrary depth limit to avoid spending hours on not finding anything
                return null
            }

            if (root instanceof type) {
                return root
            }

            // Look at all children before recursing. This optimizes for the common case of finding the Heading near the top of the hierarchy.
            for (const child of root.children) {
                if (child instanceof type) {
                    return child
                }
            }

            for (const child of root.children) {
                const result = findFirstChildOfType(child, type, depth + 1)
                if (result) {
                    return result
                }
            }

            return null
        }

        // The PlasmoidHeadings may be inside a Layout so we need to check recursively. Making this even more awkward!

        const headerHeading = findFirstChildOfType(activeApplet.fullRepresentationItem.header, PlasmaExtras.PlasmoidHeading)
        if (headerHeading) {
            // Detection
            mainStack.appletHasHeading = true
            // Configuration
            headerHeading.background.visible = false
        }

        const footerHeading = findFirstChildOfType(activeApplet.fullRepresentationItem.footer, PlasmaExtras.PlasmoidHeading)
        if (footerHeading) {
            // Detection
            mainStack.appletHasFooter = true
            // Configuration
            footerHeading.background.visible = false
        }
    }

    onActiveAppletChanged: {
        mainStack.appletHasHeading = false
        mainStack.appletHasFooter = false

        if (activeApplet != null && activeApplet.fullRepresentationItem && !activeApplet.preferredRepresentation) {
            //reset any potential anchor
            activeApplet.fullRepresentationItem.anchors.left = undefined;
            activeApplet.fullRepresentationItem.anchors.top = undefined;
            activeApplet.fullRepresentationItem.anchors.right = undefined;
            activeApplet.fullRepresentationItem.anchors.bottom = undefined;
            activeApplet.fullRepresentationItem.anchors.centerIn = undefined;
            activeApplet.fullRepresentationItem.anchors.fill = undefined;

            _detectAndConfigureHeaderAndFooter()

            let unFlipped = systemTrayState.oldVisualIndex < systemTrayState.newVisualIndex;
            if (Application.layoutDirection !== Qt.LeftToRight) {
                unFlipped = !unFlipped;
            }

            const isTransitionEnabled = systemTrayState.expanded;
            (mainStack.empty ? mainStack.push : mainStack.replace)(activeApplet.fullRepresentationItem, {
                "width": Qt.binding(() => mainStack.width),
                "height": Qt.binding(() => mainStack.height),
                "x": 0,
                "focus": Qt.binding(() => !mainStack.busy), // QTBUG-44043: retrigger binding after StackView is ready to restore focus
                "opacity": 1,
                "KeyNavigation.up": mainStack.KeyNavigation.up,
                "KeyNavigation.backtab": mainStack.KeyNavigation.backtab,
            }, isTransitionEnabled ? (unFlipped ? QQC2.StackView.PushTransition : QQC2.StackView.PopTransition) : QQC2.StackView.Immediate);
        } else {
            mainStack.clear();
        }
    }

    onCurrentItemChanged: {
        if (currentItem !== null && root.expanded) {
            currentItem.forceActiveFocus();
        }
    }

    Connections {
        target: Plasmoid
        function onAppletRemoved(applet) {
            if (applet === systemTrayState.activeApplet) {
                mainStack.clear();
            }
        }
    }
}
