/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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

    Layout.minimumWidth: plasmoid.nativeInterface.containment.editMode ? PlasmaCore.Units.gridUnit * 2 : 1
    Layout.minimumHeight: plasmoid.nativeInterface.containment.editMode ? PlasmaCore.Units.gridUnit * 2 : 1
    Layout.preferredWidth: horizontal
        ? (plasmoid.configuration.expanding ? optimalSize : plasmoid.configuration.length)
        : 0
    Layout.preferredHeight: horizontal
        ? 0
        : (plasmoid.configuration.expanding ? optimalSize : plasmoid.configuration.length)

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation

    property int optimalSize: PlasmaCore.Units.largeSpacing

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

    property real middleItemsSizeHint: {
        if (!twinSpacer || !panelLayout || !leftTwin || !rightTwin) {
            optimalSize = horizontal ? plasmoid.nativeInterface.containment.width : plasmoid.nativeInterface.containment.height;
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

        var halfContainment = root.horizontal ?plasmoid.nativeInterface.containment.width/2 : plasmoid.nativeInterface.containment.height/2;

        if (leftTwin == plasmoid) {
            optimalSize = Math.max(PlasmaCore.Units.smallSpacing, halfContainment - middleItemsHint/2 - leftItemsHint)
        } else {
            optimalSize = Math.max(PlasmaCore.Units.smallSpacing, halfContainment - middleItemsHint/2 - rightItemsHint)
        }
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

    Rectangle {
        anchors.fill: parent
        color: PlasmaCore.Theme.highlightColor
        visible: plasmoid.nativeInterface.containment.editMode
    }
}
