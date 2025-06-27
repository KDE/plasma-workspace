/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Qt.labs.platform as Platform

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.config as KConfig
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami

KCMUtils.SimpleKCM {
    id: appearancePage
    property alias cfg_autoFontAndSize: autoFontAndSizeRadioButton.checked

    // boldText and fontStyleName are not used in DigitalClock.qml
    // However, they are necessary to remember the exact font style chosen.
    // Otherwise, when the user open the font dialog again, the style will be lost.
    property alias cfg_fontFamily: fontDialog.fontChosen.family
    property alias cfg_boldText: fontDialog.fontChosen.bold
    property alias cfg_italicText: fontDialog.fontChosen.italic
    property alias cfg_fontWeight: fontDialog.fontChosen.weight
    property alias cfg_fontStyleName: fontDialog.fontChosen.styleName
    property alias cfg_fontSize: fontDialog.fontChosen.pointSize

    property string cfg_timeFormat: ""
    property alias cfg_showLocalTimezone: showLocalTimeZone.checked
    property alias cfg_displayTimezoneFormat: displayTimeZoneFormat.currentIndex
    property alias cfg_showSeconds: showSecondsComboBox.currentIndex

    property alias cfg_showDate: showDate.checked
    property string cfg_dateFormat: "shortDate"
    property alias cfg_customDateFormat: customDateFormat.text
    property alias cfg_use24hFormat: use24hFormat.currentIndex
    property alias cfg_dateDisplayFormat: dateDisplayFormat.currentIndex

    property real comboBoxWidth: Math.max(dateDisplayFormat.implicitWidth,
                                          showSecondsComboBox.implicitWidth,
                                          displayTimeZoneFormat.implicitWidth,
                                          use24hFormat.implicitWidth,
                                          dateFormat.implicitWidth)


    Kirigami.FormLayout {

        RowLayout {
            Kirigami.FormData.label: i18n("Information:")
            spacing: Kirigami.Units.smallSpacing

            QQC2.CheckBox {
                id: showDate
                text: i18n("Show date")
            }

            QQC2.ComboBox {
                id: dateDisplayFormat
                enabled: showDate.checked
                visible: Plasmoid.formFactor !== PlasmaCore.Types.Vertical
                Layout.preferredWidth: appearancePage.comboBoxWidth
                model: [
                    i18n("Adaptive location"),
                    i18n("Always beside time"),
                    i18n("Always below time"),
                ]
                onActivated: cfg_dateDisplayFormat = currentIndex
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        QQC2.ComboBox {
            id: showSecondsComboBox
            Layout.preferredWidth: appearancePage.comboBoxWidth
            Kirigami.FormData.label: i18n("Show seconds:")
            model: [
                i18nc("@option:check", "Never"),
                i18nc("@option:check", "Only in the tooltip"),
                i18n("Always"),
            ]
            onActivated: cfg_showSeconds = currentIndex;
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        ColumnLayout {
            Kirigami.FormData.label: i18n("Show time zone:")
            Kirigami.FormData.buddyFor: showLocalTimeZoneWhenDifferent
            spacing: Kirigami.Units.smallSpacing

            QQC2.RadioButton {
                id: showLocalTimeZoneWhenDifferent
                text: i18n("Only when different from local time zone")
            }

            QQC2.RadioButton {
                id: showLocalTimeZone
                text: i18n("Always")
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Display time zone as:")
            Kirigami.FormData.buddyFor: displayTimeZoneFormat
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.ComboBox {
                id: displayTimeZoneFormat

                Layout.preferredWidth: appearancePage.comboBoxWidth
                model: [
                    i18n("Code"),
                    i18n("City"),
                    i18n("Offset from UTC time"),
                ]
                onActivated: cfg_displayTimezoneFormat = currentIndex
            }
            QQC2.Button {
                id: switchTimeZoneButton
                Layout.preferredWidth: Math.max(changeRegionalSettingsButton.implicitWidth, switchTimeZoneButton.implicitWidth, dateExampleLabel.implicitWidth)
                visible: KConfig.KAuthorized.authorizeControlModule("kcm_clock")
                text: i18nc("@action:button opens kcm", "Switch Time Zone…")
                icon.name: "preferences-system-time"
                onClicked: KCMUtils.KCMLauncher.openSystemSettings("kcm_clock")
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        RowLayout {
            Layout.fillWidth: true
            Kirigami.FormData.label: i18nc("@label:listbox", "Time display:")
            spacing: Kirigami.Units.smallSpacing

            QQC2.ComboBox {
                id: use24hFormat
                Layout.preferredWidth: appearancePage.comboBoxWidth
                model: [
                    i18nc("@item:inlistbox time display option", "12-Hour"),
                    i18nc("@item:inlistbox time display option", "Use region defaults"),
                    i18nc("@item:inlistbox time display option", "24-Hour")
                ]
                onActivated: cfg_use24hFormat = currentIndex
            }

            QQC2.Button {
                id: changeRegionalSettingsButton
                visible: KConfig.KAuthorized.authorizeControlModule("kcm_regionandlang")
                Layout.preferredWidth: Math.max(changeRegionalSettingsButton.implicitWidth, switchTimeZoneButton.implicitWidth, dateExampleLabel.implicitWidth)
                text: i18nc("@action:button opens kcm", "Change Regional Settings…")
                icon.name: "preferences-desktop-locale"
                onClicked: KCMUtils.KCMLauncher.openSystemSettings("kcm_regionandlang")
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        RowLayout {
            Kirigami.FormData.label: i18nc("@label:listbox", "Date format:")
            enabled: showDate.checked
            spacing: Kirigami.Units.smallSpacing

            QQC2.ComboBox {
                id: dateFormat
                Layout.preferredWidth: appearancePage.comboBoxWidth
                textRole: "label"
                model: [
                    {
                        label: i18nc("@item:inlistbox date display option, includes e.g. day of week and month as word", "Long date"),
                        name: "longDate",
                        formatter(d) {
                            return Qt.formatDate(d, Qt.locale(), Locale.LongFormat);
                        },
                    },
                    {
                        label: i18nc("@item:inlistbox date display option, e.g. all numeric", "Short date"),
                        name: "shortDate",
                        formatter(d) {
                            return Qt.formatDate(d, Qt.locale(), Locale.ShortFormat);
                        },
                    },
                    {
                        label: i18nc("@item:inlistbox date display option, yyyy-mm-dd", "ISO date"),
                        name: "isoDate",
                        formatter(d) {
                            return Qt.formatDate(d, Qt.ISODate);
                        },
                    },
                    {
                        label: i18nc("@item:inlistbox custom date format", "Custom"),
                        name: "custom",
                        formatter(d) {
                            return Qt.locale().toString(d, customDateFormat.text);
                        },
                    },
                ]
                onActivated: cfg_dateFormat = model[currentIndex]["name"];

                Component.onCompleted: {
                    const isConfiguredDateFormat = item => item["name"] === Plasmoid.configuration.dateFormat;
                    currentIndex = model.findIndex(isConfiguredDateFormat);
                }
            }

            QQC2.Label {
                id: dateExampleLabel
                Layout.preferredWidth: Math.max(changeRegionalSettingsButton.implicitWidth, switchTimeZoneButton.implicitWidth, dateExampleLabel.implicitWidth)
                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.PlainText
                text: dateFormat.model[dateFormat.currentIndex].formatter(new Date());
            }
        }

        QQC2.TextField {
            id: customDateFormat
            Layout.fillWidth: true
            enabled: showDate.checked
            visible: cfg_dateFormat === "custom"
        }

        QQC2.Label {
            text: i18n("<a href=\"https://doc.qt.io/qt-6/qml-qtqml-qt.html#formatDateTime-method\">Time Format Documentation</a>")
            enabled: showDate.checked
            visible: cfg_dateFormat === "custom"
            wrapMode: Text.Wrap

            Layout.preferredWidth: Layout.maximumWidth
            Layout.maximumWidth: Kirigami.Units.gridUnit * 16

            HoverHandler {
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : undefined
            }

            onLinkActivated: link => Qt.openUrlExternally(link)
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        QQC2.ButtonGroup {
            buttons: [autoFontAndSizeRadioButton, manualFontAndSizeRadioButton]
        }

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing
            Kirigami.FormData.label: i18nc("@label:group", "Text display:")
            Kirigami.FormData.buddyFor: autoFontAndSizeRadioButton

            QQC2.RadioButton {
                id: autoFontAndSizeRadioButton
                text: i18nc("@option:radio", "Automatic")
            }

            QQC2.Label {
                text: i18nc("@label", "Text will follow the system font and expand to fill the available space.")
                Layout.leftMargin: autoFontAndSizeRadioButton.indicator.width + autoFontAndSizeRadioButton.spacing
                textFormat: Text.PlainText
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font: Kirigami.Theme.smallFont
            }
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.RadioButton {
                id: manualFontAndSizeRadioButton
                text: i18nc("@option:radio setting for manually configuring the font settings", "Manual")
                checked: !cfg_autoFontAndSize
                onClicked: {
                    if (cfg_fontFamily === "") {
                        fontDialog.fontChosen = Kirigami.Theme.defaultFont
                    }
                }
            }

            QQC2.Button {
                text: i18nc("@action:button", "Choose Style…")
                icon.name: "settings-configure"
                enabled: manualFontAndSizeRadioButton.checked
                onClicked: {
                    fontDialog.currentFont = fontDialog.fontChosen
                    fontDialog.open()
                }
            }

        }

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                visible: manualFontAndSizeRadioButton.checked
                Layout.leftMargin: manualFontAndSizeRadioButton.indicator.width + manualFontAndSizeRadioButton.spacing
                text: i18nc("@info %1 is the font size, %2 is the font family", "%1pt %2", cfg_fontSize, fontDialog.fontChosen.family)
                textFormat: Text.PlainText
                font: fontDialog.fontChosen
            }
            QQC2.Label {
                visible: manualFontAndSizeRadioButton.checked
                Layout.leftMargin: manualFontAndSizeRadioButton.indicator.width + manualFontAndSizeRadioButton.spacing
                text: i18nc("@info", "Note: size may be reduced if the panel is not thick enough.")
                textFormat: Text.PlainText
                font: Kirigami.Theme.smallFont
            }
        }
    }

    // Use the Qt.Labs font dialog so it looks okay, or else we get the half-baked
    // QML version shipped in Qt 6, which doesn't look good.
    // Port back to the standard QtDialogs version when one of the following happens:
    // Qt's QML font dialog implementation looks better
    // We override the default dialog with our own in plasma-integration
    Platform.FontDialog {
        id: fontDialog
        title: i18nc("@title:window", "Choose a Font")
        modality: Qt.WindowModal
        parentWindow: appearancePage.Window.window

        property font fontChosen: null

        onAccepted: {
            fontChosen = font
        }
    }

    Component.onCompleted: {
        if (!Plasmoid.configuration.showLocalTimeZone) {
            showLocalTimeZoneWhenDifferent.checked = true;
        }
    }
}
