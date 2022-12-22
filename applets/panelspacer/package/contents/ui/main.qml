/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.10 as Kirigami

Item {
    id: root

    property bool horizontal: Plasmoid.formFactor !== PlasmaCore.Types.Vertical

    Layout.fillWidth: Plasmoid.configuration.expanding
    Layout.fillHeight: Plasmoid.configuration.expanding

    Layout.minimumWidth: Plasmoid.nativeInterface.containment.editMode ? PlasmaCore.Units.gridUnit * 2 : 1
    Layout.minimumHeight: Plasmoid.nativeInterface.containment.editMode ? PlasmaCore.Units.gridUnit * 2 : 1
    Layout.preferredWidth: horizontal
        ? (Plasmoid.configuration.expanding ? optimalSize : Plasmoid.configuration.length)
        : 0
    Layout.preferredHeight: horizontal
        ? 0
        : (Plasmoid.configuration.expanding ? optimalSize : Plasmoid.configuration.length)

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation

    function action_expanding() {
        Plasmoid.configuration.expanding = Plasmoid.action("expanding").checked;
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
        return null;
    }

    Component.onCompleted: {
        Plasmoid.setAction("expanding", i18n("Set flexible size"));
        var action = Plasmoid.action("expanding");
        action.checkable = true;
        action.checked = Qt.binding(function() {return Plasmoid.configuration.expanding});

        Plasmoid.removeAction("configure");
    }

    property real optimalSize: {
        if (!panelLayout || !Plasmoid.configuration.expanding) return Plasmoid.configuration.length;
        let expandingSpacers = 0;
        let thisSpacerIndex = null;
        let sizeHints = [0];
        // Children order is guaranteed to be the same as the visual order of items in the layout
        for (var i in panelLayout.children) {
            var child = panelLayout.children[i];
            if (!child.visible) continue;

            if (child.applet && child.applet.pluginName === 'org.kde.plasma.panelspacer' && child.applet.configuration.expanding) {
                if (child === Plasmoid.parent) {
                    thisSpacerIndex = expandingSpacers
                }
                sizeHints.push(0)
                expandingSpacers += 1
            } else if (root.horizontal) {
                sizeHints[sizeHints.length - 1] += Math.min(child.Layout.maximumWidth, Math.max(child.Layout.minimumWidth, child.Layout.preferredWidth)) + panelLayout.rowSpacing;
            } else {
                sizeHints[sizeHints.length - 1] += Math.min(child.Layout.maximumWidth, Math.max(child.Layout.minimumHeight, child.Layout.preferredHeight)) + panelLayout.columnSpacing;
            }
        }
        sizeHints[0] *= 2; sizeHints[sizeHints.length - 1] *= 2
        let opt = Plasmoid.nativeInterface.containment.width / expandingSpacers - sizeHints[thisSpacerIndex] / 2 - sizeHints[thisSpacerIndex + 1] / 2
        return Math.max(opt, 0)
    }

    Rectangle {
        anchors.fill: parent
        color: PlasmaCore.Theme.highlightColor
        opacity: Plasmoid.nativeInterface.containment.editMode ? 1 : 0
        visible: Plasmoid.nativeInterface.containment.editMode || animator.running

        Behavior on opacity {
            NumberAnimation {
                id: animator
                duration: PlasmaCore.Units.longDuration
                // easing.type is updated after animation starts
                easing.type: Plasmoid.nativeInterface.containment.editMode ? Easing.InCubic : Easing.OutCubic
            }
        }
    }
}
