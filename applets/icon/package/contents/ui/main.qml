/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

import org.kde.draganddrop 2.0 as DragDrop
import org.kde.kquickcontrolsaddons 2.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.plasmoid 2.0

MouseArea {
    id: root

    readonly property bool inPanel: [PlasmaCore.Types.TopEdge, PlasmaCore.Types.RightEdge, PlasmaCore.Types.BottomEdge, PlasmaCore.Types.LeftEdge]
        .includes(Plasmoid.location)
    readonly property bool constrained: [PlasmaCore.Types.Vertical, PlasmaCore.Types.Horizontal]
        .includes(Plasmoid.formFactor)
    property bool containsAcceptableDrag: false

    height: Math.round(PlasmaCore.Units.iconSizes.desktop + 2 * PlasmaCore.Theme.mSize(PlasmaCore.Theme.defaultFont).height)
    width: Math.round(PlasmaCore.Units.iconSizes.desktop * 1.5)

    activeFocusOnTab: true
    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_Space:
        case Qt.Key_Enter:
        case Qt.Key_Return:
        case Qt.Key_Select:
            Plasmoid.nativeInterface.run()
            break;
        }
    }
    Accessible.name: Plasmoid.title
    Accessible.description: toolTip.subText
    Accessible.role: Accessible.Button

    Layout.minimumWidth: Plasmoid.formFactor === PlasmaCore.Types.Horizontal ? height : PlasmaCore.Units.iconSizes.small
    Layout.minimumHeight: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? width : (PlasmaCore.Units.iconSizes.small + 2 * PlasmaCore.Theme.mSize(PlasmaCore.Theme.defaultFont).height)

    Layout.maximumWidth: inPanel ? PlasmaCore.Units.iconSizeHints.panel : -1
    Layout.maximumHeight: inPanel ? PlasmaCore.Units.iconSizeHints.panel : -1

    hoverEnabled: true
    enabled: Plasmoid.nativeInterface.valid

    onClicked: Plasmoid.nativeInterface.run()

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation
    Plasmoid.icon: Plasmoid.nativeInterface.iconName
    Plasmoid.title: Plasmoid.nativeInterface.name
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    Plasmoid.onActivated: Plasmoid.nativeInterface.run()

    Plasmoid.onContextualActionsAboutToShow: updateActions()

    Component.onCompleted: updateActions()

    function updateActions() {
        Plasmoid.clearActions();
        Plasmoid.removeAction("configure");

        if (Plasmoid.nativeInterface.valid && Plasmoid.immutability !== PlasmaCore.Types.SystemImmutable) {
            Plasmoid.setAction("configure", i18n("Properties"), "document-properties");
        }
    }

    function action_configure() {
        Plasmoid.nativeInterface.configure();
    }

    Connections {
        target: Plasmoid.self
        function onExternalData(mimetype, data) {
            Plasmoid.nativeInterface.url = data;
        }
    }

    Connections {
        target: Plasmoid.nativeInterface
        function onValidChanged() {
            updateActions();
        }
    }

    DragDrop.DropArea {
        id: dropArea
        anchors.fill: parent
        preventStealing: true
        onDragEnter: {
            const acceptable = Plasmoid.nativeInterface.isAcceptableDrag(event);
            root.containsAcceptableDrag = acceptable;

            if (!acceptable) {
                event.ignore();
            }
        }
        onDragLeave: root.containsAcceptableDrag = false
        onDrop: {
            if (root.containsAcceptableDrag) {
                Plasmoid.nativeInterface.processDrop(event);
            } else {
                event.ignore();
            }

            root.containsAcceptableDrag = false;
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
        enabled: root.enabled
        active: root.containsMouse || root.containsAcceptableDrag
        usesPlasmaTheme: false
        opacity: Plasmoid.busy ? 0.6 : 1
        Behavior on opacity {
            OpacityAnimator {
                duration: PlasmaCore.Units.shortDuration
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
        subText: Plasmoid.nativeInterface.genericName !== mainText ? Plasmoid.nativeInterface.genericName : ""
        textFormat: Text.PlainText
    }
}
