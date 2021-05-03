/*
 *   Copyright 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
 *   Copyright 2013, 2015 Kai Uwe Broulik <kde@privat.broulik.de>
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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kquickcontrolsaddons 2.0

ColumnLayout {
    id: powerManagement
    property alias disabled: pmCheckBox.checked
    property bool pluggedIn

    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: PlasmaCore.Units.smallSpacing

        PlasmaComponents3.CheckBox {
            id: pmCheckBox
            Layout.fillWidth: true
            text: i18nc("Minimize the length of this string as much as possible", "Inhibit automatic sleep and screen locking")
            checked: false
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: PlasmaCore.Units.gridUnit + PlasmaCore.Units.smallSpacing // width of checkbox and spacer
        spacing: PlasmaCore.Units.smallSpacing

        InhibitionHint {
            Layout.fillWidth: true
            visible: pmSource.data["PowerDevil"] && pmSource.data["PowerDevil"]["Is Lid Present"] && !pmSource.data["PowerDevil"]["Triggers Lid Action"] ? true : false
            iconSource: "computer-laptop"
            text: i18nc("Minimize the length of this string as much as possible", "Your notebook is configured not to sleep when closing the lid while an external monitor is connected.")
        }


        // UI to display when there is only one inhibition
        InhibitionHint {
            // Don't need to show the inhibitions when power management
            // isn't enabled anyway
            visible: inhibitions.length === 1 && !powerManagement.disabled
            Layout.fillWidth: true
            iconSource: visible ? inhibitions[0].Icon : ""
            text: visible ?
                    inhibitions[0].Reason ?
                        i18n("%1 is inhibiting sleep and screen locking (%2)", inhibitions[0].Name, inhibitions[0].Reason)
                    :
                        i18n("%1 is inhibiting sleep and screen locking (unknown reason)", inhibitions[0].Name)
                :
                    ""
        }


        // UI to display when there is more than one inhibition
        PlasmaComponents3.Label {
            id: inhibitionExplanation
            Layout.fillWidth: true
            // Don't need to show the inhibitions when power management
            // isn't enabled anyway
            visible: inhibitions.length > 1 && !powerManagement.disabled
            font: PlasmaCore.Theme.smallestFont
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 3
            text: i18np("%1 application is inhibiting sleep and screen locking:",
                        "%1 applications are inhibiting sleep and screen locking:",
                        inhibitions.length)
        }
        Repeater {
            visible: inhibitions.length > 1

            model: inhibitionExplanation.visible ? inhibitions.length : null

            InhibitionHint {
                Layout.fillWidth: true
                iconSource: inhibitions[index].Icon || ""
                text: inhibitions[index].Reason ?
                                                i18nc("Application name: reason for preventing sleep and screen locking", "%1: %2", inhibitions[index].Name, inhibitions[index].Reason)
                                                : i18nc("Application name: reason for preventing sleep and screen locking", "%1: unknown reason", inhibitions[index].Name)
            }
        }
    }
}

