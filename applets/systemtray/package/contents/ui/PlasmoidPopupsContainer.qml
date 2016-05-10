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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

StackView {
    id: mainStack
    clip: true
    focus: true
    Keys.forwardTo: [currentItem]

    Layout.minimumWidth: units.gridUnit * 12
    Layout.minimumHeight: units.gridUnit * 12

    property Item activeApplet

    onActiveAppletChanged: {
        if (activeApplet != null) {
            //reset any potential anchor
            activeApplet.fullRepresentationItem.anchors.left = undefined;
            activeApplet.fullRepresentationItem.anchors.top = undefined;
            activeApplet.fullRepresentationItem.anchors.right = undefined;
            activeApplet.fullRepresentationItem.anchors.bottom = undefined;
            activeApplet.fullRepresentationItem.anchors.centerIn = undefined;
            activeApplet.fullRepresentationItem.anchors.fill = undefined;

            mainStack.replace({item: activeApplet.fullRepresentationItem, immediate: !dialog.visible});
        } else {
            mainStack.replace(emptyPage);
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
