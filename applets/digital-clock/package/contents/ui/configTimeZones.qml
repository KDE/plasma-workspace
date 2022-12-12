/*
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.12
import QtQuick.Controls 2.8 as QQC2
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.1

import org.kde.kquickcontrolsaddons 2.0 // For kcmshell
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.private.digitalclock 1.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.14 as Kirigami

ColumnLayout {
    id: timeZonesPage

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

    QQC2.Label {
        Layout.fillWidth: true
        text: i18n("Tip: if you travel frequently, add another entry for your home time zone to this list. It will only appear when you change the systemwide time zone to something else.")
        wrapMode: Text.Wrap
    }

    QQC2.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        // Or else the page becomes scrollable when the list has a lot of items,
        // rather than the list becoming scrollable, which is what we want
        Layout.maximumHeight: timeZonesPage.parent.height - Kirigami.Units.gridUnit * 7

        Component.onCompleted: background.visible = true // enable border

        // HACK: Hide unnecesary horizontal scrollbar (https://bugreports.qt.io/browse/QTBUG-83890)
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

        ListView {
            id: configuredTimezoneList
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

            delegate: Kirigami.BasicListItem {
                id: timeZoneListItem
                readonly property bool isCurrent: Plasmoid.configuration.lastSelectedTimezone === model.timeZoneId
                readonly property bool isIdenticalToLocal: !model.isLocalTimeZone && model.city === timeZones.localTimeZoneCity()

                bold: isCurrent
                fadeContent: isIdenticalToLocal

                // Don't want a highlight effect here because it doesn't look good
                hoverEnabled: false
                activeBackgroundColor: "transparent"
                activeTextColor: Kirigami.Theme.textColor

                reserveSpaceForSubtitle: true
                // FIXME: this should have already evaluated to false because
                // the list item doesn't have an icon
                reserveSpaceForIcon: false

                // TODO: create Kirigami.MutuallyExclusiveListItem to be the
                // RadioButton equivalent of Kirigami.CheckableListItem,
                // and then port to use that in Plasma 5.22
                leading: QQC2.RadioButton {
                    id: radioButton
                    visible: configuredTimezoneList.count > 1
                    checked: timeZoneListItem.isCurrent
                    onToggled: clickAction.trigger()
                }

                label: model.city
                subtitle: {
                    if (configuredTimezoneList.count > 1) {
                        if (isCurrent) {
                            return i18n("Clock is currently using this time zone");
                        } else if (isIdenticalToLocal) {
                            return i18nc("@label This list item shows a time zone city name that is identical to the local time zone's city, and will be hidden in the timezone display in the plasmoid's popup", "Hidden while this is the local time zone's city");
                        }
                    }
                    return "";
                }

                action: Kirigami.Action {
                    id: clickAction
                    onTriggered: Plasmoid.configuration.lastSelectedTimezone = model.timeZoneId
                }

                trailing: RowLayout {
                    QQC2.Button {
                        visible: model.isLocalTimeZone && KCMShell.authorize("kcm_clock.desktop").length > 0
                        text: i18n("Switch Systemwide Time Zone…")
                        icon.name: "preferences-system-time"
                        onClicked: KCMShell.openSystemSettings("kcm_clock")
                    }
                    QQC2.Button {
                        visible: !model.isLocalTimeZone && configuredTimezoneList.count > 1
                        icon.name: "edit-delete"
                        onClicked: model.checked = false;
                        QQC2.ToolTip {
                            text: i18n("Remove this time zone")
                        }
                    }
                }
            }

            section {
                property: "isLocalTimeZone"
                delegate: Kirigami.ListSectionHeader {
                    label: section == "true" ? i18n("Systemwide Time Zone") : i18n("Additional Time Zones")
                }
            }

            Kirigami.PlaceholderMessage {
                visible: configuredTimezoneList.count === 1
                anchors {
                    top: parent.verticalCenter // Visual offset for system timezone and header
                    left: parent.left
                    right: parent.right
                    leftMargin: Kirigami.Units.largeSpacing * 6
                    rightMargin: Kirigami.Units.largeSpacing * 6
                }
                text: i18n("Add more time zones to display all of them in the applet's pop-up, or use one of them for the clock itself")
            }
        }
    }

    QQC2.Button {
        Layout.alignment: Qt.AlignLeft // Explicitly set so it gets reversed for LTR mode
        text: i18n("Add Time Zones…")
        icon.name: "list-add"
        onClicked: timezoneSheet.open()
    }

    QQC2.CheckBox {
        id: enableWheelCheckBox
        enabled: configuredTimezoneList.count > 1
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.largeSpacing
        text: i18n("Switch displayed time zone by scrolling over clock applet")
    }

    QQC2.Label {
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing * 2
        Layout.rightMargin: Kirigami.Units.largeSpacing * 2
        text: i18n("Using this feature does not change the systemwide time zone. When you travel, switch the systemwide time zone instead.")
        font: Kirigami.Theme.smallFont
        wrapMode: Text.Wrap
    }

    Item {
        // Tighten up the layout
        Layout.fillHeight: true
    }

    Kirigami.OverlaySheet {
        id: timezoneSheet

        onSheetOpenChanged: {
            filter.text = "";
            messageWidget.visible = false;
            if (sheetOpen) {
                filter.forceActiveFocus()
            }
        }

        // Need to manually set the parent when using this in a Plasma config dialog
        parent: timeZonesPage.parent

        header: ColumnLayout {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 25

            Kirigami.Heading {
                Layout.fillWidth: true
                text: i18n("Add More Timezones")
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
                text: i18n("At least one time zone needs to be enabled. Your local timezone was enabled automatically.")
                showCloseButton: true
            }
        }

        footer: QQC2.DialogButtonBox {
            standardButtons: QQC2.DialogButtonBox.Ok
            onAccepted: timezoneSheet.close()
        }

        ListView {
            id: listView
            focus: true // keyboard navigation
            activeFocusOnTab: true // keyboard navigation
            implicitWidth: Kirigami.Units.gridUnit * 25

            model: TimeZoneFilterProxy {
                sourceModel: timeZones
                filterString: filter.text
            }

            delegate: QQC2.CheckDelegate {
                id: checkbox
                width: listView.width
                focus: true // keyboard navigation
                text: !city || city.indexOf("UTC") === 0 ? comment : comment ? i18n("%1, %2 (%3)", city, region, comment) : i18n("%1, %2", city, region)
                checked: model.checked
                onToggled: {
                    model.checked = checkbox.checked
                    listView.currentIndex = index // highlight
                    listView.forceActiveFocus() // keyboard navigation
                }
                highlighted: ListView.isCurrentItem
            }
        }
    }
}
