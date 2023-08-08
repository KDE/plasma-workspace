/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
    SPDX-FileCopyrightText: 2023 Mike Noe <noeerover@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import Qt5Compat.GraphicalEffects

import org.kde.draganddrop 2.0 as DragDrop
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.20 as Kirigami

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
                containsAcceptableDrag = acceptable;

                if (!acceptable) {
                    event.ignore();
                }
            }
            
            onDragLeave: containsAcceptableDrag = false
            
            onDrop: event => {
                if (containsAcceptableDrag) {
                    Plasmoid.processDrop(event);
                } else {
                    event.ignore();
                }

                containsAcceptableDrag = false;
            }
        }

        Kirigami.Icon {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: constrained ? parent.bottom : iconLabel.top
            }
            source: Plasmoid.icon
            enabled: mouseArea.enabled
            active: mouseArea.containsMouse || containsAcceptableDrag
            opacity: Plasmoid.busy ? 0.6 : 1
            
            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.OutCubic
                }
            }
        }

        Rectangle {
            id: fallbackRectangleBackground
            visible: GraphicsInfo.api === GraphicsInfo.Software && !constrained
            anchors {
                fill: iconLabel
                margins: -Kirigami.Units.smallSpacing
            }

            color: "black"
            radius: Kirigami.Units.smallSpacing
            opacity: 0.45
        }

        PlasmaComponents3.Label {
            id: iconLabel
            text: Plasmoid.title
            
            anchors {
                left: parent.left
                bottom: parent.bottom
                right: parent.right
            }
            
            horizontalAlignment: Text.AlignHCenter
            visible: GraphicsInfo.api === GraphicsInfo.Software && !constrained
            maximumLineCount: 2
            elide: Text.ElideRight
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            textFormat: Text.PlainText
            
            color: "white"
            
            layer.enabled: GraphicsInfo.api === GraphicsInfo.OpenGL && !constrained
            layer.effect: DropShadow {
                horizontalOffset: 1
                verticalOffset: 1

                radius: 4.0
                samples: radius * 2 + 1
                spread: 0.35
                color: "black"

                visible: GraphicsInfo.api === GraphicsInfo.OpenGL && !constrained
            }
            
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
