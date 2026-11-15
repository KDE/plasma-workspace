/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: root
    property alias cfg_showWeekNumbers: showWeekNumbers.checked
    property string cfg_compactDisplay

    Kirigami.Form {
        Kirigami.FormGroup {
            Kirigami.FormEntry {
                title: i18nc("@option:check prefix", "Full calendar:")
                contentItem: QtControls.CheckBox {
                    id: showWeekNumbers
                    text: i18nc("@option:check", "Show week numbers")
                }
            }
        }
        Kirigami.FormGroup {
            Kirigami.FormEntry {
                title: i18nc("@title group radio button prefix What information is shown in the calendar icon", "Icon:")
                contentItem: QtControls.RadioButton {

                    text: i18nc("@option: radio Show the number of the day (eg. 31) in the icon", "Show day of the month")

                    checked: root.cfg_compactDisplay == "d"
                    onCheckedChanged: if (checked) root.cfg_compactDisplay = "d"
                }
            }
            Kirigami.FormEntry {
                contentItem: QtControls.RadioButton {
                    text: i18nc("option:radio Show the week number (eg. 50) in the icon", "Show week number")

                    checked: root.cfg_compactDisplay == "w"
                    onCheckedChanged: if (checked) root.cfg_compactDisplay = "w"
                }
            }
        }
    }
}
