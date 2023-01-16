/*
    SPDX-FileCopyrightText: 2022 Tanbir Jishan <tantalising007@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

// NOTE : This header is designed to be usable by both the generic calendar component and the digital clock applet
// which requires a little different layout to accomodate for configure and pin buttons because it may be in panel

//                      CALENDAR                                               DIGTAL CLOCK
// |---------------------------------------------------|  |----------------------------------------------------|
// | January                              < today >    |  | January                            config+  pin/   |
// |        Days          Months         Year          |  |   Days       Months       Year          < today >  |
// |                                                   |  |                                                    |
// |               Rest of the calendar                |  |              Rest of the calendar                  |
// |...................................................|  |....................................................|
//


Item {
    id: root

    required property var swipeView
    required property var monthViewRoot

    readonly property bool isDigitalClock: monthViewRoot.showDigitalClockHeader
    readonly property var buttons: isDigitalClock ? dateManipulationButtonsForDigitalClock : dateManipulationButtons
    readonly property var tabButton: isDigitalClock ? configureButton : todayButton
    readonly property var previousButton: buttons.previousButton
    readonly property var todayButton: buttons.todayButton
    readonly property var nextButton: buttons.nextButton
    readonly property alias tabBar: tabBar
    readonly property alias heading: heading
    readonly property alias configureButton: dateAndPinButtons.configureButton
    readonly property alias pinButton: dateAndPinButtons.pinButton

    implicitWidth: viewHeader.implicitWidth
    implicitHeight: viewHeader.implicitHeight

    KeyNavigation.up: configureButton

    Loader {
        anchors.fill: parent
        sourceComponent: PlasmaExtras.PlasmoidHeading {}
        active: isDigitalClock
    }

    ColumnLayout {
        id: viewHeader
        width: parent.width

        RowLayout {
            spacing: 0
            Layout.leftMargin: PlasmaCore.Units.smallSpacing * 2

            PlasmaExtras.Heading {
                id: heading
                // Needed for Appium testing
                objectName: "monthHeader"

                text: root.swipeView.currentIndex > 0 || monthViewRoot.selectedYear !== today.getFullYear() ? i18ndc("plasmashellprivateplugin", "Format: month year", "%1 %2", monthViewRoot.selectedMonth, monthViewRoot.selectedYear.toString()) : monthViewRoot.selectedMonth
                level: root.isDigitalClock ? 1 : 2
                elide: Text.ElideRight
                font.capitalization: Font.Capitalize
                Layout.fillWidth: true
            }

            Loader {
                id: dateManipulationButtons

                property var previousButton: item && item.previousButton
                property var todayButton: item && item.todayButton
                property var nextButton: item && item.nextButton

                sourceComponent: buttonsGroup
                active: !root.isDigitalClock
            }

            Loader {
                id: dateAndPinButtons

                readonly property var configureButton: item && item.configureButton
                readonly property var pinButton: item && item.pinButton

                sourceComponent: dateAndPin
                active: root.isDigitalClock
            }
        }

        RowLayout {
            spacing: 0
            PlasmaComponents3.TabBar {
                id: tabBar

                currentIndex: root.swipeView.currentIndex
                Layout.fillWidth: true
                Layout.bottomMargin: root.isDigitalClock ? 0 : PlasmaCore.Units.smallSpacing

                KeyNavigation.up: root.isDigitalClock ? root.configureButton : root.previousButton
                KeyNavigation.right: dateManipulationButtonsForDigitalClock.previousButton
                KeyNavigation.left:  root.monthViewRoot.eventButton && root.monthViewRoot.eventButton.visible ?
                                                                               root.monthViewRoot.eventButton :
                                                                               root.monthViewRoot.eventButton && root.monthViewRoot.eventButton.KeyNavigation.down

                PlasmaComponents3.TabButton {
                    Accessible.onPressAction: clicked()
                    text: i18nd("plasmashellprivateplugin", "Days");
                    onClicked: monthViewRoot.showMonthView();
                    display: PlasmaComponents3.AbstractButton.TextOnly
                }
                PlasmaComponents3.TabButton {
                    Accessible.onPressAction: clicked()
                    text: i18nd("plasmashellprivateplugin", "Months");
                    onClicked: monthViewRoot.showYearView();
                    display: PlasmaComponents3.AbstractButton.TextOnly
                }
                PlasmaComponents3.TabButton {
                    Accessible.onPressAction: clicked()
                    text: i18nd("plasmashellprivateplugin", "Years");
                    onClicked: monthViewRoot.showDecadeView();
                    display: PlasmaComponents3.AbstractButton.TextOnly
                }
            }

            Loader {
                id: dateManipulationButtonsForDigitalClock

                property var previousButton: item && item.previousButton
                property var todayButton: item && item.todayButton
                property var nextButton: item && item.nextButton

                sourceComponent: buttonsGroup
                active: root.isDigitalClock
            }
        }
    }

    // ------------------------------------------ UI ends ------------------------------------------------- //

    Component {
        id: buttonsGroup

        RowLayout {
            spacing: 0

            readonly property alias previousButton: previousButton
            readonly property alias todayButton: todayButton
            readonly property alias nextButton: nextButton

            KeyNavigation.up: root.configureButton

            PlasmaComponents3.ToolButton {
                id: previousButton
                text: {
                    switch(monthViewRoot.calendarViewDisplayed) {
                        case MonthView.CalendarView.DayView:
                            return i18nd("plasmashellprivateplugin", "Previous Month")
                        case MonthView.CalendarView.MonthView:
                            return i18nd("plasmashellprivateplugin", "Previous Year")
                        case MonthView.CalendarView.YearView:
                            return i18nd("plasmashellprivateplugin", "Previous Decade")
                        default:
                            return "";
                    }
                }

                icon.name: Qt.application.layoutDirection === Qt.RightToLeft ? "go-next" : "go-previous"
                display: PlasmaComponents3.AbstractButton.IconOnly
                KeyNavigation.right: todayButton

                onClicked: monthViewRoot.previousView()

                PlasmaComponents3.ToolTip { text: parent.text }
            }

            PlasmaComponents3.ToolButton {
                id: todayButton
                text: i18ndc("plasmashellprivateplugin", "Reset calendar to today", "Today")
                Accessible.description: i18nd("plasmashellprivateplugin", "Reset calendar to today")
                KeyNavigation.right: nextButton

                onClicked: monthViewRoot.resetToToday()
            }

            PlasmaComponents3.ToolButton {
                id: nextButton
                text: {
                    switch(monthViewRoot.calendarViewDisplayed) {
                        case MonthView.CalendarView.DayView:
                            return i18nd("plasmashellprivateplugin", "Next Month")
                        case MonthView.CalendarView.MonthView:
                            return i18nd("plasmashellprivateplugin", "Next Year")
                        case MonthView.CalendarView.YearView:
                            return i18nd("plasmashellprivateplugin", "Next Decade")
                        default:
                            return "";
                    }
                }

                icon.name: Qt.application.layoutDirection === Qt.RightToLeft ? "go-previous" : "go-next"
                display: PlasmaComponents3.AbstractButton.IconOnly
                KeyNavigation.tab: root.swipeView

                onClicked: monthViewRoot.nextView();

                PlasmaComponents3.ToolTip { text: parent.text }
            }
        }
    }

    Component {
        id: dateAndPin

        RowLayout {
            spacing: 0

            readonly property alias configureButton: configureButton
            readonly property alias pinButton: pinButton

            KeyNavigation.up: pinButton

            PlasmaComponents3.ToolButton {
                id: configureButton

                visible: root.monthViewRoot.digitalClock.action("configure").enabled

                display: PlasmaComponents3.AbstractButton.IconOnly
                icon.name: "configure"
                text: root.monthViewRoot.digitalClock.action("configure").text

                KeyNavigation.left: tabBar.KeyNavigation.left
                KeyNavigation.right: pinButton
                KeyNavigation.down: root.todayButton

                onClicked: root.monthViewRoot.digitalClock.action("configure").trigger()
                PlasmaComponents3.ToolTip {
                    text: parent.text
                }
            }

            // Allows the user to keep the calendar open for reference
            PlasmaComponents3.ToolButton {
                id: pinButton

                checkable: true
                checked: root.monthViewRoot.digitalClock.configuration.pin

                display: PlasmaComponents3.AbstractButton.IconOnly
                icon.name: "window-pin"
                text: i18n("Keep Open")

                KeyNavigation.down: root.nextButton

                onToggled: root.monthViewRoot.digitalClock.configuration.pin = checked

                PlasmaComponents3.ToolTip {
                    text: parent.text
                }
            }
        }
    }
}
