/*
    SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtCore
import QtQuick 2.10
import QtQuick.Controls 2.10
import QtQuick.Dialogs 6.3
import QtQuick.Layouts 1.11
import org.kde.kcmutils as KCM
import org.kde.kirigami 2 as Kirigami
import org.kde.plasma.kcm.autostart 1.0

KCM.ScrollViewKCM {
    id: root

    implicitHeight: Kirigami.Units.gridUnit * 28
    implicitWidth: Kirigami.Units.gridUnit * 28

    header: Kirigami.InlineMessage {
        id: errorMessage

        showCloseButton: true

        Connections {
            target: kcm.model
            function onError(message) {
                errorMessage.type = Kirigami.MessageType.Error;
                errorMessage.visible = true;
                errorMessage.text = message;
            }

        }

        Connections {
            target: kcm.model
            property var fixItAction: Kirigami.Action {
                property string fileName
                text: i18n("Make Executable")
                icon.name: "dialog-ok"
                onTriggered: {
                    kcm.model.makeFileExecutable(fileName);
                    errorMessage.visible = false;
                }
            }

            function onNonExecutableScript(fileName, kind) {
                fixItAction.fileName = fileName;
                errorMessage.type = Kirigami.MessageType.Warning;
                errorMessage.visible = true;
                errorMessage.actions = [fixItAction];
                if (kind === AutostartModel.PlasmaShutdown) {
                    errorMessage.text = i18nd("kcm_autostart", "The file '%1' must be executable to run at logout.", fileName);
                } else {
                    errorMessage.text = i18nd("kcm_autostart", "The file '%1' must be executable to run at login.", fileName);
                }
            }

        }

    }

    actions: [
        Kirigami.Action {
            icon.name: "list-add"
            text: i18nc("@action:button", "Add…")

            checkable: true
            checked: menu.opened
            onTriggered: if (menu.opened) {
                menu.close();
            } else {
                menu.open();
            }

            Kirigami.Action {
                text: i18n("Add Application…")
                icon.name: "list-add"
                onTriggered: kcm.model.showApplicationDialog(root)
            }
            Kirigami.Action {
                text: i18n("Add Login Script…")
                icon.name: "list-add"
                onTriggered: loginFileDialogLoader.active = true
            }
            Kirigami.Action {
                text: i18n("Add Logout Script…")
                icon.name: "list-add"
                onTriggered: logoutFileDialogLoader.active = true
            }
        }
    ]

    view: ListView {
        id: listView

        clip: true
        model: kcm.model

        delegate: Kirigami.SwipeListItem {
            id: baseListItem

            property Unit unit: model.systemdUnit
            property bool noUnit: (unit && !kcm.model.usingSystemdBoot) || (model.source === AutostartModel.PlasmaShutdown || model.source === AutostartModel.PlasmaEnvScripts)
            background: Rectangle {
                color: (!noUnit && baseListItem.hovered) ? Kirigami.Theme.highlightColor : Kirigami.Theme.backgroundColor
                opacity: (!baseListItem.down && baseListItem.hovered) ? 0.3 : 1
            }
            activeTextColor: noUnit ? Kirigami.Theme.textColor : Kirigami.Theme.highlightedTextColor
            width: listView.width
            // content item includes its own padding
            padding: 0

            onClicked: {
                if (noUnit) {
                    return;
                }

                if (!model.systemdUnit.invalid) {
                    kcm.pop();
                    kcm.push("entry.qml", {
                        "unit": unit
                    });
                } else {
                    errorMessage.type = Kirigami.MessageType.Warning;
                    errorMessage.visible = true;
                    errorMessage.text = i18nd("kcm_autostart", "%1 has not been autostarted yet. Details will be available after the system is restarted.", model.name);
                }
            }


            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.BasicListItem {
                    width: listView.width - baseListItem.overlayWidth
                    icon.name: model.iconName
                    // prevent icon flickering now that we've disabled background color changes
                    Binding on iconSelected {
                        when: noUnit
                        value: false
                    }

                    reserveSpaceForSubtitle: true
                    background: null
                    activeTextColor: noUnit ? Kirigami.Theme.textColor : Kirigami.Theme.highlightedTextColor
                    separatorVisible: false
                    label: model.name
                    hoverEnabled: false
                    subtitle: model.source === AutostartModel.PlasmaShutdown || model.source === AutostartModel.XdgScripts ? model.targetFileDirPath : ""
                    onClicked: {
                        baseListItem.clicked();
                        baseListItem.down = undefined;
                    }
                    // Hack to pass the click event to SwipeListItem
                    onPressedChanged: {
                        if (pressed) {
                            baseListItem.down = true;
                        } else {
                            baseListItem.down = undefined;
                        }
                    }
                }

                Label {
                    text: {
                        if (noUnit || !model.systemdUnit) {
                            return "";
                        } else if (model.systemdUnit.invalid) {
                            return i18nc("@label Entry hasn't been autostarted because system hasn't been restarted", "Not autostarted yet");
                        } else {
                            return model.systemdUnit.activeState;
                        }
                    }
                    color: model.systemdUnit.activeState === i18nc("@label Entry has failed (exited with an error)", "Failed") ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
                }

            }

            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "See properties")
                    icon.name: "document-properties"
                    onTriggered: kcm.model.editApplication(model.index, root)
                    visible: model.source === AutostartModel.XdgAutoStart || model.source === AutostartModel.XdgScripts
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Remove entry")
                    icon.name: "edit-delete-remove"
                    onTriggered: kcm.model.removeEntry(model.index)
                }
            ]
        }
        section.property: "source"
        section.delegate: Kirigami.ListSectionHeader {
            text: {
                if (section == AutostartModel.XdgAutoStart){
                    return i18n("Applications");
                }
                if (section == AutostartModel.XdgScripts){
                    return i18n("Login Scripts");
                }
                if (section == AutostartModel.PlasmaEnvScripts){
                    return i18n("Pre-startup Scripts");
                }
                if (section == AutostartModel.PlasmaShutdown){
                    return i18n("Logout Scripts");
                }
            }
        }
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: parent.count === 0
            text: i18n("No user-specified autostart items")
            explanation: xi18nc("@info", "Click the <interface>Add…</interface> button below to add some")
        }
    }

    footer: Row {
        spacing: Kirigami.Units.largeSpacing

        Loader {
            id: loginFileDialogLoader

            active: false

            sourceComponent: FileDialog {
                id: loginFileDialog

                title: i18n("Choose Login Script")
                currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
                onAccepted: {
                    kcm.model.addScript(loginFileDialog.selectedFile, AutostartModel.XdgScripts);
                    loginFileDialogLoader.active = false;
                }
                onRejected: loginFileDialogLoader.active = false
                Component.onCompleted: open()
            }

        }

        Loader {
            id: logoutFileDialogLoader

            active: false

            sourceComponent: FileDialog {
                id: logoutFileDialog

                title: i18n("Choose Logout Script")
                currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
                onAccepted: {
                    kcm.model.addScript(logoutFileDialog.selectedFile, AutostartModel.PlasmaShutdown);
                    logoutFileDialogLoader.active = false;
                }
                onRejected: logoutFileDialogLoader.active = false
                Component.onCompleted: open()
            }
        }
    }
}
