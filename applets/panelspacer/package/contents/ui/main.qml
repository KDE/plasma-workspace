/*
 * Copyright 2014 Marco Martin <mart@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You chenterX have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.10 as Kirigami

Item {
    id: root

    property bool horizontal: plasmoid.formFactor !== PlasmaCore.Types.Vertical

    Layout.fillWidth: plasmoid.configuration.expanding
    Layout.fillHeight: plasmoid.configuration.expanding

    Layout.minimumWidth: plasmoid.nativeInterface.containment.editMode ? units.gridUnit * 2 : 1
    Layout.minimumHeight: plasmoid.nativeInterface.containment.editMode ? units.gridUnit * 2 : 1
    Layout.preferredWidth: horizontal
        ? (plasmoid.configuration.expanding ? optimalSize : plasmoid.configuration.length)
        : 0
    Layout.preferredHeight: horizontal
        ? 0
        : (plasmoid.configuration.expanding ? optimalSize : plasmoid.configuration.length)

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation

    property int optimalSize: units.largeSpacing

    function action_expanding() {
        plasmoid.configuration.expanding = plasmoid.action("expanding").checked;
    }

    // Search the actual gridLayout of the panel
    property GridLayout panelLayout: {
        var candidate = root.parent;
        while (candidate) {
            if (candidate instanceof GridLayout) {
                return candidate;
            }
            candidate = candidate.parent;
        }
    }

    Component.onCompleted: {
        plasmoid.setAction("expanding", i18n("Set flexible size"));
        var action = plasmoid.action("expanding");
        action.checkable = true;
        action.checked = Qt.binding(function() {return plasmoid.configuration.expanding});

        plasmoid.removeAction("configure");
    }

    property real leftItemsSizeHint: 0
    property real rightItemsSizeHint: 0
    property real middleItemsSizeHint: {
        // Every time this binding gets reevaluated we want to queue a recomputation of the size hints
        Qt.callLater(root.updateHints)
        if (!twinSpacer || !panelLayout || !leftTwin || !rightTwin) {
            return 0;
        }

        var leftTwinParent = leftTwin.parent;
        var rightTwinParent = rightTwin.parent;
        if (!leftTwinParent || !rightTwinParent) {
            return 0;
        }
        var firstSpacerFound = false;
        var secondSpacerFound = false;
        var leftItemsHint = 0;
        var middleItemsHint = 0;
        var rightItemsHint = 0;

        // Children order is guaranteed to be the same as the visual order of items in the layout
        for (var i in panelLayout.children) {
            var child = panelLayout.children[i];
            if (!child.visible) {
                continue;
            } else if (child == leftTwinParent) {
                firstSpacerFound = true;
            } else if (child == rightTwinParent) {
                secondSpacerFound = true;
            } else if (secondSpacerFound) {
                if (root.horizontal) {
                    rightItemsHint += Math.min(child.Layout.maximumWidth, Math.max(child.Layout.minimumWidth, child.Layout.preferredWidth)) + panelLayout.rowSpacing;
                } else {
                    rightItemsHint += Math.min(child.Layout.maximumWidth, Math.max(child.Layout.minimumHeight, child.Layout.preferredHeight)) + panelLayout.columnSpacing;
                }
            } else if (firstSpacerFound) {
                if (root.horizontal) {
                    middleItemsHint += Math.min(child.Layout.maximumWidth, Math.max(child.Layout.minimumWidth, child.Layout.preferredWidth)) + panelLayout.rowSpacing;
                } else {
                    middleItemsHint += Math.min(child.Layout.maximumWidth, Math.max(child.Layout.minimumHeight, child.Layout.preferredHeight)) + panelLayout.columnSpacing;
                }
            } else {
                if (root.horizontal) {
                    leftItemsHint += Math.min(child.Layout.maximumWidth, Math.max(child.Layout.minimumWidth, child.Layout.preferredWidth)) + panelLayout.rowSpacing;
                } else {
                    leftItemsHint += Math.min(child.Layout.maximumHeight, Math.max(child.Layout.minimumHeight, child.Layout.preferredHeight)) + panelLayout.columnSpacing;
                }
            }
        }

        rightItemsSizeHint = rightItemsHint;
        leftItemsSizeHint = leftItemsHint;
        return middleItemsHint;
    }

    readonly property Item twinSpacer: plasmoid.configuration.expanding && plasmoid.nativeInterface.twinSpacer && plasmoid.nativeInterface.twinSpacer.configuration.expanding ? plasmoid.nativeInterface.twinSpacer : null
    readonly property Item leftTwin: {
        if (!twinSpacer) {
            return null;
        }

        if (root.horizontal) {
            return root.Kirigami.ScenePosition.x < twinSpacer.Kirigami.ScenePosition.x ? plasmoid : twinSpacer;
        } else {
            return root.Kirigami.ScenePosition.y < twinSpacer.Kirigami.ScenePosition.y ? plasmoid : twinSpacer;
        }
    }
    readonly property Item rightTwin: {
        if (!twinSpacer) {
            return null;
        }

        if (root.horizontal) {
            return root.Kirigami.ScenePosition.x >= twinSpacer.Kirigami.ScenePosition.x ? plasmoid : twinSpacer;
        } else {
            return root.Kirigami.ScenePosition.y >= twinSpacer.Kirigami.ScenePosition.y ? plasmoid : twinSpacer;
        }
    }

    function updateHints() {
        if (!twinSpacer || !panelLayout || !leftTwin || !rightTwin) {
            root.optimalSize = root.horizontal ? plasmoid.nativeInterface.containment.width : plasmoid.nativeInterface.containment.height;
            return;
        }

        var halfContainment = root.horizontal ?plasmoid.nativeInterface.containment.width/2 : plasmoid.nativeInterface.containment.height/2;

        if (leftTwin == plasmoid) {
            root.optimalSize = Math.max(units.smallSpacing, halfContainment - middleItemsSizeHint/2 - leftItemsSizeHint)
        } else {
            root.optimalSize = Math.max(units.smallSpacing, halfContainment - middleItemsSizeHint/2 - rightItemsSizeHint)
        }
    }
    

    Rectangle {
        anchors.fill: parent
        color: theme.highlightColor
        visible: plasmoid.nativeInterface.containment.editMode
    }
}
