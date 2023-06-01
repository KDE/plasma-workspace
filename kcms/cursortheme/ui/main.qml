/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtCore
import QtQuick 2.7
import QtQuick.Window 2.2 // for Screen
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2 as QtControls
import QtQuick.Dialogs 6.3 as QtDialogs
import org.kde.kirigami 2.5 as Kirigami
import org.kde.newstuff 1.91 as NewStuff
import org.kde.kcmutils as KCM

import org.kde.private.kcm_cursortheme 1.0

KCM.GridViewKCM {
    id: root

    view.model: kcm.cursorsModel
    view.delegate: Delegate {}
    view.currentIndex: kcm.cursorThemeIndex(kcm.cursorThemeSettings.cursorTheme);

    view.onCurrentIndexChanged: {
        kcm.cursorThemeSettings.cursorTheme = kcm.cursorThemeFromIndex(view.currentIndex)
        view.positionViewAtIndex(view.currentIndex, view.GridView.Beginning);
    }

    Component.onCompleted: {
        view.positionViewAtIndex(view.currentIndex, GridView.Beginning);
    }

    KCM.SettingStateBinding {
        configObject: kcm.cursorThemeSettings
        settingName: "cursorTheme"
        extraEnabledConditions: !kcm.downloadingFile
    }

    DropArea {
        anchors.fill: parent
        onEntered: {
            if (!drag.hasUrls) {
                drag.accepted = false;
            }
        }
        onDropped: kcm.installThemeFromFile(drop.urls[0])
    }

    actions: [
        Kirigami.Action {
            displayComponent: RowLayout {
                QtControls.Label {
                    text: i18nc("@label Size of the cursor", "Size:")
                }
                QtControls.ComboBox {
                    id: sizeCombo

                    model: kcm.sizesModel
                    textRole: "display"
                    currentIndex: kcm.cursorSizeIndex(kcm.cursorThemeSettings.cursorSize);
                    onActivated: {
                        kcm.cursorThemeSettings.cursorSize = kcm.cursorSizeFromIndex(sizeCombo.currentIndex);
                        kcm.preferredSize = kcm.cursorSizeFromIndex(sizeCombo.currentIndex);
                    }

                    KCM.SettingStateBinding {
                    configObject: kcm.cursorThemeSettings
                        settingName: "cursorSize"
                        extraEnabledConditions: kcm.canResize
                    }

                    delegate: QtControls.ItemDelegate {
                        id: sizeComboDelegate

                        readonly property int size: parseInt(model.display)

                        width: parent.width
                        highlighted: ListView.isCurrentItem
                        text: model.display

                        contentItem: RowLayout {
                            Kirigami.Icon {
                                source: model.decoration
                                smooth: true
                                Layout.preferredWidth: sizeComboDelegate.size / Screen.devicePixelRatio
                                Layout.preferredHeight: sizeComboDelegate.size / Screen.devicePixelRatio
                                visible: valid && sizeComboDelegate.size > 0
                            }

                            QtControls.Label {
                                Layout.fillWidth: true
                                color: sizeComboDelegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                                text: model[sizeCombo.textRole]
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        },
        Kirigami.Action {
            text: i18nc("@action:button", "&Configure Launch Feedback…")
            icon.name: "preferences-desktop-launch-feedback"
            onTriggered: {
                const component = Qt.createComponent("LaunchFeedbackDialog.qml");
                component.incubateObject(root, {
                    "parent": root,
                });
                component.destroy();
            }
        },
        Kirigami.Action {
            text: i18n("&Install from File…")
            icon.name: "document-import"
            onTriggered: fileDialogLoader.active = true
            enabled: kcm.canInstall
        },
        NewStuff.Action {
            text: i18n("&Get New…")
            configFile: "xcursor.knsrc"
            onEntryEvent: function (entry, event) {
                if (event == NewStuff.Entry.StatusChangedEvent) {
                    kcm.ghnsEntryChanged(entry);
                }
            }
        }
    ]

    footer: ColumnLayout {
        id: footerLayout

        Kirigami.InlineMessage {
            id: infoLabel
            Layout.fillWidth: true

            showCloseButton: true

            Connections {
                target: kcm
                function onShowSuccessMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Positive;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                }
                function onShowInfoMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Information;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                }
                function onShowErrorMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Error;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                }
            }
        }
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: QtDialogs.FileDialog {
            title: i18n("Open Theme")
            currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
            nameFilters: [ i18n("Cursor Theme Files (*.tar.gz *.tar.bz2)") ]
            Component.onCompleted: open()
            onAccepted: {
                kcm.installThemeFromFile(selectedFile)
                fileDialogLoader.active = false
            }
            onRejected: {
                fileDialogLoader.active = false
            }
        }
    }
}

