/*
    SPDX-FileCopyrightText: 2020 Mikhail Zolotukhin <zomial@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs as QtDialogs
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.newstuff as NewStuff

Kirigami.Page {
    id: gtkStylePage
    title: i18n("GNOME/GTK Application Style")

    header: Kirigami.InlineMessage {
        id: infoLabel

        position: Kirigami.InlineMessage.Position.Header
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

    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Install from File…")
            icon.name: "document-import"
            onTriggered: fileDialogLoader.active = true
        },
        NewStuff.Action {
            text: i18nc("@action:button", "Get New…")
            configFile: "gtk_themes.knsrc"
            onEntryEvent: function (entry, event) {
                if (event == NewStuff.Entry.StatusChangedEvent) {
                    kcm.load();
                }
            }
        }
    ]

    ColumnLayout {
        anchors.fill: parent

        Kirigami.FormLayout {
            wideMode: true

            Row {
                Kirigami.FormData.label: i18n("GTK theme:")

                Flow {
                    spacing: Kirigami.Units.smallSpacing

                    QtControls.ComboBox {
                        id: gtkThemeCombo
                        model: kcm.gtkPage.gtkThemesModel
                        currentIndex: model.findThemeIndex(kcm.gtkPage.gtkThemesModel.selectedTheme)
                        onCurrentTextChanged: function() {
                            model.selectedTheme = currentText
                            gtkRemoveButton.enabled = model.selectedThemeRemovable()
                        }
                        onActivated: model.setSelectedThemeDirty()
                        textRole: "theme-name"
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

        QtControls.CheckBox {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Colorize Adwaita apps")
            checked: kcm.styleSettings.colorizeAdwaita
            onToggled: kcm.styleSettings.colorizeAdwaita = checked

            QtControls.ToolTip.visible: hovered
            QtControls.ToolTip.delay: Kirigami.Units.toolTipDelay
            QtControls.ToolTip.text: i18n("Apply the current Plasma color scheme and accent color to GTK 4 and libadwaita apps that use the default Adwaita style.")

            KCM.SettingStateBinding {
                configObject: kcm.styleSettings
                settingName: "colorizeAdwaita"
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: QtDialogs.FileDialog {
            title: i18n("Select GTK Theme Archive")
            currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
            nameFilters: [ i18n("GTK Theme Archive (*.tar.xz *.tar.gz *.tar.bz2)") ]
            Component.onCompleted: open()
            onAccepted: {
                kcm.gtkPage.installGtkThemeFromFile(selectedFile)
                fileDialogLoader.active = false
            }
            onRejected: {
                fileDialogLoader.active = false
            }
        }
    }
}
