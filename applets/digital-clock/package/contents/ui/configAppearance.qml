/*
    SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.0
import QtQuick.Controls 2.3 as QtControls
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.1 as QtDialogs
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.workspace.calendar 2.0 as PlasmaCalendar
import org.kde.kquickcontrolsaddons 2.0 // For KCMShell
import org.kde.kirigami 2.5 as Kirigami



ColumnLayout {
    id: appearancePage

    property alias cfg_autoFontAndSize: autoFontAndSizeRadioButton.checked

    // boldText and fontStyleName are not used in DigitalClock.qml
    // However, they are necessary to remember the exact font style chosen.
    // Otherwise, when the user open the font dialog again, the style will be lost.
    property alias cfg_fontFamily : fontDialog.fontChosen.family
    property alias cfg_boldText : fontDialog.fontChosen.bold
    property alias cfg_italicText : fontDialog.fontChosen.italic
    property alias cfg_fontWeight : fontDialog.fontChosen.weight
    property alias cfg_fontStyleName : fontDialog.fontChosen.styleName
    property alias cfg_fontSize : fontDialog.fontChosen.pointSize

    property string cfg_timeFormat: ""
    property alias cfg_showLocalTimezone: showLocalTimezone.checked
    property alias cfg_displayTimezoneFormat: displayTimezoneFormat.currentIndex
    property alias cfg_showSeconds: showSeconds.checked

    property alias cfg_showDate: showDate.checked
    property string cfg_dateFormat: "shortDate"
    property alias cfg_customDateFormat: customDateFormat.text
    property alias cfg_use24hFormat: use24hFormat.currentIndex
    property alias cfg_dateDisplayFormat: dateDisplayFormat.currentIndex

    Kirigami.FormLayout {
        Layout.fillWidth: true

        RowLayout {
            Kirigami.FormData.label: i18n("Information:")

            QtControls.CheckBox {
                id: showDate
                text: i18n("Show date")
            }

            QtControls.ComboBox {
                id: dateDisplayFormat
                enabled: showDate.checked
                visible: Plasmoid.formFactor !== PlasmaCore.Types.Vertical
                model: [
                    i18n("Adaptive location"),
                    i18n("Always beside time"),
                    i18n("Always below time"),
                ]
                onActivated: cfg_dateDisplayFormat = currentIndex
            }
        }

        QtControls.CheckBox {
            id: showSeconds
            text: i18n("Show seconds")
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        ColumnLayout {
            Kirigami.FormData.label: i18n("Show time zone:")
            Kirigami.FormData.buddyFor: showLocalTimeZoneWhenDifferent

            QtControls.RadioButton {
                id: showLocalTimeZoneWhenDifferent
                text: i18n("Only when different from local time zone")
            }

            QtControls.RadioButton {
                id: showLocalTimezone
                text: i18n("Always")
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Display time zone as:")

            QtControls.ComboBox {
                id: displayTimezoneFormat
                model: [
                    i18n("Code"),
                    i18n("City"),
                    i18n("Offset from UTC time"),
                ]
                onActivated: cfg_displayTimezoneFormat = currentIndex
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        RowLayout {
            Layout.fillWidth: true
            Kirigami.FormData.label: i18n("Time display:")

            QtControls.ComboBox {
                id: use24hFormat
                model: [
                    i18n("12-Hour"),
                    i18n("Use Region Defaults"),
                    i18n("24-Hour")
                ]
                onCurrentIndexChanged: cfg_use24hFormat = currentIndex
            }

            QtControls.Button {
                visible: KCMShell.authorize("kcm_regionandlang.desktop").length > 0
                text: i18n("Change Regional Settings…")
                icon.name: "preferences-desktop-locale"
                onClicked: KCMShell.openSystemSettings("kcm_regionandlang")
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Date format:")
            enabled: showDate.checked

            QtControls.ComboBox {
                id: dateFormat
                textRole: "label"
                model: [
                    {
                        'label': i18n("Long Date"),
                        'name': "longDate",
                        format: Qt.SystemLocaleLongDate
                    },
                    {
                        'label': i18n("Short Date"),
                        'name': "shortDate",
                        format: Qt.SystemLocaleShortDate
                    },
                    {
                        'label': i18n("ISO Date"),
                        'name': "isoDate",
                        format: Qt.ISODate
                    },
                    {
                        'label': i18nc("custom date format", "Custom"),
                        'name': "custom"
                    }
                ]
                onCurrentIndexChanged: cfg_dateFormat = model[currentIndex]["name"]

                Component.onCompleted: {
                    for (var i = 0; i < model.length; i++) {
                        if (model[i]["name"] === Plasmoid.configuration.dateFormat) {
                            dateFormat.currentIndex = i;
                        }
                    }
                }
            }

            QtControls.Label {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                text: Qt.formatDate(new Date(), cfg_dateFormat === "custom" ? customDateFormat.text
                                                                            : dateFormat.model[dateFormat.currentIndex].format)
            }
        }

        QtControls.TextField {
            id: customDateFormat
            Layout.fillWidth: true
            enabled: showDate.checked
            visible: cfg_dateFormat == "custom"
        }

        QtControls.Label {
            text: i18n("<a href=\"https://doc.qt.io/qt-5/qml-qtqml-qt.html#formatDateTime-method\">Time Format Documentation</a>")
            enabled: showDate.checked
            visible: cfg_dateFormat == "custom"
            wrapMode: Text.Wrap
            Layout.preferredWidth: Layout.maximumWidth
            Layout.maximumWidth: Kirigami.Units.gridUnit * 16

            onLinkActivated: Qt.openUrlExternally(link)
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton // We don't want to eat clicks on the Label
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        QtControls.ButtonGroup {
            buttons: [autoFontAndSizeRadioButton, manualFontAndSizeRadioButton]
        }

        QtControls.RadioButton {
            Kirigami.FormData.label: i18nc("@label:group", "Text display:")
            id: autoFontAndSizeRadioButton
            text: i18nc("@option:radio", "Automatic")
        }

        QtControls.Label {
            text: i18nc("@label", "Text will follow the system font and expand to fill the available space.")
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font: PlasmaCore.Theme.smallestFont
        }

        RowLayout {
            QtControls.RadioButton {
                id: manualFontAndSizeRadioButton
                text: i18nc("@option:radio setting for manually configuring the font settings", "Manual")
                checked: !cfg_autoFontAndSize
                onClicked: {
                    if (cfg_fontFamily === "") {
                        fontDialog.fontChosen = PlasmaCore.Theme.defaultFont
                    }
                }
            }

            QtControls.Button {
                text: i18nc("@action:button", "Choose Style…")
                icon.name: "settings-configure"
                enabled: manualFontAndSizeRadioButton.checked
                onClicked: {
                    fontDialog.font = fontDialog.fontChosen
                    fontDialog.open()
                }
            }

        }

        QtControls.Label {
            visible: manualFontAndSizeRadioButton.checked
            text: i18nc("@info %1 is the font size, %2 is the font family", "%1pt %2", cfg_fontSize, fontDialog.fontChosen.family)
            font: fontDialog.fontChosen
        }
    }

    Item {
        Layout.fillHeight: true
    }

    QtDialogs.FontDialog {
        id: fontDialog
        title: i18nc("@title:window", "Choose a Font")
        modality: Qt.WindowModal

        property font fontChosen: Qt.Font()

        onAccepted: {
            fontChosen = font
        }
    }

    Component.onCompleted: {
        if (!Plasmoid.configuration.showLocalTimezone) {
            showLocalTimeZoneWhenDifferent.checked = true;
        }
    }
}
