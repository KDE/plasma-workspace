/*
    SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    property alias cfg_compactView: compactViewRadioButton.checked
    property alias cfg_allScreens: allScreensCheckBox.checked

    Kirigami.Form {
        Kirigami.FormGroup {
            QQC2.ButtonGroup { id: buttonGroup }
            Kirigami.FormEntry {
                contentItem: QQC2.RadioButton {
                    id: compactViewRadioButton
                    QQC2.ButtonGroup.group: buttonGroup
                    text: i18nc("@option:radio", "Use single button for application menu")
                }
            }

            Kirigami.FormEntry {
                contentItem: QQC2.RadioButton {
                    id: fullViewRadioButton
                    QQC2.ButtonGroup.group: buttonGroup
                    checked: !compactViewRadioButton.checked
                    text: i18nc("@option:radio", "Show full application menu")
                }
            }
        }

        Kirigami.FormGroup {
            Kirigami.FormEntry {
                contentItem: QQC2.CheckBox {
                    id: allScreensCheckBox
                    text: i18nc("@option:check", "Show menus for apps on different screens")
                }
            }
        }
    }
}
