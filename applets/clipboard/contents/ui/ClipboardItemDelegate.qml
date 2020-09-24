/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright     2014 Sebastian Kügler <sebas@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

PlasmaExtras.ListItem {
    id: menuItem

    property bool supportsBarcodes
    property int maximumNumberOfPreviews: Math.floor(width / (PlasmaCore.Units.gridUnit * 4 + PlasmaCore.Units.smallSpacing))
    readonly property real gradientThreshold: (label.width - toolButtonsLoader.width) / label.width

    signal itemSelected(string uuid)
    signal remove(string uuid)
    signal edit(string uuid)
    signal barcode(string text)
    signal action(string uuid)

    // the 1.6 comes from ToolButton's default height
    height: Math.max(label.height, Math.round(PlasmaCore.Units.gridUnit * 1.6)) + 2 * PlasmaCore.Units.smallSpacing

    enabled: true

    onClicked: {
        menuItem.itemSelected(UuidRole);
        if (plasmoid.hideOnWindowDeactivate)
            plasmoid.expanded = false;
    }
    onContainsMouseChanged: {
        if (containsMouse) {
            menuListView.currentIndex = index
        } else {
            menuListView.currentIndex = -1
        }
    }

    ListView.onIsCurrentItemChanged: {
        if (ListView.isCurrentItem) {
            labelMask.source = label // calculate on demand
        }
    }

    // this stuff here is used so we can fade out the text behind the tool buttons
    Item {
        id: labelMaskSource
        anchors.fill: label
        visible: false

        Rectangle {
            anchors.centerIn: parent
            rotation: LayoutMirroring.enabled ? 90 : -90 // you cannot even rotate gradients without QtGraphicalEffects
            width: parent.height
            height: parent.width

            gradient: Gradient {
                GradientStop { position: 0.0; color: "white" }
                GradientStop { position: gradientThreshold - 0.25; color: "white"}
                GradientStop { position: gradientThreshold; color: "transparent"}
                GradientStop { position: 1; color: "transparent"}
            }
        }
    }

    OpacityMask {
        id: labelMask
        anchors.fill: label
        cached: true
        maskSource: labelMaskSource
        visible: !!source && menuItem.ListView.isCurrentItem
    }

    Item {
        id: label
        height: childrenRect.height
        visible: !menuItem.ListView.isCurrentItem
        anchors {
            left: parent.left
            leftMargin: PlasmaCore.Units.gridUnit / 2 - listMargins.left
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        Loader {
            width: parent.width
            source: ["Text", "Image", "Url"][TypeRole] + "ItemDelegate.qml"
        }
    }

    Loader {
        id: toolButtonsLoader
        anchors {
            right: label.right
            verticalCenter: parent.verticalCenter
        }
        source: "DelegateToolButtons.qml"
        active: menuItem.ListView.isCurrentItem
        onActiveChanged: {
            if (active) {
                // break binding, once it was loaded, never unload
                active = true;
            }
        }
    }
}
