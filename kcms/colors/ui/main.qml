/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtCore
import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Dialogs 6.3 as QtDialogs
import QtQuick.Controls 2.3 as QtControls
import QtQuick.Templates 2.3 as T
import QtQml 2.15

import org.kde.kirigami 2.8 as Kirigami
import org.kde.newstuff 1.91 as NewStuff
import org.kde.kcmutils as KCM
import org.kde.private.kcms.colors 1.0 as Private

KCM.GridViewKCM {
    id: root

    view.model: kcm.filteredModel
    view.currentIndex: kcm.filteredModel.selectedSchemeIndex

    Binding {
        target: kcm.filteredModel
        property: "query"
        value: searchField.text
        restoreMode: Binding.RestoreBinding
    }

    Binding {
        target: kcm.filteredModel
        property: "filter"
        value:  filterCombo.model[filterCombo.currentIndex].filter
        restoreMode: Binding.RestoreBinding
    }

    KCM.SettingStateBinding {
        configObject: kcm.colorsSettings
        settingName: "colorScheme"
        extraEnabledConditions: !kcm.downloadingFile
    }

    KCM.SettingHighlighter {
        target: colourFlow
        highlight: !Qt.colorEqual(kcm.accentColor, "transparent")
    }

    // The thumbnails are a bit more elaborate and need more room, especially when translated
    view.implicitCellWidth: Kirigami.Units.gridUnit * 15
    view.implicitCellHeight: Kirigami.Units.gridUnit * 13

    // we have a duplicate property here as "var" instead of "color", so that we
    // can set it to "undefined", which lets us use the "a || b" shorthand for
    // "a if a is defined, otherwise b"
    readonly property var accentColor: Qt.colorEqual(kcm.accentColor, "transparent") ? undefined : kcm.accentColor

    DropArea {
        anchors.fill: parent
        onEntered: {
            if (!drag.hasUrls) {
                drag.accepted = false;
            }
        }
        onDropped: {
            infoLabel.visible = false;
            kcm.installSchemeFromFile(drop.urls[0]);
        }
    }

    // putting the InlineMessage as header item causes it to show up initially despite visible false
    header: ColumnLayout {
        Kirigami.InlineMessage {
            id: notInstalledWarning
            Layout.fillWidth: true

            type: Kirigami.MessageType.Warning
            showCloseButton: true
            visible: false

            Connections {
                target: kcm
                function onShowSchemeNotInstalledWarning(schemeName) {
                    notInstalledWarning.text = i18n("The color scheme '%1' is not installed. Selecting the default theme instead.", schemeName)
                    notInstalledWarning.visible = true;
                }
            }
        }

        AccentColorUI {
            id: colourFlow
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.bottomMargin: Kirigami.Units.smallSpacing
        }

        RowLayout {
            Layout.fillWidth: true

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
            }

            QtControls.ComboBox {
                id: filterCombo
                Layout.rightMargin: parent.spacing * 3
                textRole: "text"
                model: [
                    {text: i18n("All Schemes"), filter: Private.KCM.AllSchemes},
                    {text: i18n("Light Schemes"), filter: Private.KCM.LightSchemes},
                    {text: i18n("Dark Schemes"), filter: Private.KCM.DarkSchemes}
                ]

                // HACK QQC2 doesn't support icons, so we just tamper with the desktop style ComboBox's background
                // and inject a nice little filter icon.
                Component.onCompleted: {
                    if (!background || !background.hasOwnProperty("properties")) {
                        // not a KQuickStyleItem
                        return;
                    }

                    var props = background.properties || {};

                    background.properties = Qt.binding(function() {
                        var newProps = props;
                        newProps.currentIcon = "view-filter";
                        newProps.iconColor = Kirigami.Theme.textColor;
                        return newProps;
                    });
                }
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Install from File…")
            icon.name: "document-import"
            onTriggered: fileDialogLoader.active = true
        },
        NewStuff.Action {
            text: i18n("Get New…")
            configFile: "colorschemes.knsrc"
            onEntryEvent: function (entry, event) {
                if (event == NewStuff.Entry.StatusChangedEvent) {
                    kcm.knsEntryChanged(entry)
                } else if (event == NewStuff.Entry.AdoptedEvent) {
                    kcm.loadSelectedColorScheme()
                }
            }
        }
    ]

    RowLayout {
        view.delegate: ColorSchmeGridDelegate {
            type: ColorSchmeGrid.Type.Light
        }
        view.delegate: ColorSchmeGridDelegate {
            type: ColorSchmeGrid.Type.Dark
        }
    }

    footer: ColumnLayout {
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
                    // Avoid dual message widgets
                    notInstalledWarning.visible = false;
                }
                function onShowErrorMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Error;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                    notInstalledWarning.visible = false;
                }
            }
        }
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: QtDialogs.FileDialog {
            title: i18n("Open Color Scheme")
            currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
            nameFilters: [ i18n("Color Scheme Files (*.colors)") ]
            Component.onCompleted: open()
            onAccepted: {
                infoLabel.visible = false;
                kcm.installSchemeFromFile(selectedFile)
                fileDialogLoader.active = false
            }
            onRejected: {
                fileDialogLoader.active = false
            }
        }
    }
}
