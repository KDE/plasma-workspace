/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.8

import org.kde.plasma.plasmoid 2.0
import org.kde.kquickcontrolsaddons 2.0 // For KCMShell
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.private.appmenu 1.0 as AppMenuPrivate
import org.kde.kirigami 2.5 as Kirigami

Item {
    id: root

    readonly property bool vertical: plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool view: plasmoid.configuration.compactView
    readonly property bool menuAvailable: appMenuModel.menuAvailable

    readonly property bool kcmAuthorized: KCMShell.authorize(["style.desktop"]).length > 0

    onViewChanged: {
        plasmoid.nativeInterface.view = view
    }

    Plasmoid.constraintHints: PlasmaCore.Types.CanFillArea
    Plasmoid.preferredRepresentation: (plasmoid.configuration.compactView) ? Plasmoid.compactRepresentation : Plasmoid.fullRepresentation

    Plasmoid.compactRepresentation: PlasmaComponents3.ToolButton {
        readonly property int fakeIndex: 0
        Layout.fillWidth: false
        Layout.fillHeight: false
        Layout.minimumWidth: implicitWidth
        Layout.maximumWidth: implicitWidth
        enabled:  menuAvailable
        checkable: menuAvailable && plasmoid.nativeInterface.currentIndex === fakeIndex
        checked: checkable
        icon.name: "application-menu"
        onClicked: plasmoid.nativeInterface.trigger(this, 0);
    }

    Plasmoid.fullRepresentation: GridLayout {
        id: buttonGrid

        Plasmoid.status: {
            if (menuAvailable && plasmoid.nativeInterface.currentIndex > -1 && buttonRepeater.count > 0) {
                return PlasmaCore.Types.NeedsAttentionStatus;
            } else {
                //when we're not enabled set to active to show the configure button
                return buttonRepeater.count > 0 ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.HiddenStatus;
            }
        }

        Layout.minimumWidth: implicitWidth
        Layout.minimumHeight: implicitHeight

        flow: root.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        rowSpacing: 0
        columnSpacing: 0

        Component.onCompleted: {
            plasmoid.nativeInterface.buttonGrid = buttonGrid

            // using a Connections {} doesn't work for some reason in Qt >= 5.8
            plasmoid.nativeInterface.requestActivateIndex.connect(function (index) {
                var idx = Math.max(0, Math.min(buttonRepeater.count - 1, index))
                var button = buttonRepeater.itemAt(index)
                if (button) {
                    button.clicked()
                }
            });

            plasmoid.activated.connect(function () {
                var button = buttonRepeater.itemAt(0);
                if (button) {
                    button.clicked();
                }
            });
        }

        // So we can show mnemonic underlines only while Alt is pressed
        PlasmaCore.DataSource {
            id: keystateSource
            engine: "keystate"
            connectedSources: ["Alt"]
        }

        PlasmaComponents3.ToolButton {
            id: noMenuPlaceholder
            visible: buttonRepeater.count == 0
            text: plasmoid.title
            Layout.fillWidth: root.vertical
            Layout.fillHeight: !root.vertical
        }

        Repeater {
            id: buttonRepeater
            model: appMenuModel.visible ? appMenuModel : null

            MenuDelegate {
                readonly property int buttonIndex: index

                Layout.fillWidth: root.vertical
                Layout.fillHeight: !root.vertical
                text: activeMenu
                // TODO: Alt and other modifiers might be unavailable on Wayland
                Kirigami.MnemonicData.active: keystateSource.data.Alt !== undefined && keystateSource.data.Alt.Pressed

                down: pressed || plasmoid.nativeInterface.currentIndex === index

                visible: text !== ""
                onClicked: {
                    plasmoid.nativeInterface.trigger(this, index)

                    checked = Qt.binding(function() {
                        return plasmoid.nativeInterface.currentIndex === index;
                    });
                }

                // QMenu opens on press, so we'll replicate that here
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: plasmoid.nativeInterface.currentIndex !== -1
                    onPressed: parent.clicked()
                    onEntered: parent.clicked()
                }
            }
        }
    }

    AppMenuPrivate.AppMenuModel {
        id: appMenuModel
        screenGeometry: plasmoid.screenGeometry
        onRequestActivateIndex: plasmoid.nativeInterface.requestActivateIndex(index)
        Component.onCompleted: {
            plasmoid.nativeInterface.model = appMenuModel
        }
    }
}
