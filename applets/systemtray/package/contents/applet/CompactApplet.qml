/*
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

import "../ui/items" as Items

PlasmaCore.ToolTipArea {
    id: appletRoot
    objectName: "org.kde.desktop-CompactApplet"
    anchors.fill: parent

    mainText: plasmoidItem?.toolTipMainText ?? ""
    subText: plasmoidItem?.toolTipSubText ?? ""
    location: if ((plasmoidItem?.parent as Items.AbstractItem)?.inHiddenLayout && Plasmoid.location !== PlasmaCore.Types.LeftEdge) {
        return PlasmaCore.Types.RightEdge;
    } else {
        return Plasmoid.location;
    }
    active: plasmoidItem ? !plasmoidItem.expanded : false
    textFormat: plasmoidItem?.toolTipTextFormat ?? Text.AutoText
    mainItem: plasmoidItem?.toolTipItem ?? null

    property Item fullRepresentation
    property Item compactRepresentation
    property PlasmoidItem plasmoidItem

    Connections {
        target: Plasmoid
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
            return Kirigami.Units.gridUnit * 3;
        }
    }

    Layout.minimumHeight: {
        switch (Plasmoid.formFactor) {
        case PlasmaCore.Types.Vertical:
            return width;
        case PlasmaCore.Types.Horizontal:
            return 0;
        default:
            return Kirigami.Units.gridUnit * 3;
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

