/*
  SPDX-FileCopyrightLabel: 2021 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.7 as Kirigami
import org.kde.kcm 1.2

SimpleKCM {
    id: root
    implicitHeight: Kirigami.Units.gridUnit * 40
    implicitWidth: Kirigami.Units.gridUnit * 20
    Kirigami.FormLayout {
        id: formLayout

        Kirigami.BasicListItem {
            text: i18n("Region")
            subtitle: kcm.settings.lang
            onClicked: {
                pushWithSetting("lang")
            }
            Layout.preferredWidth: root.width
        }
        Kirigami.BasicListItem {
            text: i18n("Number")
            subtitle: kcm.settings.numeric
            trailing: Text {
                text: kcm.numberExample
            }
            onClicked: {
                pushWithSetting("numeric")
            }
            Layout.preferredWidth: root.width
        }
        Kirigami.BasicListItem {
            text: i18n("Time")
            subtitle: kcm.settings.time
            trailing: Text {
                text: kcm.timeExample
            }
            onClicked: {
                pushWithSetting("time")
            }
            Layout.preferredWidth: root.width
        }
        Kirigami.BasicListItem {
            text: i18n("Currency")
            subtitle: kcm.settings.monetary
            trailing: Text {
                text: kcm.currencyExample
            }
            onClicked: {
                pushWithSetting("currency")
            }
            Layout.preferredWidth: root.width
        }
        Kirigami.BasicListItem {
            text: i18n("Measurement")
            subtitle: kcm.settings.measurement
            trailing: Text {
                text: kcm.measurementExample
            }
            onClicked: {
                pushWithSetting("measurement")
            }
            Layout.preferredWidth: root.width
        }
        Kirigami.BasicListItem {
            text: i18n("Collate and Sorting")
            subtitle: kcm.settings.collate
            trailing: Text {
                text: kcm.collateExample
            }
            onClicked: {
                pushWithSetting("collate")
            }
            Layout.preferredWidth: root.width
        }
    }

    function pushWithSetting(setting) {
        if (kcm.depth === 1) {
            kcm.push("LocaleListPage.qml", {"setting": setting});
        } else {
            kcm.getSubPage(0).setting = setting;
            kcm.currentIndex = 1;
        }
    }
}
