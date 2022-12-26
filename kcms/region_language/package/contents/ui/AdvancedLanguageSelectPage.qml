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

import org.kde.kirigami 2.15 as Kirigami
import org.kde.kcm 1.2 as KCM
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
    header: ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            text: i18n("Putting American English above other languages will cause undesired behavior in some applications. If you would like your system to use American English, remove all other languages.")
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            visible: languageListModel.selectedLanguageModel.shouldWarnMultipleLang
        }

        Kirigami.InlineMessage {
            id: unsupportedLanguageMsg
            text: i18nc("Error message, %1 is the language name", "The language \"%1\" is unsupported", languageListModel.selectedLanguageModel.unsupportedLanguage)
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            visible: languageListModel.selectedLanguageModel.unsupportedLanguage.length > 0
        }

        QQC2.Label {
            horizontalAlignment: Qt.AlignHCenter
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            text: i18n("Add languages in the order you want to see them in your applications.")
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
                        listView: languageListView
                        visible: languageListView.count > 1
                        onMoveRequested: {
                            languageListModel.selectedLanguageModel.move(oldIndex, newIndex);
                        }
                    }

                    QQC2.Label {
                        Layout.alignment: Qt.AlignLeft
                        Layout.fillWidth: true
                        text: model.display

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
                        iconName: "go-top"
                        tooltip: i18nc("@info:tooltip", "Move to top")
                        onTriggered: languageListModel.selectedLanguageModel.move(index, 0)
                    },
                    Kirigami.Action {
                        visible: languageListView.count > 1
                        iconName: "edit-delete"
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

        Kirigami.BasicListItem  {
            id: languageItem

            width: availableLanguagesList.width
            reserveSpaceForIcon: false

            label: model.nativeName
            action: Kirigami.Action {
                onTriggered: {
                    if (replaceLangIndex >= 0) {
                        languageListModel.selectedLanguageModel.replaceLanguage(replaceLangIndex, model.languageCode);
                        replaceLangIndex = -1;
                    } else {
                        languageListModel.selectedLanguageModel.addLanguage(model.languageCode);
                    }
                    addLanguagesSheet.close();
                }
            }
        }
    }    

    Kirigami.OverlaySheet {
        id: addLanguagesSheet
        property string titleText: i18nc("@title:window", "Add Languages")
        parent: languageSelectPage

        topPadding: 0
        leftPadding: 0
        rightPadding: 0
        bottomPadding: 0

        title: titleText

        ListView {
            id: availableLanguagesList
            implicitWidth: 18 * Kirigami.Units.gridUnit
            model: languageListModel
            delegate: addLanguageItemComponent
            cacheBuffer: Math.max(0, contentHeight)
            reuseItems: true
        }
        onSheetOpenChanged: {
            if (!sheetOpen) {
                titleText = i18nc("@title:window", "Add Languages");
            }
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
            checked: addLanguagesSheet.sheetOpen
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            QQC2.Label {
                text: languageListModel.numberExample
            }
            QQC2.Label {
                text: languageListModel.currencyExample
            }
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: languageListModel.metric
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: languageListModel.timeExample
        }
    }
}
