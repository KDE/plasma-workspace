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
import org.kde.ksysguard.faces 1.0 as Faces

Kirigami.FormLayout {
    id: root

    property alias cfg_showLegend: showSensorsLegendCheckbox.checked
    property alias cfg_fromAngle: fromAngleSpin.value
    property alias cfg_toAngle: toAngleSpin.value
    property alias cfg_smoothEnds: smoothEndsCheckbox.checked
    property alias cfg_rangeAuto: rangeAutoCheckbox.checked
    property alias cfg_rangeFrom: rangeFromSpin.value
    property alias cfg_rangeTo: rangeToSpin.value

    Controls.CheckBox {
        id: showSensorsLegendCheckbox
        text: i18n("Show Sensors Legend")
    }
    Controls.SpinBox {
        id: fromAngleSpin
        Kirigami.FormData.label: i18n("Start from Angle")
        from: -180
        to: 180
        editable: true
        textFromValue: function(value, locale) {
            return i18nc("angle degrees", "%1°", value + 180);
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text.replace(i18nc("angle degrees", "°"), "")) - 180;
        }
    }
    Controls.SpinBox {
        id: toAngleSpin
        Kirigami.FormData.label: i18n("Total Pie Angle")
        from: 0
        to: 360
        editable: true
        textFromValue: function(value, locale) {
            return i18nc("angle", "%1°", value);
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text.replace(i18nc("angle degrees", "°"), ""));
        }
    }
    Controls.CheckBox {
        id: smoothEndsCheckbox
        text: i18n("Rounded Lines")
    }

    Controls.CheckBox {
        id: rangeAutoCheckbox
        text: i18n("Automatic Data Range")
    }
    Controls.SpinBox {
        id: rangeFromSpin
        editable: true
        Kirigami.FormData.label: i18n("From:")
        enabled: !rangeAutoCheckbox.checked
    }
    Controls.SpinBox {
        id: rangeToSpin
        to: 99999
        editable: true
        Kirigami.FormData.label: i18n("To:")
        enabled: !rangeAutoCheckbox.checked
    }
}

