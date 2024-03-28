/*
    SPDX-FileCopyrightText: 2022 Tanbir Jishan <tantalising007@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras

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

    required property T.SwipeView swipeView
    // Can't use the real type due to a "Cyclic dependency"
    required property QtObject /*MonthView*/ monthViewRoot

    readonly property bool isDigitalClock: monthViewRoot.showDigitalClockHeader
    readonly property var buttons: isDigitalClock ? dateManipulationButtonsForDigitalClock : dateManipulationButtons
    readonly property T.AbstractButton tabButton: isDigitalClock ? configureButton : todayButton
    readonly property T.AbstractButton previousButton: buttons.previousButton
    readonly property T.AbstractButton todayButton: buttons.todayButton
    readonly property T.AbstractButton nextButton: buttons.nextButton
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
        active: root.isDigitalClock
    }

    ColumnLayout {
        id: viewHeader
        width: parent.width

        RowLayout {
            spacing: 0
            Layout.leftMargin: Kirigami.Units.largeSpacing

            Kirigami.Heading {
                id: heading
                // Needed for Appium testing
                objectName: "monthHeader"

                text: root.swipeView.currentIndex > 0 || root.monthViewRoot.selectedYear !== today.getFullYear()
                    ? i18ndc("plasmashellprivateplugin", "Format: month year", "%1 %2",
                        root.monthViewRoot.selectedMonth, root.monthViewRoot.selectedYear.toString())
                    : root.monthViewRoot.selectedMonth
                textFormat: Text.PlainText
                level: root.isDigitalClock ? 1 : 2
                elide: Text.ElideRight
                font.capitalization: Font.Capitalize
                Layout.fillWidth: true
            }

            Loader {
                id: dateManipulationButtons

                readonly property T.AbstractButton previousButton: item?.previousButton ?? null
                readonly property T.AbstractButton todayButton: item?.todayButton ?? null
                readonly property T.AbstractButton nextButton: item?.nextButton ?? null

                sourceComponent: buttonsGroup
                active: !root.isDigitalClock
            }

            Loader {
                id: dateAndPinButtons

                readonly property T.AbstractButton configureButton: item?.configureButton ?? null
                readonly property T.AbstractButton pinButton: item?.pinButton ?? null

                sourceComponent: dateAndPin
                active: root.isDigitalClock
            }
        }

        RowLayout {
            spacing: 0
            PlasmaComponents.TabBar {
                id: tabBar

                currentIndex: root.swipeView.currentIndex
                Layout.fillWidth: true
                Layout.bottomMargin: root.isDigitalClock ? 0 : Kirigami.Units.smallSpacing

                KeyNavigation.up: root.isDigitalClock ? root.configureButton : root.previousButton
                KeyNavigation.right: dateManipulationButtonsForDigitalClock.previousButton
                KeyNavigation.left: root.monthViewRoot.eventButton && root.monthViewRoot.eventButton.visible
                    ? root.monthViewRoot.eventButton
                    : root.monthViewRoot.eventButton && root.monthViewRoot.eventButton.KeyNavigation.down

                PlasmaComponents.TabButton {
                    Accessible.onPressAction: clicked()
                    text: i18nd("plasmashellprivateplugin", "Days");
                    onClicked: root.monthViewRoot.showMonthView();
                    display: T.AbstractButton.TextOnly
                }
                PlasmaComponents.TabButton {
                    Accessible.onPressAction: clicked()
                    text: i18nd("plasmashellprivateplugin", "Months");
                    onClicked: root.monthViewRoot.showYearView();
                    display: T.AbstractButton.TextOnly
                }
                PlasmaComponents.TabButton {
                    Accessible.onPressAction: clicked()
                    text: i18nd("plasmashellprivateplugin", "Years");
                    onClicked: root.monthViewRoot.showDecadeView();
                    display: T.AbstractButton.TextOnly
                }
            }

            Loader {
                id: dateManipulationButtonsForDigitalClock

                readonly property T.AbstractButton previousButton: item?.previousButton ?? null
                readonly property T.AbstractButton todayButton: item?.todayButton ?? null
                readonly property T.AbstractButton nextButton: item?.nextButton ?? null

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

            PlasmaComponents.ToolButton {
                id: previousButton
                text: {
                    switch (root.monthViewRoot.calendarViewDisplayed) {
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
                display: T.AbstractButton.IconOnly
                KeyNavigation.right: todayButton

                onClicked: root.monthViewRoot.previousView()

                PlasmaComponents.ToolTip { text: parent.text }
            }

            PlasmaComponents.ToolButton {
                id: todayButton
                text: i18ndc("plasmashellprivateplugin", "Reset calendar to today", "Today")
                Accessible.description: i18nd("plasmashellprivateplugin", "Reset calendar to today")
                KeyNavigation.right: nextButton

                onClicked: root.monthViewRoot.resetToToday()
            }

            PlasmaComponents.ToolButton {
                id: nextButton
                text: {
                    switch (root.monthViewRoot.calendarViewDisplayed) {
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
                display: T.AbstractButton.IconOnly
                KeyNavigation.tab: root.swipeView

                onClicked: root.monthViewRoot.nextView();

                PlasmaComponents.ToolTip { text: parent.text }
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

            PlasmaComponents.ToolButton {
                id: configureButton

                visible: root.monthViewRoot.digitalClock.internalAction("configure").enabled

                display: T.AbstractButton.IconOnly
                icon.name: "configure"
                text: root.monthViewRoot.digitalClock.internalAction("configure").text

                KeyNavigation.left: tabBar.KeyNavigation.left
                KeyNavigation.right: pinButton
                KeyNavigation.down: root.todayButton

                onClicked: root.monthViewRoot.digitalClock.internalAction("configure").trigger()
                PlasmaComponents.ToolTip {
                    text: parent.text
                }
            }

            // Allows the user to keep the calendar open for reference
            PlasmaComponents.ToolButton {
                id: pinButton

                checkable: true
                checked: root.monthViewRoot.digitalClock.configuration.pin

                display: T.AbstractButton.IconOnly
                icon.name: "window-pin"
                text: i18n("Keep Open")

                KeyNavigation.down: root.nextButton

                onToggled: root.monthViewRoot.digitalClock.configuration.pin = checked

                PlasmaComponents.ToolTip {
                    text: parent.text
                }
            }
        }
    }
}
