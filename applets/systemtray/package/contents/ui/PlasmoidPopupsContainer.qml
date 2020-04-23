/*
 *   Copyright 2015 Marco Martin <mart@kde.org>
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

import QtQuick 2.4
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
//needed for units
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

StackView {
    id: mainStack
    clip: true
    focus: true

    Layout.minimumWidth: units.gridUnit * 12
    Layout.minimumHeight: units.gridUnit * 12

    property Item activeApplet

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

            if (activeApplet.fullRepresentationItem instanceof PlasmaComponents3.Page) {
                if (activeApplet.fullRepresentationItem.header && activeApplet.fullRepresentationItem.header instanceof PlasmaExtras.PlasmoidHeading) {
                    mainStack.appletHasHeading = true
                    activeApplet.fullRepresentationItem.header.background.visible = false
                }
                if (activeApplet.fullRepresentationItem.footer && activeApplet.fullRepresentationItem.footer instanceof PlasmaExtras.PlasmoidHeading) {
                    mainStack.appletHasFooter = true
                    activeApplet.fullRepresentationItem.footer.background.visible = false
                }
            }

            mainStack.replace({item: activeApplet.fullRepresentationItem, immediate: !dialog.visible, properties: {focus: true}});
        } else {
            mainStack.replace(emptyPage);
        }
    }
    Connections {
        target: plasmoid
        onAppletRemoved: {
            if (applet == root.activeApplet) {
                mainStack.clear()
            }
        }
    }
    //used to animate away to nothing
    Item {
        id: emptyPage
    }

    delegate: StackViewDelegate {
        function transitionFinished(properties) {
            properties.exitItem.opacity = 1
        }
        replaceTransition: StackViewTransition {
            ParallelAnimation {
                PropertyAnimation {
                    target: enterItem
                    property: "x"
                    from: enterItem.width
                    to: 0
                    duration: units.longDuration
                }
                PropertyAnimation {
                    target: enterItem
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: units.longDuration
                }
            }
            ParallelAnimation {
                PropertyAnimation {
                    target: exitItem
                    property: "x"
                    from: 0
                    to: -exitItem.width
                    duration: units.longDuration
                }
                PropertyAnimation {
                    target: exitItem
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: units.longDuration
                }
            }
        }
    }
}
