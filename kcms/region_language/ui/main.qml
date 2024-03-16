/*
  SPDX-FileCopyrightLabel: 2021 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
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
        function onDefaultsClicked() {
            while (kcm.depth > 1) {
                kcm.takeLast();
            }
            if (languageSelectPage.active) {
                languageSelectPage.active = false;
            }
        }
        function onLoadClicked() {
            while (kcm.depth > 1) {
                kcm.takeLast();
            }
            if (languageSelectPage.active) {
                languageSelectPage.active = false;
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
        delegate: Kirigami.SubtitleDelegate {
            id: optionsDelegate

            required property int index
            required property var model

            width: ListView.view.width

            text: model.name
            subtitle: {
                if (model.page === SettingType.Lang) {
                    return model.localeName;
                }
                return model.example;
            }

            Kirigami.Theme.useAlternateBackgroundColor: true
            hoverEnabled: false
            down: false

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.TitleSubtitle {
                    Layout.fillWidth: true
                    title: optionsDelegate.text
                    subtitle: optionsDelegate.subtitle
                    reserveSpaceForSubtitle: true
                }

                QQC2.Button {
                    id: changeButton
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
                        } else if (model.page === SettingType.BinaryDialect) {
                            binaryDialectSelectPage.active = true;
                            kcm.push(binaryDialectSelectPage.item);
                        } else {
                            localeListPage.active = true;
                            localeListPage.item.setting = optionsDelegate.model.page;
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
        id: binaryDialectSelectPage
        active: false
        sourceComponent: KCM.ScrollViewKCM {
            title: i18n("Data and Storage Sizes")

            BinaryDialectModel {
                id: binaryDialectModel
            }

            view: ListView {
                id: binaryDialectListView
                clip: true
                model: binaryDialectModel
                delegate: Kirigami.SubtitleDelegate {
                    id: binaryDialectDelegate

                    required property var model

                    width: ListView.view.width

                    text: model.name
                    subtitle: model.example ? model.example : ""

                    Kirigami.Theme.useAlternateBackgroundColor: true

                    contentItem: RowLayout {
                        Kirigami.TitleSubtitle {
                            id: titleSubtitle
                            Layout.fillWidth: true
                            title: binaryDialectDelegate.text
                            subtitle: binaryDialectDelegate.subtitle
                            selected: model.index === kcm.optionsModel.binaryDialect
                        }

                        QQC2.Label {
                            color: !titleSubtitle.selected ? Kirigami.Theme.disabledTextColor: titleSubtitle.color
                            text: model.description
                        }
                    }

                    onClicked: {
                        kcm.optionsModel.binaryDialect = model.index;
                        kcm.takeLast();
                    }
                }
            }
        }
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
                filterRoleName: "filter"
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
                delegate: Kirigami.SubtitleDelegate {
                    id: localeDelegate

                    required property var model

                    width: ListView.view.width

                    icon.name: model.flag
                    text: model.display
                    subtitle: model.example ? model.example : ""

                    Kirigami.Theme.useAlternateBackgroundColor: true

                    contentItem: RowLayout {
                        Kirigami.IconTitleSubtitle {
                            id: icontTitle
                            Layout.fillWidth: true
                            icon: icon.fromControlsIcon(localeDelegate.icon)
                            title: localeDelegate.text
                            subtitle: localeDelegate.subtitle
                            selected: {
                                switch (setting) {
                                case SettingType.Lang:
                                    return kcm.settings.lang === model.localeName;
                                case SettingType.Numeric:
                                    return kcm.settings.numeric === model.localeName;
                                case SettingType.Time:
                                    return kcm.settings.time === model.localeName;
                                case SettingType.Currency:
                                    return kcm.settings.monetary === model.localeName;
                                case SettingType.Measurement:
                                    return kcm.settings.measurement === model.localeName;
                                case SettingType.PaperSize:
                                    return kcm.settings.paperSize === model.localeName;
                                case SettingType.Address:
                                    return kcm.settings.address === model.localeName;
                                case SettingType.NameStyle:
                                    return kcm.settings.nameStyle === model.localeName;
                                case SettingType.PhoneNumbers:
                                    return kcm.settings.phoneNumbers === model.localeName;
                                }
                                return false;
                            }
                        }

                        QQC2.Label {
                            color: !icontTitle.selected ? Kirigami.Theme.disabledTextColor: icontTitle.color
                            text: model.localeName
                            textFormat: Text.PlainText
                        }
                    }

                    onClicked: {
                        if (model.localeName !== i18n("Default")) {
                            switch (setting) {
                            case SettingType.Lang:
                                kcm.settings.lang = model.localeName;
                                break;
                            case SettingType.Numeric:
                                kcm.settings.numeric = model.localeName;
                                break;
                            case SettingType.Time:
                                kcm.settings.time = model.localeName;
                                break;
                            case SettingType.Currency:
                                kcm.settings.monetary = model.localeName;
                                break;
                            case SettingType.Measurement:
                                kcm.settings.measurement = model.localeName;
                                break;
                            case SettingType.PaperSize:
                                kcm.settings.paperSize = model.localeName;
                                break;
                            case SettingType.Address:
                                kcm.settings.address = model.localeName;
                                break;
                            case SettingType.NameStyle:
                                kcm.settings.nameStyle = model.localeName;
                                break;
                            case SettingType.PhoneNumbers:
                                kcm.settings.phoneNumbers = model.localeName;
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
