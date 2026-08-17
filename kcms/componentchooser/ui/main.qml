/*
    SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: root

    function unsupportedMimeText(kcm_component) {
        return i18n("’%1’ seems to not support the following mimetypes associated with this kind of application: %2", kcm_component.applications[kcm_component.index]["name"], kcm_component.unsupportedMimeTypes.join(", "))
    }

    topPadding: 0
    bottomPadding: Kirigami.Units.gridUnit

    ComponentOverlay {
        id: overlay
        parent: root.QQC2.Overlay.overlay
        width: Math.min(root.width, Kirigami.Units.gridUnit * 25)
    }

    Kirigami.SizeGroup {
        id: sizeGroup
        mode: Kirigami.SizeGroup.Width
    }

    component ChooserFormEntry: Kirigami.FormEntry {
        id: formEntry
        required property var component
        contentItem: ColumnLayout {
            spacing: parent.spacing ?? Kirigami.Units.smallSpacing
            Kirigami.FormData.buddyFor: comboBox
            ComponentComboBox {
                id: comboBox
                component: formEntry.component
                KCM.SettingHighlighter {
                    highlight: !formEntry.component.isDefaults
                }
                Component.onCompleted: sizeGroup.items.push(comboBox);
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.maximumWidth: comboBox.width
                visible: formEntry.component.unsupportedMimeTypes.length > 0 || formEntry.component.mimeTypesNotAssociated.length > 0
                type: Kirigami.MessageType.Warning
                text: i18nc("@info:status", "This application may not be able to open all file types.");
                actions: Kirigami.Action {
                    text: i18nc("@action:button", "View Details")
                    onTriggered: {
                        overlay.componentChooser = formEntry.component
                        overlay.open()
                    }
                }
            }
        }
    }

    Kirigami.Form {
        id: form

        Kirigami.FormGroup {
            title: i18nc("Internet related application’s category’s name", "Internet")

            ChooserFormEntry {
                title: i18n("Web browser:")
                component: kcm.browsers
            }

            ChooserFormEntry {
                title: i18n("Email client:")
                component: kcm.emailClients
            }

            ChooserFormEntry {
                title: i18nc("Default calendar application", "Calendar:")
                component: kcm.calendar
            }

            ChooserFormEntry {
                title: i18nc("Default phone app", "Phone Numbers:")
                component: kcm.telUriHandlers
            }
        }

        Kirigami.FormGroup {
            title: i18nc("Multimedia related application’s category’s name", "Multimedia")

            ChooserFormEntry {
                title: i18n("Image viewer:")
                component: kcm.imageViewers
            }

            ChooserFormEntry {
                title: i18n("Music player:")
                component: kcm.musicPlayers
            }

            ChooserFormEntry {
                title: i18n("Video player:")
                component: kcm.videoPlayers
            }
        }

        Kirigami.FormGroup {
            title: i18nc("Documents related application’s category’s name", "Documents")

            ChooserFormEntry {
                title: i18n("Text editor:")
                component: kcm.textEditors
            }


            ChooserFormEntry {
                title: i18n("PDF viewer:")
                component: kcm.pdfViewers
            }
        }

        Kirigami.FormGroup {
            title: i18nc("Utilities related application’s category’s name", "Utilities")

            ChooserFormEntry {
                title: i18n("File manager:")
                component: kcm.fileManagers
            }

            ChooserFormEntry {
                title: i18n("Terminal emulator:")
                component: kcm.terminalEmulators
            }

            ChooserFormEntry {
                title: i18n("Archive manager:")
                component: kcm.archiveManagers
            }

            ChooserFormEntry {
                title: i18nc("Map related application’s category’s name", "Map:")
                component: kcm.geoUriHandlers
            }
        }
    }
}
