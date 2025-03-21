/*
  SPDX-FileCopyrightLabel: 2021 Han Young <hanyoung@protonmail.com>
  SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>
  SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KDialogs
import org.kde.kcmutils as KCM
import org.kde.kitemmodels 1.0 as ItemModels

import kcmregionandlang 1.0

KCM.ScrollViewKCM {
    property int replaceLangIndex: -1
    implicitHeight: languageListView.height
    id: languageSelectPage
    title: i18n("Language")
    LanguageListModel {
        id: languageListModel
        Component.onCompleted: {
            languageListModel.setRegionAndLangSettings(kcm.settings, kcm);
        }
    }

    headerPaddingEnabled: false // Let the InlineMessages touch the edges
    header: ColumnLayout {
        spacing: 0

        Kirigami.InlineMessage {
            text: i18n("Putting any other languages below English will cause undesired behavior in some applications. If you would like to use your system in English, remove all other languages.")
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            position: Kirigami.InlineMessage.Position.Header
            visible: languageListModel.selectedLanguageModel.shouldWarnMultipleLang
        }

        Kirigami.InlineMessage {
            id: unsupportedLanguageMsg
            text: i18nc("Error message, %1 is the language name", "The language \"%1\" is unsupported", languageListModel.selectedLanguageModel.unsupportedLanguage)
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            position: Kirigami.InlineMessage.Position.Header
            visible: languageListModel.selectedLanguageModel.unsupportedLanguage.length > 0
        }

        QQC2.Label {
            horizontalAlignment: Qt.AlignHCenter
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            // Equal to the margins removed by disabling header padding
            Layout.margins: Kirigami.Units.mediumSpacing
            text: i18n("Add languages in the order you want to see them in your applications.")
            textFormat: Text.PlainText
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
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.ListItemDragHandle {
                        listItem: listItem
                        listView: languageListView
                        visible: languageListView.count > 1
                        onMoveRequested: (oldIndex, newIndex) => {
                            languageListModel.selectedLanguageModel.move(oldIndex, newIndex);
                        }
                    }

                    QQC2.Label {
                        Layout.alignment: Qt.AlignLeft
                        Layout.fillWidth: true
                        text: model.display
                        textFormat: Text.PlainText

                        color: listItem.checked || (listItem.pressed && !listItem.checked && !listItem.sectionDelegate) ? listItem.activeTextColor : listItem.textColor;
                    }

                    QQC2.Button {
                        Layout.alignment: Qt.AlignRight
                        visible: languageListView.count <= 1
                        text: i18nc("@info:tooltip", "Change Language")
                        icon.name: "configure"
                        onClicked: {
                            replaceLangIndex = index;
                            addLanguagesSheet.titleText = i18nc("@title:window", "Change Language");
                            addLanguagesSheet.open();
                        }
                    }
                }

                actions: [
                    Kirigami.Action {
                        enabled: index > 0
                        visible: languageListView.count > 1
                        icon.name: "go-top"
                        tooltip: i18nc("@info:tooltip", "Move to top")
                        onTriggered: languageListModel.selectedLanguageModel.move(index, 0)
                    },
                    Kirigami.Action {
                        visible: languageListView.count > 1
                        icon.name: "edit-delete"
                        tooltip: i18nc("@info:tooltip", "Remove")
                        onTriggered: languageListModel.selectedLanguageModel.remove(index);
                    }]
            }
        }
    }
    view: ListView {
        id: languageListView
        model: languageListModel.selectedLanguageModel
        delegate: languagesListItemComponent
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: languageListView.count === 0
            text: i18nc("@info:placeholder", "No Language Configured")
        }
    }

    Component {
        id: addLanguageItemComponent

        QQC2.ItemDelegate {
            id: languageItem

            required property string nativeName
            required property string languageCode

            function chooseLanguage() {
                if (replaceLangIndex >= 0) {
                    languageListModel.selectedLanguageModel.replaceLanguage(replaceLangIndex, languageCode);
                    replaceLangIndex = -1;
                } else {
                    languageListModel.selectedLanguageModel.addLanguage(languageCode);
                }
                addLanguagesSheet.close();
            }

            width: availableLanguagesList.width

            text: nativeName

            onClicked: chooseLanguage();
            Keys.onPressed: (event) => {
                if (event.key == Qt.Key_Enter || event.key == Qt.Key_Return) {
                    event.accepted = true;
                    chooseLanguage();
                } else if (event.key == Qt.Key_Up && availableLanguagesList.currentIndex === 0) {
                    event.accepted = true;
                    searchField.forceActiveFocus(Qt.TabFocusReason)
                }
            }
        }
    }

    Kirigami.Dialog {
        id: addLanguagesSheet
        property string titleText: i18nc("@title:window", "Add Languages")

        parent: languageSelectPage
        title: titleText
        preferredWidth: 18 * Kirigami.Units.gridUnit
        implicitHeight: Math.round(parent.height * 0.8)

        header: KDialogs.DialogHeader {
            dialog: addLanguagesSheet

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                KDialogs.DialogHeaderTopContent {
                    dialog: addLanguagesSheet
                }

                Kirigami.SearchField {
                    id: searchField
                    Layout.fillWidth: true
                    focus: true
                    Keys.onDownPressed: event => {
                        availableLanguagesList.currentIndex = 0;
                        availableLanguagesList.forceActiveFocus(Qt.TabFocusReason)
                    }
                }
            }
        }

        onOpened: {
            searchField.forceActiveFocus(Qt.PopupFocusReason)
        }

        onClosed: {
            titleText = i18nc("@title:window", "Add Languages");
            searchField.clear();
        }

        ListView {
            id: availableLanguagesList
            model: ItemModels.KSortFilterProxyModel {
                id: filterModel
                sourceModel: languageListModel
                filterString: searchField.text
                filterRoleName: "nativeName"
                filterCaseSensitivity: Qt.CaseInsensitive
            }
            delegate: addLanguageItemComponent
            cacheBuffer: Math.max(0, contentHeight)
            reuseItems: true
        }
    }
    footer: ColumnLayout {
        QQC2.Button {
            Layout.alignment: Qt.AlignRight
            enabled: availableLanguagesList.count

            text: i18nc("@action:button", "Add More…")

            onClicked: {
                addLanguagesSheet.open();
                replaceLangIndex = -1;
            }

            checkable: true
            checked: addLanguagesSheet.visible
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            QQC2.Label {
                text: languageListModel.numberExample
                textFormat: Text.PlainText
            }
            QQC2.Label {
                text: languageListModel.currencyExample
                textFormat: Text.PlainText
            }
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: languageListModel.metric
            textFormat: Text.PlainText
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: languageListModel.timeExample
            textFormat: Text.PlainText
        }
    }
}
