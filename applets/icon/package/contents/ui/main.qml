/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

import org.kde.draganddrop 2.0 as DragDrop
import org.kde.kquickcontrolsaddons 2.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.20 as Kirigami

PlasmoidItem {
    id: root

    readonly property bool inPanel: [PlasmaCore.Types.TopEdge, PlasmaCore.Types.RightEdge, PlasmaCore.Types.BottomEdge, PlasmaCore.Types.LeftEdge]
        .includes(Plasmoid.location)
    readonly property bool constrained: [PlasmaCore.Types.Vertical, PlasmaCore.Types.Horizontal]
        .includes(Plasmoid.formFactor)
    property bool containsAcceptableDrag: false

    preferredRepresentation: fullRepresentation
    height: Math.round(Kirigami.Units.iconSizes.desktop + 2 * Kirigami.Units.gridUnit)
    width: Math.round(Kirigami.Units.iconSizes.desktop * 1.5)

    Layout.minimumWidth: Plasmoid.formFactor === PlasmaCore.Types.Horizontal ? height : Kirigami.Units.iconSizes.small
    Layout.minimumHeight: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? width : (Kirigami.Units.iconSizes.small + 2 * Kirigami.Units.iconSizes.desktop + 2 * Kirigami.Units.gridUnit)

    enabled: Plasmoid.valid

    Plasmoid.icon: Plasmoid.iconName
    Plasmoid.title: Plasmoid.name

    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    Plasmoid.onActivated: Plasmoid.run()

    Plasmoid.onContextualActionsAboutToShow: updateActions()
    Plasmoid.contextualActions: Plasmoid.extraActions

    PlasmaCore.Action {
        id: configureAction
        text: i18n("Properties")
        icon.name: "document-properties"
        visible: Plasmoid.valid && Plasmoid.immutability !== PlasmaCore.Types.SystemImmutable
        onTriggered: Plasmoid.configure()
    }
    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction);
    }

    onExternalData: (mimetype, data) => {
        root.Plasmoid.url = data;
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        activeFocusOnTab: true

        Keys.onPressed: {
            switch (event.key) {
            case Qt.Key_Space:
            case Qt.Key_Enter:
            case Qt.Key_Return:
            case Qt.Key_Select:
                Plasmoid.run()
                break;
            }
        }
        Accessible.name: Plasmoid.title
        Accessible.description: toolTip.subText
        Accessible.role: Accessible.Button

        Layout.minimumWidth: Plasmoid.formFactor === PlasmaCore.Types.Horizontal ? height : Kirigami.Units.iconSizes.small
        Layout.minimumHeight: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? width : (Kirigami.Units.iconSizes.small + 2 * Kirigami.Theme.gridUnit

        hoverEnabled: true
        enabled: Plasmoid.valid

        onClicked: Plasmoid.run()

        Component.onCompleted: updateActions()

        Connections {
            target: Plasmoid
            function onValidChanged() {
                updateActions();
            }
        }

        DragDrop.DropArea {
            id: dropArea
            anchors.fill: parent
            preventStealing: true
            onDragEnter: {
                const acceptable = Plasmoid.isAcceptableDrag(event);
                containsAcceptableDrag = acceptable;

                if (!acceptable) {
                    event.ignore();
                }
            }
            onDragLeave: containsAcceptableDrag = false
            onDrop: {
                if (containsAcceptableDrag) {
                    Plasmoid.processDrop(event);
                } else {
                    event.ignore();
                }

                containsAcceptableDrag = false;
            }
        }

        PlasmaCore.IconItem {
            id: icon
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: constrained ? parent.bottom : text.top
            }
            source: Plasmoid.icon
            enabled: mouseArea.enabled
            active: mouseArea.containsMouse || containsAcceptableDrag
            usesPlasmaTheme: false
            opacity: Plasmoid.busy ? 0.6 : 1
            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.OutCubic
                }
            }
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
            id: text
            text: Plasmoid.title
            anchors {
                left: parent.left
                bottom: parent.bottom
                right: parent.right
            }
            horizontalAlignment: Text.AlignHCenter
            visible: false // rendered by DropShadow
            maximumLineCount: 2
            color: "white"
            elide: Text.ElideRight
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            textFormat: Text.PlainText
        }

        PlasmaCore.ToolTipArea {
            id: toolTip
            anchors.fill: parent
            mainText: Plasmoid.title
            subText: Plasmoid.genericName !== mainText ? Plasmoid.genericName : ""
            textFormat: Text.PlainText
        }
    }
}
