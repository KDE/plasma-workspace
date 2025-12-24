/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
    SPDX-FileCopyrightText: 2023 Mike Noe <noeerover@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick

import org.kde.draganddrop as DragDrop
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    readonly property bool constrained: [PlasmaCore.Types.Vertical, PlasmaCore.Types.Horizontal]
        .includes(Plasmoid.formFactor)
    property bool containsAcceptableDrag: false

    preferredRepresentation: fullRepresentation

    enabled: Plasmoid.valid

    Plasmoid.icon: Plasmoid.iconName
    Plasmoid.title: Plasmoid.name

    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    Plasmoid.onActivated: Plasmoid.run()

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
        
        Keys.onPressed: event => {
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

        hoverEnabled: true
        enabled: Plasmoid.valid

        onClicked: Plasmoid.run()

        DragDrop.DropArea {
            anchors.fill: parent
            preventStealing: true
            
            onDragEnter: event => {
                const acceptable = Plasmoid.isAcceptableDrag(event);
                root.containsAcceptableDrag = acceptable;

                if (!acceptable) {
                    event.ignore();
                }
            }
            
            onDragLeave: root.containsAcceptableDrag = false
            
            onDrop: event => {
                if (root.containsAcceptableDrag) {
                    Plasmoid.processDrop(event);
                } else {
                    event.ignore();
                }

                root.containsAcceptableDrag = false;
            }
        }

        Kirigami.Icon {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: root.constrained ? parent.bottom : iconLabel.top
            }
            source: Plasmoid.icon
            enabled: mouseArea.enabled
            active: mouseArea.containsMouse || root.containsAcceptableDrag
            opacity: Plasmoid.busy ? 0.6 : 1
            
            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.OutCubic
                }
            }
        }

        PlasmaExtras.ShadowedLabel {
            id: iconLabel

            anchors {
                left: parent.left
                bottom: parent.bottom
                right: parent.right
            }

            text: Plasmoid.title
            horizontalAlignment: Text.AlignHCenter
            maximumLineCount: 2

            visible: !root.constrained
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
