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


    Kirigami.Form {
        Kirigami.FormGroup {
            Kirigami.FormEntry {
                contentItem: RowLayout {
                    Kirigami.FormData.label: i18nc("@option:check also @label:listbox", "Show date:")
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.CheckBox {
                        id: showDate
                        text: !dateDisplayFormat.visible ? i18nc("@option:check completes Show date …", "Below time") : ""
                    }

                    QQC2.ComboBox {
                        id: dateDisplayFormat
                        enabled: showDate.checked
                        visible: Plasmoid.formFactor !== PlasmaCore.Types.Vertical
                        Layout.preferredWidth: appearancePage.comboBoxWidth
                        model: [
                            i18nc("@item:inlistbox for Show date", "Adaptive location"),
                            i18nc("@item:inlistbox for Show date", "Always beside time"),
                            i18nc("@item:inlistbox for Show date", "Always below time"),
                        ]
                        onActivated: appearancePage.cfg_dateDisplayFormat = currentIndex
                    }
                }
            }
            Kirigami.FormEntry {
                contentItem: QQC2.ComboBox {
                    id: showSecondsComboBox
                    Layout.preferredWidth: appearancePage.comboBoxWidth
                    Kirigami.FormData.label: i18n("Show seconds:")
                    model: [
                        i18nc("@item:inlistbox Show Seconds", "Never"),
                        i18nc("@item:inlistbox Show Seconds", "Only in the tooltip"),
                        i18nc("@item:inlistbox Show Seconds", "Always"),
                    ]
                    onActivated: appearancePage.cfg_showSeconds = currentIndex;
                }
            }
            QQC2.ButtonGroup { id: timeZoneButtonGroup }
            Kirigami.FormEntry {
                title: i18nc("@title:group prefix for group of radio buttons, e.g. Always", "Show time zone:")
                contentItem: QQC2.RadioButton {
                    id: showLocalTimeZoneWhenDifferent
                    QQC2.ButtonGroup.group: timeZoneButtonGroup
                    text: i18nc("@option:radio Show time zone", "Only when different from local time zone")
                    checked: !showLocalTimeZone.checked
                }
            }
            Kirigami.FormEntry {
                contentItem: QQC2.RadioButton {
                    QQC2.ButtonGroup.group: timeZoneButtonGroup
                    id: showLocalTimeZone
                    text: i18nc("@option:radio Show time zone", "Always")
                }
            }
        }

        Kirigami.FormGroup {
            Kirigami.FormEntry {
                title: i18nc("@label:listbox e.g. City", "Display time zone as:")
                contentItem: QQC2.ComboBox {
                    id: displayTimeZoneFormat

                    Layout.preferredWidth: appearancePage.comboBoxWidth
                    model: [
                        i18nc("@item:inlistbox Display time zone as", "Code"),
                        i18nc("@item:inlistbox Display time zone as", "City"),
                        i18nc("@item:inlistbox Display time zone as", "Offset from UTC time"),
                    ]
                    onActivated: appearancePage.cfg_displayTimezoneFormat = currentIndex
                }
                trailingItems: [
                    QQC2.Button {
                        id: switchTimeZoneButton
                        Layout.preferredWidth: Math.max(changeRegionalSettingsButton.implicitWidth, switchTimeZoneButton.implicitWidth)
                        visible: KConfig.KAuthorized.authorizeControlModule("kcm_clock")
                        text: i18nc("@action:button opens kcm", "Switch Time Zone…")
                        icon.name: "preferences-system-time"
                        onClicked: KCMUtils.KCMLauncher.openSystemSettings("kcm_clock")
                    }
                ]
            }

            Kirigami.FormEntry {
                title: i18nc("@label:listbox", "Time display:")
                contentItem: QQC2.ComboBox {
                    id: use24hFormat
                    Layout.preferredWidth: appearancePage.comboBoxWidth
                    model: [
                        i18nc("@item:inlistbox time display option", "12-Hour"),
                        i18nc("@item:inlistbox time display option", "Use region defaults"),
                        i18nc("@item:inlistbox time display option", "24-Hour")
                    ]
                    onActivated: appearancePage.cfg_use24hFormat = currentIndex
                }
                trailingItems: [
                    QQC2.Button {
                        id: changeRegionalSettingsButton
                        visible: KConfig.KAuthorized.authorizeControlModule("kcm_regionandlang")
                        Layout.preferredWidth: Math.max(changeRegionalSettingsButton.implicitWidth, switchTimeZoneButton.implicitWidth)
                        text: i18nc("@action:button opens kcm", "Change Regional Settings…")
                        icon.name: "preferences-desktop-locale"
                        onClicked: KCMUtils.KCMLauncher.openSystemSettings("kcm_regionandlang")
                    }
                ]
            }

            Kirigami.FormEntry {
                title: i18nc("@label:listbox", "Date format:")
                enabled: appearancePage.cfg_showDate
                contentItem: QQC2.ComboBox {
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
                    onActivated: appearancePage.cfg_dateFormat = model[currentIndex]["name"];

                    Component.onCompleted: {
                        const isConfiguredDateFormat = item => item["name"] === Plasmoid.configuration.dateFormat;
                        currentIndex = model.findIndex(isConfiguredDateFormat);
                    }
                }
                trailingItems: [
                    QQC2.Label {
                        id: dateExampleLabel
                        Layout.preferredWidth: Math.max(changeRegionalSettingsButton.implicitWidth, switchTimeZoneButton.implicitWidth)
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        textFormat: Text.PlainText
                        text: dateFormat.model[dateFormat.currentIndex].formatter(new Date());
                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.visible: (exampleHoverHandler.hovered || activeFocus) && implicitWidth > width
                        // no ToolTip.delay, this is an edge case and we might as well show it immediately if it applies

                        HoverHandler {
                            id: exampleHoverHandler
                        }
                    }
                ]
            }

            Kirigami.FormEntry {
                visible: appearancePage.cfg_dateFormat === "custom"
                enabled: appearancePage.cfg_showDate
                contentItem: QQC2.TextField {
                    id: customDateFormat
                    Layout.fillWidth: true
                    enabled: showDate.checked
                }
                subtitle: i18nc("@info:usagetip", "<a href=\"https://doc.qt.io/qt-6/qml-qtqml-qt.html#formatDateTime-method\">Time Format Documentation</a>")
            }
        }

        Kirigami.FormGroup {
            QQC2.ButtonGroup {
                buttons: [autoFontAndSizeRadioButton, manualFontAndSizeRadioButton]
            }

            Kirigami.FormEntry {
                title: i18nc("@label:group", "Text display:")
                contentItem: QQC2.RadioButton {
                    id: autoFontAndSizeRadioButton
                    text: i18nc("@option:radio", "Automatic")
                }
                subtitle: i18nc("@label", "Text will follow the system font and expand to fill the available space.")
            }

            Kirigami.FormEntry {
                contentItem: QQC2.RadioButton {
                    id: manualFontAndSizeRadioButton
                    text: i18nc("@option:radio setting for manually configuring the font settings", "Manual")
                    checked: !appearancePage.cfg_autoFontAndSize
                    onClicked: {
                        if (appearancePage.cfg_fontFamily === "") {
                            fontDialog.fontChosen = Kirigami.Theme.defaultFont
                        }
                    }
                }
                trailingItems: [
                    QQC2.Button {
                        text: i18nc("@action:button", "Choose Style…")
                        icon.name: "settings-configure"
                        enabled: manualFontAndSizeRadioButton.checked
                        onClicked: {
                            fontDialog.open()
                            fontDialog.currentFont = fontDialog.fontChosen
                        }
                    }
                ]
                subtitle: manualFontAndSizeRadioButton.checked ? i18nc("@info", "Note: size may be reduced if the panel is not thick enough.") : ""
            }
            Kirigami.FormEntry {
                contentItem: QQC2.Label {
                    visible: manualFontAndSizeRadioButton.checked
                    leftPadding: manualFontAndSizeRadioButton.indicator.width + manualFontAndSizeRadioButton.spacing
                    text: i18nc("@info %1 is the font size, %2 is the font family", "%1pt %2", appearancePage.cfg_fontSize, fontDialog.fontChosen.family)
                    textFormat: Text.PlainText
                    font: fontDialog.fontChosen
                }
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
}
