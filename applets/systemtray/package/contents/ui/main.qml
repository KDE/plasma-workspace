/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
// import org.kde.plasma.components 2.0 as PlasmaComponents
// import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.private.systemtray 2.0 as SystemTray
import "Layout.js" as LayoutManager

Item {
    id: root
    objectName: "SystemTrayRootItem"

    property bool vertical: (plasmoid.formFactor == PlasmaCore.Types.Vertical)

    Plasmoid.toolTipMainText: ""
    Plasmoid.toolTipSubText: ""

    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.onExpandedChanged: {
        if (!plasmoid.expanded && root.expandedTask) {
            root.expandedTask.expanded = false;
            root.expandedTask = null;
        }
    }

    property int preferredItemSize: 128 // will be set by the grid, just needs a high-enough default

    // Sizes depend on the font, and thus on DPI
    property int baseSize: theme.mSize(theme.defaultFont).height
    property int itemSize: LayoutManager.alignedSize(Math.min(baseSize * 2, preferredItemSize))

    property bool debug: plasmoid.configuration.debug

    //a pointer to the Task* that is the current one expanded, that shows full ui
    property QtObject expandedTask: null;

    function togglePopup() {
        print("toggle popup => " + !plasmoid.expanded);
        if (!plasmoid.expanded) {
            plasmoid.expanded = true
        } else {
            //hidePopupTimer.start();
        }
    }

    Plasmoid.compactRepresentation: CompactRepresentation {
        systrayhost: host
    }
    Plasmoid.fullRepresentation: ExpandedRepresentation {
        Layout.minimumWidth: Layout.minimumHeight * 1.75
        Layout.minimumHeight: units.gridUnit * 14
        Layout.preferredWidth: Layout.minimumWidth
        Layout.preferredHeight: Layout.minimumHeight * 1.5
    }

    Rectangle {
        anchors.fill: parent;
        border.width: 2;
        border.color: "black";
        color: "blue";
        visible: root.debug;
        opacity: 0.2;
    }
    
    SystemTray.Host {
        id: host
        rootItem: plasmoid
    }

}