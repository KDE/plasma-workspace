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

    z: 9999
    property bool horizontal: plasmoid.formFactor !== PlasmaCore.Types.Vertical

    Layout.fillWidth: plasmoid.configuration.expanding
    Layout.fillHeight: plasmoid.configuration.expanding

    Layout.minimumWidth: 1
    Layout.minimumHeight: 1
    Layout.preferredWidth: horizontal
        ? (plasmoid.configuration.expanding ? optimalSpace : plasmoid.configuration.length)
        : 0
    Layout.preferredHeight: horizontal
        ? 0
        : (plasmoid.configuration.expanding ? plasmoid.nativeInterface.containment.width : plasmoid.configuration.length)

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation

    function action_expanding() {
        plasmoid.configuration.expanding = plasmoid.action("expanding").checked;
    }
    Component.onCompleted: {
        plasmoid.setAction("expanding", i18n("Set flexible size"));
        var action = plasmoid.action("expanding");
        action.checkable = true;
        action.checked = plasmoid.configuration.expanding;

        plasmoid.removeAction("configure");
    }

    readonly property Item twinSpacer: plasmoid.nativeInterface.twinSpacer
    readonly property Item leftTwin: {
        if (!twinSpacer) {
            return null;
        }

        if (root.horizontal) {
            return root.Kirigami.ScenePosition.x < twinSpacer.Kirigami.ScenePosition.x ? root : twinSpacer;
        } else {
            return root.Kirigami.ScenePosition.y < twinSpacer.Kirigami.ScenePosition.y ? root : twinSpacer;
        }
    }
    readonly property Item rightTwin: {
        if (!twinSpacer) {
            return null;
        }

        if (root.horizontal) {
            return root.Kirigami.ScenePosition.x >= twinSpacer.Kirigami.ScenePosition.x ? root : twinSpacer;
        } else {
            return root.Kirigami.ScenePosition.y >= twinSpacer.Kirigami.ScenePosition.y ? root : twinSpacer;
        }
    }

    readonly property int containmentLeftSpace: twinSpacer
        ? (horizontal ? leftTwin.Kirigami.ScenePosition.x : leftTwin.Kirigami.ScenePosition.y)
        : -1
    readonly property int containmentRightSpace: twinSpacer
        ? ( horizontal
            ? plasmoid.nativeInterface.containment.width - rightTwin.Kirigami.ScenePosition.x - rightTwin.width
            : plasmoid.nativeInterface.containment.height - rightTwin.Kirigami.ScenePosition.y - rightTwin.height)
        : -1
    readonly property real misalignment: containmentRightSpace - containmentLeftSpace
    onWidthChanged: {
        if (twinSpacer) {
            relayoutTimer.restart();
        }
    }
    Timer {
        id: relayoutTimer
        interval: 0
        onTriggered: {
            if (!twinSpacer) {
                root.optimalSpace = plasmoid.nativeInterface.containment.width;
            }
            var chenterX = ((leftTwin.Kirigami.ScenePosition.x + leftTwin.width)+(plasmoid.nativeInterface.containment.width - rightTwin.Kirigami.ScenePosition.x))/2
            if (leftTwin == root) {
                root.optimalSpace =  Math.min(plasmoid.nativeInterface.containment.width, Math.max(0, chenterX - containmentLeftSpace))
            } else {
                root.optimalSpace =  Math.min(plasmoid.nativeInterface.containment.width, Math.max(0, chenterX - containmentRightSpace))
            }
        }
    }
    property int optimalSpace: plasmoid.nativeInterface.containment.width

    Rectangle {
        anchors.fill: parent
        color: theme.highlightColor
        visible: plasmoid.nativeInterface.containment.editMode
    }
}
