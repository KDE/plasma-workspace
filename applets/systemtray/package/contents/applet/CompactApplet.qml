/*
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0


PlasmaCore.ToolTipArea {
    id: appletRoot
    objectName: "org.kde.desktop-CompactApplet"
    anchors.fill: parent

    mainText: Plasmoid.toolTipMainText
    subText: Plasmoid.toolTipSubText
    location: if (Plasmoid.parent && Plasmoid.parent.inHiddenLayout && Plasmoid.location !== PlasmaCore.Types.LeftEdge) {
        return PlasmaCore.Types.RightEdge;
    } else {
        return Plasmoid.location;
    }
    active: !Plasmoid.expanded
    textFormat: Plasmoid.toolTipTextFormat
    mainItem: Plasmoid.toolTipItem

    property Item fullRepresentation
    property Item compactRepresentation

    Connections {
        target: Plasmoid.self
        function onContextualActionsAboutToShow() {
            appletRoot.hideImmediately()
        }
    }

    Layout.minimumWidth: {
        switch (Plasmoid.formFactor) {
        case PlasmaCore.Types.Vertical:
            return 0;
        case PlasmaCore.Types.Horizontal:
            return height;
        default:
            return PlasmaCore.Units.gridUnit * 3;
        }
    }

    Layout.minimumHeight: {
        switch (Plasmoid.formFactor) {
        case PlasmaCore.Types.Vertical:
            return width;
        case PlasmaCore.Types.Horizontal:
            return 0;
        default:
            return PlasmaCore.Units.gridUnit * 3;
        }
    }

    onCompactRepresentationChanged: {
        if (compactRepresentation) {
            compactRepresentation.parent = appletRoot;
            compactRepresentation.anchors.fill = appletRoot;
            compactRepresentation.visible = true;
        }
        appletRoot.visible = true;
    }
}

