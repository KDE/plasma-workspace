/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.ksvg as KSvg
import org.kde.plasma.workspace.calendar as PlasmaCalendar
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.private.digitalclock
import org.kde.config as KConfig
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami

// Top-level layout containing:
// - Leading column with world clock and agenda view
// - Trailing column with current date header and calendar
//
// Trailing column fills exactly half of the popup width, then there's 1
// logical pixel wide separator, and the rest is left for the Leading.
// Representation's header is intentionally zero-sized, because Calendar view
// brings its own header, and there's currently no other way to stack them.
PlasmaExtras.Representation {
    id: calendar

    readonly property var appletInterface: root

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    Layout.minimumWidth: (calendar.showAgenda || calendar.showClocks) ? Kirigami.Units.gridUnit * 45 : Kirigami.Units.gridUnit * 22
    Layout.maximumWidth: Kirigami.Units.gridUnit * 80

    Layout.minimumHeight: Kirigami.Units.gridUnit * 25
    Layout.maximumHeight: Kirigami.Units.gridUnit * 40

    collapseMarginsHint: true

    readonly property int paddings: Kirigami.Units.largeSpacing
    readonly property bool showAgenda: eventPluginsManager.enabledPlugins.length > 0
    readonly property bool showClocks: Plasmoid.configuration.selectedTimeZones.length > 1

    readonly property alias monthView: monthView

    Keys.onDownPressed: event => {
        monthView.Keys.downPressed(event);
    }

    Connections {
        target: root

        function onExpandedChanged() {
            // clear all the selections when the plasmoid is showing/hiding
            monthView.resetToToday();
        }
    }

    PlasmaCalendar.EventPluginsManager {
        id: eventPluginsManager
        enabledPlugins: Plasmoid.configuration.enabledCalendarPlugins
    }

    // Having this in place helps preserving top margins for Pin and Configure
    // buttons somehow. Actual headers are spread across leading and trailing
    // columns.
    header: Item {}

    // Leading column containing agenda view and time zones
    // ==================================================
    ColumnLayout {
        id: leadingColumn

        visible: calendar.showAgenda || calendar.showClocks

        anchors {
            top: parent.top
            left: parent.left
            right: mainSeparator.left
            bottom: parent.bottom
        }

        spacing: 0

        PlasmaExtras.PlasmoidHeading {
            Layout.fillWidth: true
            Layout.preferredHeight: monthView.viewHeader.height
            leftInset: 0
            rightInset: 0

            // Agenda view header
            // -----------------
            contentItem: ColumnLayout {
                spacing: 0

                Kirigami.Heading {
                    Layout.alignment: Qt.AlignTop
                    // Match calendar title
                    Layout.leftMargin: calendar.paddings
                    Layout.rightMargin: calendar.paddings
                    Layout.fillWidth: true

                    text: monthView.currentDate.toLocaleDateString(Qt.locale(), Locale.LongFormat)
                    textFormat: Text.PlainText
                }

                PlasmaComponents.Label {
                    visible: monthView.currentDateAuxilliaryText.length > 0

                    Layout.leftMargin: calendar.paddings
                    Layout.rightMargin: calendar.paddings
                    Layout.fillWidth: true

                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                    text: monthView.currentDateAuxilliaryText
                    textFormat: Text.PlainText
                }

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: Kirigami.Units.mediumSpacing

                    // Heading text
                    Kirigami.Heading {
                        visible: agenda.visible

                        Layout.fillWidth: true
                        Layout.leftMargin: calendar.paddings
                        Layout.rightMargin: calendar.paddings

                        level: 2

                        text: i18n("Events")
                        textFormat: Text.PlainText
                        maximumLineCount: 1
                        elide: Text.ElideRight
                    }
                    PlasmaComponents.ToolButton {
                        id: addEventButton

                        visible: agenda.visible && ApplicationIntegration.calendarInstalled
                        text: i18nc("@action:button Add event", "Add…")
                        Layout.rightMargin: Kirigami.Units.smallSpacing
                        icon.name: "list-add"

                        Accessible.description: i18nc("@info:tooltip", "Add a new event")
                        KeyNavigation.down: KeyNavigation.tab
                        KeyNavigation.right: monthView.viewHeader.tabBar

                        onClicked: ApplicationIntegration.launchCalendar()
                        KeyNavigation.tab: calendar.showAgenda && eventsList.count ? eventsList : eventsList.KeyNavigation.down
                    }
                }
            }
        }

        // Agenda view itself
        Item {
            id: agenda
            visible: calendar.showAgenda

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: Kirigami.Units.gridUnit * 4

            function formatDateWithoutYear(date: date): string {
                // Unfortunatelly Qt overrides ECMA's Date.toLocaleDateString(),
                // which is able to return locale-specific date-and-month-only date
                // formats, with its dumb version that only supports Qt::DateFormat
                // enum subset. So to get a day-and-month-only date format string we
                // must resort to this magic and hope there are no locales that use
                // other separators...
                const format = Qt.locale().dateFormat(Locale.ShortFormat).replace(/[./ ]*Y{2,4}[./ ]*/i, '');
                return Qt.formatDate(date, format);
            }

            function dateEquals(date1: date, date2: date): bool {
                // Compare two dates without taking time into account
                return date1.getFullYear() === date2.getFullYear()
                    && date1.getMonth() === date2.getMonth()
                    && date1.getDate() === date2.getDate();
            }

            function updateEventsForCurrentDate() {
                eventsList.model = monthView.daysModel.eventsForDate(monthView.currentDate);
            }

            Connections {
                target: monthView

                function onCurrentDateChanged() {
                    agenda.updateEventsForCurrentDate();
                }
            }

            Connections {
                target: monthView.daysModel

                function onAgendaUpdated(updatedDate: date) {
                    if (agenda.dateEquals(updatedDate, monthView.currentDate)) {
                        agenda.updateEventsForCurrentDate();
                    }
                }
            }

            TextMetrics {
                id: dateLabelMetrics

                // Date/time are arbitrary values with all parts being two-digit
                readonly property string timeString: Qt.formatTime(new Date(2000, 12, 12, 12, 12, 12, 12))
                readonly property string dateString: agenda.formatDateWithoutYear(new Date(2000, 12, 12, 12, 12, 12))

                font: Kirigami.Theme.defaultFont
                text: timeString.length > dateString.length ? timeString : dateString
            }

            PlasmaComponents.ScrollView {
                id: eventsView
                anchors.fill: parent

                ListView {
                    id: eventsList

                    focus: false
                    activeFocusOnTab: true
                    highlight: null
                    currentIndex: -1

                    KeyNavigation.down: switchTimeZoneButton.visible ? switchTimeZoneButton : clocksList
                    Keys.onRightPressed: event => switchTimeZoneButton.Keys.rightPressed(event)

                    onCurrentIndexChanged: if (!activeFocus) {
                        currentIndex = -1;
                    }

                    onActiveFocusChanged: if (activeFocus) {
                        currentIndex = 0;
                    } else {
                        currentIndex = -1;
                    }

                    delegate: PlasmaComponents.ItemDelegate {
                        id: eventItem

                        // Crashes if the type is declared as eventData (which is Q_GADGET)
                        required property /*PlasmaCalendar.eventData*/var modelData

                        width: ListView.view.width

                        leftPadding: calendar.paddings

                        text: eventTitle.text
                        hoverEnabled: true
                        highlighted: ListView.isCurrentItem
                        Accessible.description: modelData.description
                        readonly property bool hasTime: {
                            // Explicitly all-day event
                            if (modelData.isAllDay) {
                                return false;
                            }
                            // Multi-day event which does not start or end today (so
                            // is all-day from today's point of view)
                            if (modelData.startDateTime - monthView.currentDate < 0 &&
                                modelData.endDateTime - monthView.currentDate > 86400000) { // 24hrs in ms
                                return false;
                            }

                            // Non-explicit all-day event
                            const startIsMidnight = modelData.startDateTime.getHours() === 0
                                && modelData.startDateTime.getMinutes() === 0;

                            const endIsMidnight = modelData.endDateTime.getHours() === 0
                                && modelData.endDateTime.getMinutes() === 0;

                            const sameDay = modelData.startDateTime.getDate() === modelData.endDateTime.getDate()
                                && modelData.startDateTime.getDay() === modelData.endDateTime.getDay();

                            return !(startIsMidnight && endIsMidnight && sameDay);
                        }

                        PlasmaComponents.ToolTip {
                            text: eventItem.modelData.description
                            visible: text !== "" && eventItem.hovered
                        }

                        contentItem: GridLayout {
                            id: eventGrid
                            columns: 3
                            rows: 2
                            rowSpacing: 0
                            columnSpacing: Kirigami.Units.largeSpacing

                            Rectangle {
                                id: eventColor

                                Layout.row: 0
                                Layout.column: 0
                                Layout.rowSpan: 2
                                Layout.fillHeight: true

                                color: eventItem.modelData.eventColor
                                width: 5
                                visible: eventItem.modelData.eventColor !== ""
                            }

                            PlasmaComponents.Label {
                                id: startTimeLabel

                                readonly property bool startsToday: eventItem.modelData.startDateTime - monthView.currentDate >= 0
                                readonly property bool startedYesterdayLessThan12HoursAgo: eventItem.modelData.startDateTime - monthView.currentDate >= -43200000 //12hrs in ms

                                Layout.row: 0
                                Layout.column: 1
                                Layout.minimumWidth: dateLabelMetrics.width

                                text: startsToday || startedYesterdayLessThan12HoursAgo
                                    ? Qt.formatTime(eventItem.modelData.startDateTime)
                                    : agenda.formatDateWithoutYear(eventItem.modelData.startDateTime)
                                textFormat: Text.PlainText
                                horizontalAlignment: Qt.AlignRight
                                visible: eventItem.hasTime
                            }

                            PlasmaComponents.Label {
                                id: endTimeLabel

                                readonly property bool endsToday: eventItem.modelData.endDateTime - monthView.currentDate <= 86400000 // 24hrs in ms
                                readonly property bool endsTomorrowInLessThan12Hours: eventItem.modelData.endDateTime - monthView.currentDate <= 86400000 + 43200000 // 36hrs in ms

                                Layout.row: 1
                                Layout.column: 1
                                Layout.minimumWidth: dateLabelMetrics.width

                                text: endsToday || endsTomorrowInLessThan12Hours
                                    ? Qt.formatTime(eventItem.modelData.endDateTime)
                                    : agenda.formatDateWithoutYear(eventItem.modelData.endDateTime)
                                textFormat: Text.PlainText
                                horizontalAlignment: Qt.AlignRight
                                opacity: 0.7

                                visible: eventItem.hasTime
                            }

                            PlasmaComponents.Label {
                                id: eventTitle

                                Layout.row: 0
                                Layout.column: 2
                                Layout.fillWidth: true

                                elide: Text.ElideRight
                                text: eventItem.modelData.title
                                textFormat: Text.PlainText
                                verticalAlignment: Text.AlignVCenter
                                maximumLineCount: 2
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: eventsView
                width: eventsView.width - (Kirigami.Units.gridUnit * 8)

                visible: eventsList.count === 0

                iconName: "checkmark"
                text: monthView.isToday(monthView.currentDate)
                    ? i18n("No events for today")
                    : i18n("No events for this day");
            }
        }

        // Horizontal separator line between events and time zones
        KSvg.SvgItem {
            visible: worldClocks.visible && agenda.visible

            Layout.fillWidth: true
            Layout.preferredHeight: naturalSize.height

            imagePath: "widgets/line"
            elementId: "horizontal-line"
        }

        // Clocks stuff
        // ------------
        // Header text + button to change time & time zone
        PlasmaExtras.PlasmoidHeading {
            visible: worldClocks.visible

            enabledBorders: Qt.TopEdge | Qt.BottomEdge
            // Normally gets some positive/negative values from base component.
            topInset: 0
            topPadding: Kirigami.Units.smallSpacing

            leftInset: 0
            rightInset: 0
            leftPadding: mirrored ? Kirigami.Units.smallSpacing : calendar.paddings
            rightPadding: mirrored ? calendar.paddings : Kirigami.Units.smallSpacing

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    Layout.fillWidth: true

                    level: 2

                    text: i18n("Time Zones")
                    textFormat: Text.PlainText
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    Accessible.ignored: true
                }

                PlasmaComponents.ToolButton {
                    id: switchTimeZoneButton

                    visible: KConfig.KAuthorized.authorizeControlModule("kcm_clock.desktop")
                    text: i18n("Switch…")
                    icon.name: "preferences-system-time"

                    Accessible.name: i18n("Switch to another time zone")
                    Accessible.description: i18n("Switch to another time zone")

                    KeyNavigation.down: clocksList
                    Keys.onRightPressed: event => {
                        monthView.Keys.downPressed(event);
                    }

                    onClicked: KCMUtils.KCMLauncher.openSystemSettings("kcm_clock")

                    PlasmaComponents.ToolTip {
                        text: parent.Accessible.description
                    }
                }
            }
        }

        // Clocks view itself
        PlasmaComponents.ScrollView {
            id: worldClocks
            visible: calendar.showClocks

            Layout.fillWidth: true
            Layout.fillHeight: !agenda.visible
            Layout.minimumHeight: visible ? Kirigami.Units.gridUnit * 7 : 0
            Layout.maximumHeight: agenda.visible ? Kirigami.Units.gridUnit * 10 : -1

            ListView {
                id: clocksList
                activeFocusOnTab: true

                highlight: null
                currentIndex: -1
                onActiveFocusChanged: if (activeFocus) {
                    currentIndex = 0;
                } else {
                    currentIndex = -1;
                }

                Keys.onRightPressed: event => {
                    switchTimeZoneButton.Keys.rightPressed(event);
                }

                // Can't use KeyNavigation.tab since the focus won't go to config button, instead it will be redirected to somewhere else because of
                // some existing code. Since now the header was in this file and this was not a problem. Now the header is also implicitly
                // inside the monthViewWrapper.
                Keys.onTabPressed: event => {
                    monthView.viewHeader.configureButton.forceActiveFocus(Qt.BacktabFocusReason);
                }

                model: root.selectedTimeZonesDeduplicatingExplicitLocalTimeZone()

                delegate: PlasmaComponents.ItemDelegate {
                    id: listItem

                    required property string modelData

                    readonly property bool isCurrentTimeZone: root.timeZoneResolvesToLastSelectedTimeZone(modelData)

                    width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin

                    leftPadding: calendar.paddings
                    rightPadding: calendar.paddings

                    highlighted: ListView.isCurrentItem
                    Accessible.name: root.displayStringForTimeZone(modelData)
                    Accessible.description: root.timeForZone(modelData, Plasmoid.configuration.showSeconds === 2)

                    // Only highlight with keyboard
                    down: false
                    hoverEnabled: false

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        PlasmaComponents.Label {
                            Layout.fillWidth: true
                            text: root.displayStringForTimeZone(listItem.modelData)
                            textFormat: Text.PlainText
                            font.weight: listItem.isCurrentTimeZone ? Font.Bold : Font.Normal
                            maximumLineCount: 1
                            elide: Text.ElideRight
                        }

                        PlasmaComponents.Label {
                            horizontalAlignment: Qt.AlignRight
                            text: root.timeForZone(listItem.modelData, Plasmoid.configuration.showSeconds === 2)
                            textFormat: Text.PlainText
                            font.weight: listItem.isCurrentTimeZone ? Font.Bold : Font.Normal
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }
                    }
                }
            }
        }
    }

    // Vertical separator line between columns
    // =======================================
    KSvg.SvgItem {
        id: mainSeparator

        anchors {
            top: parent.top
            right: monthViewWrapper.left
            bottom: parent.bottom
            // Stretch all the way to the top of a dialog. This magic comes
            // from PlasmaCore.Dialog::margins and CompactApplet containment.
            topMargin: calendar.parent ? -calendar.parent.y : 0
        }

        width: naturalSize.width
        visible: calendar.showAgenda || calendar.showClocks

        imagePath: "widgets/line"
        elementId: "vertical-line"
    }

    // Trailing column containing calendar
    // ===============================
    FocusScope {
        id: monthViewWrapper

        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }

        // Not anchoring to horizontalCenter to avoid sub-pixel misalignments
        width: (calendar.showAgenda || calendar.showClocks) ? Math.round(parent.width / 2) : parent.width

        onActiveFocusChanged: if (activeFocus) {
            monthViewWrapper.nextItemInFocusChain().forceActiveFocus();
        }

        PlasmaCalendar.MonthView {
            id: monthView

            anchors {
                fill: parent
                leftMargin: Kirigami.Units.smallSpacing
                rightMargin: Kirigami.Units.smallSpacing
                bottomMargin: Kirigami.Units.smallSpacing
            }

            borderOpacity: 0.25

            eventPluginsManager: eventPluginsManager
            today: root.currentDateTimeInSelectedTimeZone
            firstDayOfWeek: Plasmoid.configuration.firstDayOfWeek > -1
                ? Plasmoid.configuration.firstDayOfWeek
                : Qt.locale().firstDayOfWeek
            showWeekNumbers: Plasmoid.configuration.showWeekNumbers

            showDigitalClockHeader: true
            digitalClock: Plasmoid
            eventButton: addEventButton

            KeyNavigation.left: KeyNavigation.tab
            KeyNavigation.tab: addEventButton.visible ? addEventButton : addEventButton.KeyNavigation.down
            Keys.onUpPressed: event => {
                viewHeader.tabBar.currentItem.forceActiveFocus(Qt.BacktabFocusReason);
            }
        }
    }
}
