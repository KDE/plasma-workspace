/*
  SPDX-FileCopyrightLabel: 2021 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.15 as Kirigami
import org.kde.kcm 1.2 as KCM
import LocaleListModel 1.0

KCM.ScrollViewKCM {
    id: root
    implicitHeight: Kirigami.Units.gridUnit * 40
    implicitWidth: Kirigami.Units.gridUnit * 20
    header: Kirigami.InlineMessage {
        id: helpMsg
        text: i18n("Your changes will take effect the next time you log in.")
    }

    view: ListView {
        model: kcm.optionsModel
        delegate: Kirigami.BasicListItem {
            text: model.name
            subtitle: model.localeName
            trailing: QQC2.Label {
                text: model.example
            }
            reserveSpaceForSubtitle: true
            onClicked: {
                if (kcm.depth === 1) {
                    localeListPage.active = true;
                    localeListPage.item.setting = page;
                    kcm.push(localeListPage.item);
                } else {
                    kcm.getSubPage(0).setting = page;
                    kcm.getSubPage(0).filterText = '';
                    kcm.currentIndex = 1;
                }
            }
        }
    }

    Loader {
        id: localeListPage
        active: false
        sourceComponent: KCM.ScrollViewKCM {
            property string setting: "lang"
            property alias filterText: searchField.text
            title: {
                localeListView.currentIndex = -1;
                localeListModel.selectedConfig = setting;
                switch (setting) {
                case "lang":
                    return i18n("Region");
                case "numeric":
                    return i18n("Numbers");
                case "time":
                    return i18n("Time");
                case "currency":
                    return i18n("Currency");
                case "measurement":
                    return i18n("Measurement");
                case "collate":
                    return i18n("Collate");
                }
            }

            LocaleListModel {
                id: localeListModel
            }

            header: Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                onTextChanged: localeListModel.filter = text
            }

            view: ListView {
                id: localeListView
                clip: true
                model: localeListModel
                delegate: Kirigami.BasicListItem {
                    icon: model.flag
                    text: model.display
                    subtitle: model.localeName
                    trailing: QQC2.Label {
                        color: Kirigami.Theme.disabledTextColor
                        text: model.example ? model.example : ''
                    }
                    onClicked: {
                        if (model.localeName !== i18n("Default")) {
                            switch (setting) {
                            case "lang":
                                kcm.settings.lang = localeName;
                                break;
                            case "numeric":
                                kcm.settings.numeric = localeName;
                                break;
                            case "time":
                                kcm.settings.time = localeName;
                                break;
                            case "currency":
                                kcm.settings.monetary = localeName;
                                break;
                            case "measurement":
                                kcm.settings.measurement = localeName;
                                break;
                            case "collate":
                                kcm.settings.collate = localeName;
                                break;
                            }
                        } else {
                            kcm.unset(setting);
                        }

                        kcm.currentIndex = 0;
                        helpMsg.visible = true;
                    }
                }
            }
        }
    }
}
