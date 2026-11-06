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
                    id: gtkThemeControls

                    // Styles size text and icon-only controls differently
                    readonly property real controlHeight: Math.max(gtkThemeCombo.implicitHeight, gtkRemoveButton.implicitHeight, gtkPreviewButton.implicitHeight)

                    spacing: Kirigami.Units.smallSpacing

                    QtControls.ComboBox {
                        id: gtkThemeCombo
                        height: gtkThemeControls.controlHeight
                        model: kcm.gtkPage.gtkThemesModel
                        currentIndex: model.findThemeIndex(kcm.gtkPage.gtkThemesModel.selectedTheme)
                        onCurrentTextChanged: function() {
                            model.selectedTheme = currentText
                            gtkRemoveButton.enabled = model.selectedThemeRemovable()
                        }
                        onActivated: model.setSelectedThemeDirty()
                        textRole: "theme-name"
                        delegate: QtControls.ItemDelegate {
                            id: gtkThemeDelegate

                            required property int index
                            required property var model

                            width: parent?.width ?? 0
                            highlighted: gtkThemeCombo.highlightedIndex === index
                            text: model["theme-name"]

                            contentItem: Kirigami.TitleSubtitle {
                                title: gtkThemeDelegate.text
                                subtitle: i18nc("@item:inlistbox Supported GTK versions, %1 is a list of numbers", "GTK %1", gtkThemeDelegate.model["gtk-versions"].join(", "))
                                font: gtkThemeDelegate.font
                                selected: gtkThemeDelegate.highlighted || gtkThemeDelegate.down
                            }
                        }
                    }

                    QtControls.Button {
                        id: gtkRemoveButton
                        height: gtkThemeControls.controlHeight
                        icon.name: "edit-delete"
                        onClicked: gtkThemeCombo.model.removeSelectedTheme()
                    }

                    QtControls.Button {
                        id: gtkPreviewButton
                        height: gtkThemeControls.controlHeight
                        icon.name: "preview"
                        text: i18n("Preview…")
                        onClicked: kcm.gtkPage.showGtkPreview()
                        visible: kcm.gtkPage.gtkPreviewAvailable()
                        enabled: kcm.gtkPage.gtkThemesModel.selectedThemeSupportsGtk4

                        QtControls.ToolTip {
                            text: i18n("Preview supports GTK4 themes only")
                            delay: Kirigami.Units.toolTipDelay
                            visible: gtkPreviewButton.hovered && !gtkPreviewButton.enabled
                        }
                    }
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: kcm.gtkPage.gtkThemesModel.selectedThemeMissingGtkVersions.length > 0
            text: i18nc("@info %1 is a list of GTK versions",
                        "This theme does not support GTK %1. Applications built with those versions will fall back to GTK defaults.",
                        kcm.gtkPage.gtkThemesModel.selectedThemeMissingGtkVersions.join(", "))
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
