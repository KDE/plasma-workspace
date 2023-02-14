/*
  SPDX-FileCopyrightLabel: 2021 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.15 as Kirigami
import org.kde.kcm 1.2 as KCM
import org.kde.kitemmodels 1.0
import kcmregionandlang 1.0

KCM.ScrollViewKCM {
    id: root
    implicitHeight: Kirigami.Units.gridUnit * 40
    implicitWidth: Kirigami.Units.gridUnit * 20
    enabled: kcm.enabled

    header: ColumnLayout {
        id: messagesLayout

        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            id: takeEffectNextTimeMsg
            Layout.fillWidth: true

            type: Kirigami.MessageType.Information

            text: i18nc("@info", "Changes will take effect the next time you log in.")

            actions: [
                Kirigami.Action {
                    icon.name: "system-reboot"
                    visible: takeEffectNextTimeMsg.visible && !dontShutdownMsg.visible
                    text: i18n("Restart now")
                    onTriggered: {
                        kcm.reboot()
                    }
                }
            ]
        }

        Kirigami.InlineMessage {
            id: dontShutdownMsg
            Layout.fillWidth: true

            type: Kirigami.MessageType.Warning

            text:  i18nc("@info", "Generating locale and language support files; don't turn off the computer yet.")
        }

        Kirigami.InlineMessage {
            id: installFontMsg
            Layout.fillWidth: true

            type: Kirigami.MessageType.Information

            text: i18nc("@info", "Locale and language support files have been generated, but language-specific fonts could not be automatically installed. If your language requires specialized fonts to be displayed properly, you will need to discover what they are and install them yourself.")
        }
        Kirigami.InlineMessage {
            id: manualInstallMsg
            Layout.fillWidth: true

            type: Kirigami.MessageType.Error
        }
        Kirigami.InlineMessage {
            id: installSuccessMsg
            Layout.fillWidth: true

            type: Kirigami.MessageType.Positive

            text: i18nc("@info", "Necessary locale and language support files have been installed. It is now safe to turn off the computer.")
        }

        Kirigami.InlineMessage {
            id: encountedErrorMsg
            Layout.fillWidth: true

            type: Kirigami.MessageType.Error
        }
    }

    Connections {
        target: kcm
        function onStartGenerateLocale() {
            dontShutdownMsg.visible = true;
        }
        function onRequireInstallFont() {
            dontShutdownMsg.visible = false;
            installFontMsg.visible = true;
        }
        function onUserHasToGenerateManually(reason) {
            manualInstallMsg.text = reason;
            dontShutdownMsg.visible = false;
            manualInstallMsg.visible = true;
        }
        function onGenerateFinished() {
            dontShutdownMsg.visible = false;
            installSuccessMsg.visible = true;
        }
        function onSaveClicked() {
            // return to first page on save action since all messages are here
            while (kcm.depth > 1) {
                kcm.takeLast();
            }
        }
        function onTakeEffectNextTime() {
            takeEffectNextTimeMsg.visible = true;
        }
        function onEncountedError(reason) {
            encountedErrorMsg.text = reason;
            encountedErrorMsg.visible = true;
        }
    }

    view: ListView {
        model: kcm.optionsModel
        delegate: Kirigami.BasicListItem {
            highlighted: false
            hoverEnabled: false
            down: false

            text: model.name
            subtitle: {
                if (model.page === SettingType.Lang) {
                    return model.localeName;
                }
                return model.example;
            }
            reserveSpaceForSubtitle: true
            trailing: Item {
                implicitWidth: changeButton.implicitWidth
                QQC2.Button {
                    id: changeButton
                    anchors.centerIn: parent
                    text: i18nc("@action:button for change the locale used", "Modify…")
                    icon.name: "edit-entry"
                    onClicked: {

                        // remove the excess pages before pushing new page
                        while (kcm.depth > 1) {
                            kcm.takeLast();
                        }

                        if (model.page === SettingType.Lang) {
                            languageSelectPage.active = true;
                            kcm.push(languageSelectPage.item);
                        } else {
                            localeListPage.active = true;
                            localeListPage.item.setting = model.page;
                            localeListPage.item.filterText = '';
                            kcm.push(localeListPage.item);
                        }
                    }
                }
            }
        }
    }

    Loader {
        id: languageSelectPage
        active: false
        source: "AdvancedLanguageSelectPage.qml"
    }

    Loader {
        id: localeListPage
        active: false
        sourceComponent: KCM.ScrollViewKCM {
            property int setting: SettingType.Lang
            property alias filterText: searchField.text
            title: {
                localeListView.currentIndex = -1;
                localeListModel.selectedConfig = setting;
                switch (setting) {
                case SettingType.Numeric:
                    return i18n("Numbers");
                case SettingType.Time:
                    return i18n("Time");
                case SettingType.Currency:
                    return i18n("Currency");
                case SettingType.Measurement:
                    return i18n("Measurements");
                case SettingType.PaperSize:
                    return i18n("Paper Size");
                case SettingType.Address:
                    return i18nc("Postal Address", "Address");
                case SettingType.NameStyle:
                    return i18nc("Name Style", "Name");
                case SettingType.PhoneNumbers:
                    return i18nc("Phone Numbers","Phone number");
                }
                console.warn("Invalid setting passed: ", setting);
                return "Invalid"; // guard
            }

            LocaleListModel {
                id: localeListModel
                Component.onCompleted: {
                    localeListModel.setLang(kcm.settings.lang);
                }
            }

            KSortFilterProxyModel {
                id: filterModel
                sourceModel: localeListModel
                filterRole: "filter"
            }

            Connections {
                target: kcm.settings
                function onLangChanged() {
                    localeListModel.setLang(kcm.settings.lang);
                }
            }

            header: Kirigami.SearchField {
                id: searchField
                onTextChanged: filterModel.filterString = text.toLowerCase()
            }

            view: ListView {
                id: localeListView
                clip: true
                model: filterModel
                delegate: Kirigami.BasicListItem {
                    icon: model.flag
                    text: model.display
                    subtitle: model.example ? model.example : ''
                    trailing: QQC2.Label {
                        color: Kirigami.Theme.disabledTextColor
                        text: model.localeName
                    }

                    onClicked: {
                        if (model.localeName !== i18n("Default")) {
                            switch (setting) {
                            case SettingType.Lang:
                                kcm.settings.lang = localeName;
                                break;
                            case SettingType.Numeric:
                                kcm.settings.numeric = localeName;
                                break;
                            case SettingType.Time:
                                kcm.settings.time = localeName;
                                break;
                            case SettingType.Currency:
                                kcm.settings.monetary = localeName;
                                break;
                            case SettingType.Measurement:
                                kcm.settings.measurement = localeName;
                                break;
                            case SettingType.PaperSize:
                                kcm.settings.paperSize = localeName;
                                break;
                            case SettingType.Address:
                                kcm.settings.address = localeName;
                                break;
                            case SettingType.NameStyle:
                                kcm.settings.nameStyle = localeName;
                                break;
                            case SettingType.PhoneNumbers:
                                kcm.settings.phoneNumbers = localeName;
                                break;
                            }
                        } else {
                            kcm.unset(setting);
                        }

                        kcm.takeLast();
                    }
                }
            }
        }
    }
}
