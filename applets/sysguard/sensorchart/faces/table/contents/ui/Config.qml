/*
 *   Copyright 2019 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
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

import QtQuick 2.9
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2 as Controls

import org.kde.kirigami 2.8 as Kirigami

import org.kde.ksysguard.sensors 1.0 as Sensors

Kirigami.FormLayout {
    id: root

    property alias cfg_showTableHeaders: showTableHeadersCheckbox.checked
    property alias cfg_sortColumn: sortColumnCombo.currentIndex
    property alias cfg_sortDescending: sortDescendingCheckBox.checked

    RowLayout {
        Kirigami.FormData.label: i18n("Sort by:")

        Controls.ComboBox {
            id: sortColumnCombo
            textRole: "display"
            model: Sensors.HeadingHelperModel {
                sourceModel: Sensors.SensorDataModel {
                    sensors: plasmoid.configuration.sensorIds
                }
            }
        }

        Controls.CheckBox {
            id: sortDescendingCheckBox
            // or do a ComboBox like in Dolphin with pretty names "highest first"<->"lowest first"?
            text: i18nc("Sort descending", "Descending")
        }
    }

    Controls.CheckBox {
        id: showTableHeadersCheckbox
        text: i18n("Show table headers")
    }
}

