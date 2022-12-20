/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

PlasmaCore.ToolTipArea {
    id: tooltip

    readonly property int arrowAnimationDuration: PlasmaCore.Units.shortDuration
    property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    property int iconSize: PlasmaCore.Units.iconSizes.smallMedium
    implicitWidth: iconSize
    implicitHeight: iconSize
    activeFocusOnTab: true

    Accessible.name: i18n("Expand System Tray")
    Accessible.description: i18n("Show all the items in the system tray in a popup")
    Accessible.role: Accessible.Button

    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_Space:
        case Qt.Key_Enter:
        case Qt.Key_Return:
        case Qt.Key_Select:
            systemTrayState.expanded = !systemTrayState.expanded;
        }
    }

    subText: systemTrayState.expanded ? i18n("Close popup") : i18n("Show hidden icons")

    property bool wasExpanded

    TapHandler {
        onPressedChanged: {
            if (pressed) {
                tooltip.wasExpanded = systemTrayState.expanded;
            }
        }
        onTapped: {
            systemTrayState.expanded = !tooltip.wasExpanded;
            expandedRepresentation.hiddenLayout.currentIndex = -1;
        }
    }

    PlasmaCore.Svg {
        id: arrowSvg
        imagePath: "widgets/arrows"
    }

    PlasmaCore.SvgItem {
        id: arrow

        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height)
        height: width

        rotation: systemTrayState.expanded ? 180 : 0
        Behavior on rotation {
            RotationAnimation {
                duration: tooltip.arrowAnimationDuration
            }
        }
        opacity: systemTrayState.expanded ? 0 : 1
        Behavior on opacity {
            NumberAnimation {
                duration: tooltip.arrowAnimationDuration
            }
        }

        svg: arrowSvg
        elementId: {
            if (Plasmoid.location === PlasmaCore.Types.TopEdge) {
                return "down-arrow";
            } else if (Plasmoid.location === PlasmaCore.Types.LeftEdge) {
                return "right-arrow";
            } else if (Plasmoid.location === PlasmaCore.Types.RightEdge) {
                return "left-arrow";
            } else {
                return "up-arrow";
            }
        }
    }

    PlasmaCore.SvgItem {
        anchors.centerIn: parent
        width: arrow.width
        height: arrow.height

        rotation: systemTrayState.expanded ? 0 : -180
        Behavior on rotation {
            RotationAnimation {
                duration: tooltip.arrowAnimationDuration
            }
        }
        opacity: systemTrayState.expanded ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: tooltip.arrowAnimationDuration
            }
        }

        svg: arrowSvg
        elementId: {
            if (Plasmoid.location === PlasmaCore.Types.TopEdge) {
                return "up-arrow";
            } else if (Plasmoid.location === PlasmaCore.Types.LeftEdge) {
                return "left-arrow";
            } else if (Plasmoid.location === PlasmaCore.Types.RightEdge) {
                return "right-arrow";
            } else {
                return "down-arrow";
            }
        }
    }
}
