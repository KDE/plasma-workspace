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

KCMUtils.ScrollViewKCM {
    id: timeZonesPage

    property string cfg_lastSelectedTimezone
    property alias cfg_selectedTimeZones: timeZones.selectedTimeZones
    property alias cfg_wheelChangesTimezone: enableWheelCheckBox.checked

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

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("Tip: if you travel frequently, add your home time zone to this list. It will only appear when you change the systemwide time zone to something else.")
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }
    }

    view: ListView {
        id: configuredTimeZoneList
        clip: true // Avoid visual glitches
        focus: true // keyboard navigation
        activeFocusOnTab: true // keyboard navigation

        headerPositioning: ListView.OverlayHeader
        header: Kirigami.InlineViewHeader {
            width: ListView.view.width
            actions: [
                Kirigami.Action {
                    text: i18n("Add Time Zones…")
                    icon.name: "list-add-symbolic"
                    onTriggered: timeZoneSheet.open()
                }
            ]
        }
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

            readonly property bool isCurrent: timeZonesPage.cfg_lastSelectedTimezone === model.timeZoneId
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
                    timeZonesPage.cfg_lastSelectedTimezone = model.timeZoneId;
                }
            }

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.TitleSubtitle {
                    Layout.fillWidth: true

                    opacity: timeZoneListItem.isIdenticalToLocal ? 0.6 : 1.0

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
                    icon.name: "edit-delete"
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

    footer: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.CheckBox {
            id: enableWheelCheckBox
            enabled: configuredTimeZoneList.count > 1
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            text: i18n("Switch displayed time zone by scrolling over clock applet")
        }

        QQC2.Label {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing * 2
            Layout.rightMargin: Kirigami.Units.largeSpacing * 2
            text: i18n("Using this feature does not change the systemwide time zone. When you travel, switch the systemwide time zone instead.")
            textFormat: Text.PlainText
            font: Kirigami.Theme.smallFont
            wrapMode: Text.Wrap
        }
    }

    Kirigami.OverlaySheet {
        id: timeZoneSheet

        parent: timeZonesPage.QQC2.Overlay.overlay

        onVisibleChanged: {
            filter.text = "";
            messageWidget.visible = false;
            if (visible) {
                filter.forceActiveFocus()
            }
        }

        header: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                Layout.fillWidth: true
                text: i18n("Add More Time Zones")
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
            Kirigami.SearchField {
                id: filter
                Layout.fillWidth: true
            }
            Kirigami.InlineMessage {
                id: messageWidget
                Layout.fillWidth: true
                type: Kirigami.MessageType.Warning
                text: i18n("At least one time zone needs to be enabled. Your local time zone was enabled automatically.")
                showCloseButton: true
            }
        }

        footer: QQC2.DialogButtonBox {
            standardButtons: QQC2.DialogButtonBox.Ok
            onAccepted: timeZoneSheet.close()
        }

        ListView {
            focus: true // keyboard navigation
            activeFocusOnTab: true // keyboard navigation
            clip: true
            implicitWidth: Math.max(timeZonesPage.width/2, Kirigami.Units.gridUnit * 25)

            model: TimeZoneFilterProxy {
                sourceModel: timeZones
                filterString: filter.text
            }

            delegate: QQC2.CheckDelegate {
                required property int index
                required property var model

                required checked
                required property string city
                required property string comment
                required property string region

                width: ListView.view.width
                focus: true // keyboard navigation
                text: {
                    if (!city || city.indexOf("UTC") === 0) {
                        return comment;
                    } else if (comment) {
                        return i18n("%1, %2 (%3)", city, region, comment);
                    } else {
                        return i18n("%1, %2", city, region)
                    }
                }

                onToggled: {
                    model.checked = checked

                    ListView.view.currentIndex = index // highlight
                    ListView.view.forceActiveFocus() // keyboard navigation
                }
                highlighted: ListView.isCurrentItem
            }
        }
    }
}
