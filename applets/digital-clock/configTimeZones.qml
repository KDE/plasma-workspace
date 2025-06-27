/*
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.plasma.private.digitalclock
import org.kde.kirigami as Kirigami
import org.kde.config as KConfig
import org.kde.kcmutils as KCMUtils
import org.kde.plasma.workspace.timezoneselector as TimeZone

Kirigami.PageRow {
    id: timeZonesRow

    property string title
    property string cfg_lastSelectedTimezone
    property alias cfg_selectedTimeZones: timeZones.selectedTimeZones
    property alias cfg_wheelChangesTimezone: enableWheelCheckBox.checked

    defaultColumnWidth: timeZonesRow.width
    globalToolBar.style: Kirigami.ApplicationHeaderStyle.Auto

    /*Component.onCompleted: {
        applicationWindow().footer.visible = Qt.binding(function() {
            return timeZonesRow.currentIndex !== 1 || !timeZonesRow.visible
        })
    }*/

    Component.onCompleted: {
        timeZonesRow.realFooter = applicationWindow().footer
    }

    onVisibleChanged: {
        if (!visible && timeZonesRow.fakeFooter.parent) {
            applicationWindow().footer = timeZonesRow.realFooter
            timeZonesRow.fakeFooter.parent = null
        }
    }

    onCurrentIndexChanged: {
        if (currentIndex == 1) {
            applicationWindow().footer = timeZonesRow.fakeFooter
            timeZonesRow.realFooter.parent = null
        } else {
            applicationWindow().footer = timeZonesRow.realFooter
            timeZonesRow.fakeFooter.parent = null
        }
    }

    property Item realFooter
    property Item fakeFooter: QQC2.DialogButtonBox {
        background: Item {
            Kirigami.Separator {
                id: bottomSeparator
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                }
            }
        }
        QQC2.Button {
            text: i18n("Cancel")
            onClicked: {
                timeZonesRow.currentIndex = 0
                timeZoneSelector.selectedTimeZone = ""
            }
        }
        QQC2.Button {
            text: i18n("Add Selected Time Zone")
            icon.name: "list-add"
            enabled: timeZoneSelector.selectedTimeZone
            onClicked: {
                timeZones.selectedTimeZones = [...timeZones.selectedTimeZones, timeZoneSelector.selectedTimeZone]
                timeZoneSelector.selectedTimeZone = ""
                timeZonesRow.currentIndex = 0
            }
        }
    }

    initialPage: KCMUtils.ScrollViewKCM {

        title: timeZonesRow.title

        actions: [
            Kirigami.Action {
                text: i18n("Add Time Zone…")
                icon.name: "list-add-symbolic"
                Accessible.name: text // https://bugreports.qt.io/browse/QTBUG-130360
                onTriggered: {
                    if (timeZonesRow.depth == 1) {
                        timeZonesRow.push(timeZonesRow.addTimeZonePage)
                    } else {
                        timeZonesRow.currentIndex = 1
                    }
                }
            }
        ]

        TimeZoneModel {
            id: timeZones

            onSelectedTimeZonesChanged: {
                if (selectedTimeZones.length === 0) {
                    // Don't let the user remove all time zones
                    messageWidget.visible = true;
                    timeZones.selectLocalTimeZone();
                }
            }
        }

        view: ListView {
            id: configuredTimeZoneList
            clip: true // Avoid visual glitches
            focus: true // keyboard navigation
            activeFocusOnTab: true // keyboard navigation

            model: TimeZoneFilterProxy {
                sourceModel: timeZones
                onlyShowChecked: true
            }
            // We have no concept of selection in this list, so don't pre-select
            // the first item
            currentIndex: -1

            delegate: Kirigami.RadioSubtitleDelegate {
                id: timeZoneListItem

                required property int index // indirectly required by useAlternateBackgroundColor
                required property var model

                readonly property bool isCurrent: timeZonesRow.cfg_lastSelectedTimezone === model.timeZoneId
                readonly property bool isIdenticalToLocal: !model.isLocalTimeZone && model.city === timeZones.localTimeZoneCity()

                width: ListView.view.width

                font.bold: isCurrent

                // Stripes help the eye line up the text on the left and the button on the right
                Kirigami.Theme.useAlternateBackgroundColor: true

                text: model.city
                subtitle: {
                    if (configuredTimeZoneList.count > 1) {
                        if (isCurrent) {
                            return i18n("Clock is currently using this time zone");
                        } else if (isIdenticalToLocal) {
                            return i18nc("@label This list item shows a time zone city name that is identical to the local time zone's city, and will be hidden in the time zone display in the plasmoid's popup", "Hidden while this is the local time zone's city");
                        }
                    }
                    return "";
                }

                checked: isCurrent

                onToggled: {
                    if (checked) {
                        timeZonesRow.cfg_lastSelectedTimezone = model.timeZoneId;
                    }
                }

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.TitleSubtitle {
                        Layout.fillWidth: true

                        opacity: timeZoneListItem.isIdenticalToLocal ? 0.75 : 1.0

                        title: timeZoneListItem.text
                        subtitle: timeZoneListItem.subtitle

                        reserveSpaceForSubtitle: true
                    }

                    QQC2.Button {
                        visible: timeZoneListItem.model.isLocalTimeZone && KConfig.KAuthorized.authorizeControlModule("kcm_clock.desktop")
                        text: i18n("Switch Systemwide Time Zone…")
                        icon.name: "preferences-system-time"
                        font.bold: false
                        onClicked: KCMUtils.KCMLauncher.openSystemSettings("kcm_clock")
                    }

                    QQC2.Button {
                        visible: !timeZoneListItem.model.isLocalTimeZone && configuredTimeZoneList.count > 1
                        icon.name: "edit-delete-remove"
                        font.bold: false
                        onClicked: timeZoneListItem.model.checked = false;
                        QQC2.ToolTip {
                            text: i18n("Remove this time zone")
                        }
                    }
                }
            }

            section {
                property: "isLocalTimeZone"
                delegate: Kirigami.ListSectionHeader {
                    required property string section

                    width: configuredTimeZoneList.width
                    label: section === "true" ? i18n("Systemwide Time Zone") : i18n("Additional Time Zones")
                }
            }

            Kirigami.PlaceholderMessage {
                visible: configuredTimeZoneList.count === 1
                anchors {
                    top: parent.verticalCenter // Visual offset for system time zone and header
                    left: parent.left
                    right: parent.right
                    leftMargin: Kirigami.Units.largeSpacing * 6
                    rightMargin: Kirigami.Units.largeSpacing * 6
                }
                text: i18n("Add more time zones to display all of them in the applet's pop-up, or use one of them for the clock itself")
            }
        }

        // Re-add separator line between footer and list view
        extraFooterTopPadding: true
        footer: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.fillWidth: true
                leftPadding: Application.layoutDirection === Qt.LeftToRight ? enableWheelCheckBox.spacing : Kirigami.Units.largeSpacing * 2
                rightPadding: Application.layoutDirection === Qt.LeftToRight ? Kirigami.Units.largeSpacing * 2 : enableWheelCheckBox.spacing
                text: i18nc("@info:usagetip shown below listview", "Tip: Add your home time zone to this list to see the time there even when you're traveling. It will not be shown twice while at home.")
                font: Kirigami.Theme.smallFont
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
            QQC2.CheckBox {
                id: enableWheelCheckBox
                enabled: configuredTimeZoneList.count > 1
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.largeSpacing
                text: i18n("Switch displayed time zone by scrolling over clock applet")
            }

            QQC2.Label {
                enabled: enableWheelCheckBox.enabled
                color: enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                Layout.fillWidth: true
                Layout.leftMargin: enableWheelCheckBox.indicator.width + enableWheelCheckBox.spacing
                Layout.rightMargin: Kirigami.Units.largeSpacing * 2
                text: i18n("Using this feature does not change the systemwide time zone. When you travel, switch the systemwide time zone instead.")
                textFormat: Text.PlainText
                font: Kirigami.Theme.smallFont
                wrapMode: Text.Wrap
            }
        }
    }

    property Item addTimeZonePage: Kirigami.Page {
        padding: 0
        title: i18n("Choose Time Zone")

        Layout.fillHeight: true
        Layout.fillWidth: true

        TimeZone.TimezoneSelector {
            id: timeZoneSelector
            anchors.fill: parent
        }
    }

}
