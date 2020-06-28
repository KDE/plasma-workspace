/*
 * Copyright 2013  Bhushan Shah <bhush94@gmail.com>
 * Copyright 2016  Kai Uwe Broulik <kde@privat.broulik.de>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kquickcontrolsaddons 2.0
import org.kde.draganddrop 2.0 as DragDrop

MouseArea {
    id: root

    readonly property bool inPanel: (plasmoid.location === PlasmaCore.Types.TopEdge
        || plasmoid.location === PlasmaCore.Types.RightEdge
        || plasmoid.location === PlasmaCore.Types.BottomEdge
        || plasmoid.location === PlasmaCore.Types.LeftEdge)
    readonly property bool constrained: plasmoid.formFactor === PlasmaCore.Types.Vertical || plasmoid.formFactor === PlasmaCore.Types.Horizontal
    property bool containsAcceptableDrag: false

    height: Math.round(units.iconSizes.desktop + 2 * theme.mSize(theme.defaultFont).height)
    width: Math.round(units.iconSizes.desktop * 1.5)

    Layout.minimumWidth: plasmoid.formFactor === PlasmaCore.Types.Horizontal ? height : units.iconSizes.small
    Layout.minimumHeight: plasmoid.formFactor === PlasmaCore.Types.Vertical ? width : (units.iconSizes.small + 2 * theme.mSize(theme.defaultFont).height)

    Layout.maximumWidth: inPanel ? units.iconSizeHints.panel : -1
    Layout.maximumHeight: inPanel ? units.iconSizeHints.panel : -1

    hoverEnabled: true

    onClicked: plasmoid.nativeInterface.run()

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation
    Plasmoid.icon: plasmoid.nativeInterface.iconName
    Plasmoid.title: plasmoid.nativeInterface.name
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    Plasmoid.onActivated: plasmoid.nativeInterface.run()

    Plasmoid.onContextualActionsAboutToShow: updateActions()

    Component.onCompleted: updateActions()

    function updateActions() {
        plasmoid.clearActions()

        plasmoid.removeAction("configure");

        if (plasmoid.immutability !== PlasmaCore.Types.SystemImmutable) {
            plasmoid.setAction("configure", i18n("Properties"), "document-properties");
        }
    }

    function action_configure() {
        plasmoid.nativeInterface.configure()
    }

    Connections {
        target: plasmoid
        onExternalData: plasmoid.nativeInterface.url = data
    }

    DragDrop.DropArea {
        id: dropArea
        anchors.fill: parent
        preventStealing: true
        onDragEnter: {
            var acceptable = plasmoid.nativeInterface.isAcceptableDrag(event);
            root.containsAcceptableDrag = acceptable;

            if (!acceptable) {
                event.ignore();
            }
        }
        onDragLeave: root.containsAcceptableDrag = false
        onDrop: {
            if (root.containsAcceptableDrag) {
                plasmoid.nativeInterface.processDrop(event)
            } else {
                event.ignore();
            }

            root.containsAcceptableDrag = false
        }
    }

    PlasmaCore.IconItem {
        id: icon
        anchors{
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: constrained ? parent.bottom : text.top
        }
        source: plasmoid.icon
        enabled: root.enabled
        active: root.containsMouse || root.containsAcceptableDrag
        usesPlasmaTheme: false
        opacity: plasmoid.busy ? 0.6 : 1
    }

    DropShadow {
        id: textShadow

        anchors.fill: text

        visible: !constrained

        horizontalOffset: 1
        verticalOffset: 1

        radius: 4
        samples: 9
        spread: 0.35

        color: "black"

        source: constrained ? null : text
    }

    PlasmaComponents3.Label {
        id : text
        text : plasmoid.title
        anchors {
            left : parent.left
            bottom : parent.bottom
            right : parent.right
        }
        horizontalAlignment : Text.AlignHCenter
        visible: false // rendered by DropShadow
        maximumLineCount: 2
        color: "white"
        elide: Text.ElideRight
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        textFormat: Text.PlainText
    }

    PlasmaCore.ToolTipArea {
        anchors.fill: parent
        mainText: plasmoid.title
        subText: plasmoid.nativeInterface.genericName !== mainText ? plasmoid.nativeInterface.genericName :""
        textFormat: Text.PlainText
    }
}
