/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Controls 2.15 as QQC2

import org.kde.kquickcontrols 2.0 as KQuickControls
import org.kde.kirigami 2.5 as Kirigami

Kirigami.FormLayout {
    id: root
    twinFormLayouts: parentLayout

    property alias cfg_Color: colorButton.color
    property alias cfg_UseRandomColors: randomCheckBox.checked
    property int cfg_SlideInterval: 0
    property int hoursIntervalValue: Math.floor(cfg_SlideInterval / 3600)
    property int minutesIntervalValue: Math.floor(cfg_SlideInterval % 3600) / 60
    property int secondsIntervalValue: cfg_SlideInterval % 3600 % 60

    property alias formLayout: root

    Row {
        Kirigami.FormData.label: i18nd("plasma_wallpaper_org.kde.color", "Color:")
        spacing: Kirigami.Units.smallSpacing

        KQuickControls.ColorButton {
            id: colorButton
            enabled: !randomCheckBox.checked
            dialogTitle: i18nd("plasma_wallpaper_org.kde.color", "Select Background Color")
        }

        QQC2.CheckBox {
            id: randomCheckBox
            anchors.verticalCenter: colorButton.verticalCenter
            text: i18nc("@option:check", "Use random colors")
        }
    }

    Row {
        Kirigami.FormData.label: i18nc("@label:spinbox slideshow change interval", "Change every:")
        enabled: randomCheckBox.checked
        spacing: Kirigami.Units.smallSpacing

        Connections {
            target: root

            function onHoursIntervalValueChanged() {
                hoursInterval.value = root.hoursIntervalValue;
            }

            function onMinutesIntervalValueChanged() {
                minutesInterval.value = root.minutesIntervalValue;
            }

            function onSecondsIntervalValueChanged() {
                secondsInterval.value = root.secondsIntervalValue;
            }
        }

        QQC2.SpinBox {
            id: hoursInterval
            value: root.hoursIntervalValue
            from: 0
            to: 24
            editable: true
            onValueChanged: root.cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value

            textFromValue: (value, locale) => i18np("%1 hour", "%1 hours", value)
            valueFromText: (text, locale) => parseInt(text)
        }

        QQC2.SpinBox {
            id: minutesInterval
            value: root.minutesIntervalValue
            from: 0
            to: 60
            editable: true
            onValueChanged: root.cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value

            textFromValue: (value, locale) => i18np("%1 minute", "%1 minutes", value)
            valueFromText: (text, locale) => parseInt(text)
        }

        QQC2.SpinBox {
            id: secondsInterval
            value: root.secondsIntervalValue
            from: root.hoursIntervalValue === 0 && root.minutesIntervalValue === 0 ? 1 : 0
            to: 60
            editable: true
            onValueChanged: root.cfg_SlideInterval = hoursInterval.value * 3600 + minutesInterval.value * 60 + secondsInterval.value

            textFromValue: (value, locale) => i18np("%1 second", "%1 seconds", value)
            valueFromText: (text, locale) => parseInt(text)
        }
    }
}
