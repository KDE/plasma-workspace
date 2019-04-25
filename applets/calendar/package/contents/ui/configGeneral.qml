/*
 * Copyright 2015 Martin Klapetek <mklapetek@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.5
import QtQuick.Controls 2.5 as QtControls
import org.kde.kirigami 2.5 as Kirigami

Kirigami.FormLayout {
    id: generalPage

    anchors.left: parent.left
    anchors.right: parent.right

    property alias cfg_showWeekNumbers: showWeekNumbers.checked
    property string cfg_compactDisplay


    QtControls.CheckBox {
        id: showWeekNumbers

        Kirigami.FormData.label: i18n("Calendar version:")

        text: i18n("Show week numbers")
    }


    Item {
        Kirigami.FormData.isSection: true
    }


    QtControls.RadioButton {
        Kirigami.FormData.label: i18nc("What information is shown in the calendar icon", "Compact version:")

        text: i18nc("Show the number of the day (eg. 31) in the icon", "Show day of the month")

        checked: cfg_compactDisplay == "d"
        onCheckedChanged: if (checked) cfg_compactDisplay = "d"
    }
    QtControls.RadioButton {
        text: i18nc("Show the week number (eg. 50) in the icon", "Show week number")

        checked: cfg_compactDisplay == "w"
        onCheckedChanged: if (checked) cfg_compactDisplay = "w"
    }
}
