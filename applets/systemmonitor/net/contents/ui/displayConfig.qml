/*
 *   Copyright 2019 George Vogiatzis <Gvgeo@protonmail.com>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import QtQuick.Controls 2.5 as QQC2
import org.kde.kirigami 2.5 as Kirigami

Kirigami.FormLayout {
    property int cfg_displayUnit: plasmoid.configuration.displayUnit

    QQC2.ButtonGroup {
        id: displayUnitGroup
    }

    QQC2.RadioButton {
        id: byteDisplayUnit
        QQC2.ButtonGroup.group: displayUnitGroup

        Kirigami.FormData.label: i18nc("@label", "Display unit:")

        text: i18nc("@option:radio", "Byte")
        checked: cfg_displayUnit == 0
        onClicked: if (checked) {cfg_displayUnit = 0;}
    }

    QQC2.RadioButton {
        id: bitDisplayUnit
        QQC2.ButtonGroup.group: displayUnitGroup

        text: i18nc("@option:radio", "bit")

        checked: cfg_displayUnit == 1
        onClicked: if (checked) {cfg_displayUnit = 1;}
    }
}
