/*
    SPDX-FileCopyrightText: 2020 Mikhail Zolotukhin <zomial@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.0 as QtDialogs
import QtQuick.Controls 2.10 as QtControls
import org.kde.kirigami 2.10 as Kirigami
import org.kde.private.kcms.style 1.0 as Private
import org.kde.newstuff 1.62 as NewStuff
import org.kde.kcm 1.2 as KCM

Kirigami.Page {
    id: gtkStylePage
    title: i18n("GNOME/GTK Application Style")

    ColumnLayout {
        anchors.fill: parent

        Kirigami.InlineMessage {
            id: infoLabel
            Layout.fillWidth: true

            showCloseButton: true
            visible: false

            Connections {
                target: kcm.gtkPage
                function onShowErrorMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Error;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                }
            }
        }

        Kirigami.FormLayout {
            wideMode: true

            Row {
                Kirigami.FormData.label: i18n("GTK theme:")

                Flow {
                    spacing: Kirigami.Units.smallSpacing

                    QtControls.ComboBox {
                        id: gtkThemeCombo
                        model: kcm.gtkPage.gtkThemesModel
                        currentIndex: model.findThemeIndex(kcm.gtkPage.gtkThemeFromConfig())
                        onCurrentTextChanged: function() {
                            model.selectedTheme = currentText
                            gtkRemoveButton.enabled = model.selectedThemeRemovable()
                        }
                        onActivated: model.setSelectedThemeDirty()
                        textRole: "theme-name"

                        Connections {
                            target: kcm.gtkPage
                            function onSelectGtkThemeInCombobox(themeName) {
                                gtkThemeCombo.currentIndex = gtkThemeCombo.model.findThemeIndex(themeName)
                            }
                        }
                    }

                    QtControls.Button {
                        id: gtkRemoveButton
                        icon.name: "edit-delete"
                        onClicked: gtkThemeCombo.model.removeSelectedTheme()
                    }

                    QtControls.Button {
                        icon.name: "preview"
                        text: i18n("Preview…")
                        onClicked: kcm.gtkPage.showGtkPreview()
                        visible: kcm.gtkPage.gtkPreviewAvailable()
                    }

                }
            }

        }

        Item {
            Layout.fillHeight: true
        }

        Kirigami.ActionToolBar {
            flat: false
            alignment: Qt.AlignRight
            actions: [
                Kirigami.Action {
                    text: i18n("Install from File…")
                    icon.name: "document-import"
                    onTriggered: fileDialogLoader.active = true
                },
                Kirigami.Action {
                    text: i18n("Get New GNOME/GTK Application Styles…")
                    icon.name: "get-hot-new-stuff"
                    onTriggered: { newStuffPage.open(); }
                }
            ]
        }
    }

    Loader {
        id: newStuffPage

        // Use this function to open the dialog. It seems roundabout, but this ensures
        // that the dialog is not constructed until we want it to be shown the first time,
        // since it will initialise itself on the first load (which causes it to phone
        // home) and we don't want that until the user explicitly asks for it.
        function open() {
            if (item) {
                item.open();
            } else {
                active = true;
            }
        }
        onLoaded: {
            item.open();
        }

        active: false
        asynchronous: true

        sourceComponent: NewStuff.Dialog {
            id: newStuffDialog
            configFile: "gtk_themes.knsrc"
            viewMode: NewStuff.Page.ViewMode.Preview
            Connections {
                target: newStuffDialog.engine
                function onEntryEvent(entry, event) {
                    if (event == 1) { // StatusChangedEvent
                        kcm.load();
                    }
                }

            }
        }
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: QtDialogs.FileDialog {
            title: i18n("Select GTK Theme Archive")
            folder: shortcuts.home
            nameFilters: [ i18n("GTK Theme Archive (*.tar.xz *.tar.gz *.tar.bz2)") ]
            Component.onCompleted: open()
            onAccepted: {
                kcm.gtkPage.installGtkThemeFromFile(fileUrls[0])
                fileDialogLoader.active = false
            }
            onRejected: {
                fileDialogLoader.active = false
            }
        }
    }
}
