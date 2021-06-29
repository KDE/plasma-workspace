/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QtControls
import org.kde.kirigami 2.5 as Kirigami
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.kcm 1.2
import org.kde.kitemmodels 1.0 as KItemModels

ScrollViewKCM {
    id: root

    ConfigModule.quickHelp: i18n("Language")

    implicitWidth: Kirigami.Units.gridUnit * 25
    implicitHeight: Kirigami.Units.gridUnit * 25

    KItemModels.KSortFilterProxyModel {
        id: selectedTranslationsModel
        sourceModel: kcm.translationsModel
        filterRole: "IsSelected"
        filterString: "true"
        sortRole: "SelectionPreference"
    }

    KItemModels.KSortFilterProxyModel {
        id: availableTranslationsModel
        sourceModel: kcm.translationsModel
        filterRole: "IsSelected"
        filterString: "false"
        sortRole: "DisplayRole"
        sortCaseSensitivity: Qt.CaseInsensitive
    }

    Component {
        id: addLanguageItemComponent

        Kirigami.BasicListItem {
            id: languageItem

            property string languageCode: model ? model.LanguageCode : null

            width: availableLanguagesList.width
            reserveSpaceForIcon: false

            label: model.display

            checkable: true
            onCheckedChanged: {
                if (checked) {
                    addLanguagesSheet.selectedLanguages.push(model.LanguageCode);

                    // There's no property change notification for pushing to an array
                    // in a var prop, so we can't bind selectedLanguages.length to
                    // addLanguagesButton.enabled.
                    addLanguagesButton.enabled = true;
                } else {
                    addLanguagesSheet.selectedLanguages = addLanguagesSheet.selectedLanguages.filter(function(item) { return item !== model.LanguageCode });

                    // There's no property change notification for pushing to an array
                    // in a var prop, so we can't bind selectedLanguages.length to
                    // addLanguagesButton.enabled.
                    if (addLanguagesSheet.selectedLanguages.length > 0) {
                        addLanguagesButton.enabled = false;
                    }
                }
            }

            data: [Connections {
                target: addLanguagesSheet

                function onSheetOpenChanged() {
                    languageItem.checked = false
                }
            }]
        }
    }

    Kirigami.OverlaySheet {
        id: addLanguagesSheet

        parent: root.parent

        topPadding: 0
        leftPadding: 0
        rightPadding: 0
        bottomPadding: 0

        title: i18nc("@title:window", "Add Languages")

        property var selectedLanguages: []

        onSheetOpenChanged: selectedLanguages = []

        ListView {
            id: availableLanguagesList

            implicitWidth: 18 * Kirigami.Units.gridUnit
            model: availableTranslationsModel
            delegate: addLanguageItemComponent
        }

        footer: RowLayout {
            QtControls.Button {
                id: addLanguagesButton

                Layout.alignment: Qt.AlignHCenter

                text: i18nc("@action:button", "Add")

                enabled: false

                onClicked: {
                    var langs = kcm.translationsModel.selectedLanguages.slice();
                    langs = langs.concat(addLanguagesSheet.selectedLanguages);

                    kcm.translationsModel.selectedLanguages = langs;

                    addLanguagesSheet.sheetOpen = false;
                }
            }
        }
    }

    header: ColumnLayout {
        id: messagesLayout

        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true

            type: Kirigami.MessageType.Error

            text: i18nc("@info", "There are no additional languages available on this system.")

            visible: !availableLanguagesList.count
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true

            type: kcm.everSaved ? Kirigami.MessageType.Positive : Kirigami.MessageType.Information

            text: (kcm.everSaved ? i18nc("@info", "Your changes will take effect the next time you log in.")
                : i18nc("@info", "There are currently no preferred languages configured."))

            visible: !languagesList.count || kcm.everSaved
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true

            type: Kirigami.MessageType.Error

            text: {
                // Don't eval the i18ncp call when we have no missing languages. It causes unnecesssary warnings
                // as %2 will be "" and thus considered missing.
                if (!visible) {
                    return ""
                }
                i18ncp("@info %2 is the language code",
                    "The translation files for the language with the code '%2' could not be found. The language will be removed from your configuration. If you want to add it back, please install the localization files for it and add the language again.",
                    "The translation files for the languages with the codes '%2' could not be found. These languages will be removed from your configuration. If you want to add them back, please install the localization files for it and add the languages again.",
                    kcm.translationsModel.missingLanguages.length,
                    kcm.translationsModel.missingLanguages.join(i18nc("@info separator in list of language codes", "', '")))
            }

            visible: kcm.translationsModel.missingLanguages.length > 0
        }

        QtControls.Label {
            Layout.fillWidth: true

            visible: languagesList.count

            text: i18n("The language at the top of this list is the one you want to see and use most often.")
            wrapMode: Text.WordWrap
        }
    }

    Component {
        id: languagesListItemComponent

        Item {
            width: ListView.view.width
            height: listItem.implicitHeight

            Kirigami.SwipeListItem {
                id: listItem

                contentItem: RowLayout {
                    Kirigami.ListItemDragHandle {
                        listItem: listItem
                        listView: languagesList
                        onMoveRequested: kcm.translationsModel.move(oldIndex, newIndex)
                    }

                    QtControls.BusyIndicator {
                        visible: model.IsInstalling
                        running: visible
                        // the control style (e.g. org.kde.desktop) may force a padding that will shrink the indicator way down. ignore it.
                        padding: 0

                        Layout.alignment: Qt.AlignVCenter

                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: implicitWidth

                        QtControls.ToolTip {
                            text: xi18nc('@info:tooltip/rich',
                                         'Installing missing packages to complete this translation.')
                        }

                    }

                    Kirigami.Icon {
                        visible: model.IsIncomplete

                        Layout.alignment: Qt.AlignVCenter

                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: implicitWidth

                        source: "data-warning"
                        color: Kirigami.Theme.negativeTextColor
                        MouseArea {
                            id: area
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                        QtControls.ToolTip {
                            visible: area.containsMouse
                            text: xi18nc('@info:tooltip/rich',
                                         `Not all translations for this language are installed.
                                          Use the <interface>Install Missing Packages</interface> button to download
                                          and install all missing packages.`)
                        }

                    }

                    QtControls.Label {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter

                        text: switch(index){
                            // Don't assing undefind to string if the index is invalid.
                            case -1: ""; break;
                            case 0: i18nc("@item:inlistbox 1 = Language name", "%1 (Default)", model.display); break;
                            default: model.display; break;
                        }

                        color: (listItem.checked || (listItem.pressed && !listItem.checked && !listItem.sectionDelegate)) ? listItem.activeTextColor : listItem.textColor
                    }
                }

            actions: [
                Kirigami.Action {
                    visible: model.IsIncomplete
                    iconName: "install"
                    tooltip: i18nc("@info:tooltip", "Install Missing Packages")
                    onTriggered: model.Object.complete()
                },
                Kirigami.Action {
                    enabled: index > 0
                    visible: languagesList.count > 1
                    iconName: "go-top"
                    tooltip: i18nc("@info:tooltip", "Promote to default")
                    onTriggered: kcm.translationsModel.move(index, 0)
                },
                Kirigami.Action {
                    iconName: "edit-delete"
                    visible: languagesList.count > 1
                    tooltip: i18nc("@info:tooltip", "Remove")
                    onTriggered: kcm.translationsModel.remove(model.LanguageCode);
                }]
            }
        }
    }

    view: ListView {
        id: languagesList

        model: selectedTranslationsModel
        delegate: languagesListItemComponent
    }

    footer: RowLayout {
        id: footerLayout

        QtControls.Button {
            Layout.alignment: Qt.AlignRight

            enabled: availableLanguagesList.count

            text: i18nc("@action:button", "Add languages…")

            onClicked: addLanguagesSheet.sheetOpen = !addLanguagesSheet.sheetOpen

            checkable: true
            checked: addLanguagesSheet.sheetOpen
        }
    }
}

