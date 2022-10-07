/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.4
import QtQuick.Layouts 1.1
import QtQml 2.15

import org.kde.kquickcontrolsaddons 2.0 // For kcmshell
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.workspace.calendar 2.0 as PlasmaCalendar
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.private.digitalclock 1.0

// Top-level layout containing:
// - Left column with world clock and agenda view
// - Right column with current date header and calendar
PlasmaExtras.Representation {
    id: calendar

    readonly property var appletInterface: Plasmoid.self

    PlasmaCore.ColorScope.inherit: false
    PlasmaCore.ColorScope.colorGroup: PlasmaCore.Theme.NormalColorGroup

    Layout.minimumWidth: (calendar.showAgenda || calendar.showClocks) ? PlasmaCore.Units.gridUnit * 45 : PlasmaCore.Units.gridUnit * 22
    Layout.maximumWidth: PlasmaCore.Units.gridUnit * 80

    Layout.minimumHeight: PlasmaCore.Units.gridUnit * 25
    Layout.maximumHeight: PlasmaCore.Units.gridUnit * 40

    collapseMarginsHint: true

    readonly property int paddings: PlasmaCore.Units.smallSpacing
    readonly property bool showAgenda: eventPluginsManager.enabledPlugins.length > 0
    readonly property bool showClocks: Plasmoid.configuration.selectedTimeZones.length > 1

    property alias borderWidth: monthView.borderWidth
    property alias monthView: monthView

    property bool debug: false

    Keys.onDownPressed: monthView.Keys.onDownPressed(event);

    Connections {
        target: Plasmoid.self

        function onExpandedChanged() {
            // clear all the selections when the plasmoid is showing/hiding
            monthView.resetToToday();
        }
    }

    header: Item {
        PlasmaExtras.PlasmoidHeading {
            width: Math.round(parent.width / 2) - monthView.anchors.leftMargin
            visible: eventHeader.visible

            anchors {
                left: eventHeader.left
                top: eventHeader.top
                bottom: eventHeader.bottom
            }
        }

        // Agenda view header
        // -----------------
        ColumnLayout {
            id: eventHeader

            anchors.left: parent.left
            width: visible ? parent.width / 2 - 1 : 0
            height: visible ? monthView.viewHeader.height : 0
            spacing: 0

            visible: calendar.showAgenda || calendar.showClocks

            PlasmaExtras.Heading {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.leftMargin: calendar.paddings // Match calendar title

                text: monthView.currentDate.toLocaleDateString(Qt.locale(), Locale.LongFormat)
            }

            PlasmaComponents3.Label {
                visible: monthView.currentDateAuxilliaryText.length > 0
                Layout.leftMargin: calendar.paddings
                font.pixelSize: PlasmaCore.Theme.smallestFont.pixelSize
                text: monthView.currentDateAuxilliaryText
            }

            RowLayout {
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: Math.round(PlasmaCore.Units.smallSpacing * 1.5)
                // Heading text
                PlasmaExtras.Heading {
                    visible: agenda.visible

                    Layout.fillWidth: true
                    Layout.leftMargin: calendar.paddings

                    level: 2

                    text: i18n("Events")
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }
                PlasmaComponents3.ToolButton {
                    id: addEventButton

                    visible: agenda.visible && ApplicationIntegration.calendarInstalled
                    text: i18nc("@action:button Add event", "Add…")
                    Layout.rightMargin: calendar.paddings
                    icon.name: "list-add"

                    Accessible.description: i18nc("@info:tooltip", "Add a new event")
                    KeyNavigation.down: KeyNavigation.tab
                    KeyNavigation.right: monthView.viewHeader.tabBar

                    onClicked: ApplicationIntegration.launchCalendar()
                    KeyNavigation.tab: calendar.showAgenda && holidaysList.count ? holidaysList : holidaysList.KeyNavigation.down
                }
            }
        }
        // Vertical separator line between columns
        // =======================================
        PlasmaCore.SvgItem {
            id: headerSeparator
            anchors.left: eventHeader.right
            anchors.bottomMargin: PlasmaCore.Units.smallSpacing * 2
            width: visible ? 1 : 0
            height: monthView.viewHeader.height - PlasmaCore.Units.smallSpacing * 2
            visible: eventHeader.visible

            elementId: "vertical-line"
            svg: PlasmaCore.Svg {
                imagePath: "widgets/line"
            }
        }
    }

    PlasmaCalendar.EventPluginsManager {
        id: eventPluginsManager
        enabledPlugins: Plasmoid.configuration.enabledCalendarPlugins
    }

    // Left column containing agenda view and time zones
    // ==================================================
    ColumnLayout {
        id: leftColumn

        visible: calendar.showAgenda || calendar.showClocks
        width: parent.width / 2 - 1
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            topMargin: monthView.viewHeader.height
        }


        // Agenda view itself
        Item {
            id: agenda
            visible: calendar.showAgenda

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: PlasmaCore.Units.gridUnit * 4

            function formatDateWithoutYear(date) {
                // Unfortunatelly Qt overrides ECMA's Date.toLocaleDateString(),
                // which is able to return locale-specific date-and-month-only date
                // formats, with its dumb version that only supports Qt::DateFormat
                // enum subset. So to get a day-and-month-only date format string we
                // must resort to this magic and hope there are no locales that use
                // other separators...
                var format = Qt.locale().dateFormat(Locale.ShortFormat).replace(/[./ ]*Y{2,4}[./ ]*/i, '');
                return Qt.formatDate(date, format);
            }

            function dateEquals(date1, date2) {
                const values1 = [
                    date1.getFullYear(),
                    date1.getMonth(),
                    date1.getDate()
                ];

                const values2 = [
                    date2.getFullYear(),
                    date2.getMonth(),
                    date2.getDate()
                ];

                return values1.every((value, index) => {
                    return (value === values2[index]);
                }, false)
            }

            Connections {
                target: monthView

                function onCurrentDateChanged() {
                    // Apparently this is needed because this is a simple QList being
                    // returned and if the list for the current day has 1 event and the
                    // user clicks some other date which also has 1 event, QML sees the
                    // sizes match and does not update the labels with the content.
                    // Resetting the model to null first clears it and then correct data
                    // are displayed.
                    holidaysList.model = null;
                    holidaysList.model = monthView.daysModel.eventsForDate(monthView.currentDate);
                }
            }

            Connections {
                target: monthView.daysModel

                function onAgendaUpdated(updatedDate) {
                    if (agenda.dateEquals(updatedDate, monthView.currentDate)) {
                        holidaysList.model = null;
                        holidaysList.model = monthView.daysModel.eventsForDate(monthView.currentDate);
                    }
                }
            }

            Binding {
                target: Plasmoid.self
                property: "hideOnWindowDeactivate"
                value: !Plasmoid.configuration.pin
                restoreMode: Binding.RestoreBinding
            }

            TextMetrics {
                id: dateLabelMetrics

                // Date/time are arbitrary values with all parts being two-digit
                readonly property string timeString: Qt.formatTime(new Date(2000, 12, 12, 12, 12, 12, 12))
                readonly property string dateString: agenda.formatDateWithoutYear(new Date(2000, 12, 12, 12, 12, 12))

                font: PlasmaCore.Theme.defaultFont
                text: timeString.length > dateString.length ? timeString : dateString
            }

            PlasmaComponents3.ScrollView {
                id: holidaysView
                anchors.fill: parent

                // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
                PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

                ListView {
                    id: holidaysList

                    focus: false
                    activeFocusOnTab: true
                    highlight: null
                    currentIndex: -1

                    KeyNavigation.down: switchTimeZoneButton.visible ? switchTimeZoneButton : clocksList
                    Keys.onRightPressed: switchTimeZoneButton.Keys.onRightPressed(event);

                    onCurrentIndexChanged: if (!activeFocus) {
                        currentIndex = -1;
                    }
 
                    onActiveFocusChanged: if (activeFocus) {
                        currentIndex = 0;
                    } else {
                        currentIndex = -1;
                    }

                    delegate: PlasmaComponents3.ItemDelegate {
                        id: eventItem
                        width: holidaysList.width
                        padding: calendar.paddings
                        leftPadding: calendar.paddings + PlasmaCore.Units.smallSpacing * 2
                        text: eventTitle.text
                        hoverEnabled: true
                        highlighted: ListView.isCurrentItem
                        Accessible.description: modelData.description
                        property bool hasTime: {
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
                                    && modelData.startDateTime.getDay() === modelData.endDateTime.getDay()

                            return !(startIsMidnight && endIsMidnight && sameDay);
                        }

                        PlasmaComponents3.ToolTip {
                            text: modelData.description
                            visible: text !== "" && eventItem.hovered
                        }

                        contentItem: GridLayout {
                            id: eventGrid
                            columns: 3
                            rows: 2
                            rowSpacing: 0
                            columnSpacing: 2 * PlasmaCore.Units.smallSpacing

                            Rectangle {
                                id: eventColor

                                Layout.row: 0
                                Layout.column: 0
                                Layout.rowSpan: 2
                                Layout.fillHeight: true

                                color: modelData.eventColor
                                width: 5 * PlasmaCore.Units.devicePixelRatio
                                visible: modelData.eventColor !== ""
                            }

                            PlasmaComponents3.Label {
                                id: startTimeLabel

                                readonly property bool startsToday: modelData.startDateTime - monthView.currentDate >= 0
                                readonly property bool startedYesterdayLessThan12HoursAgo: modelData.startDateTime - monthView.currentDate >= -43200000 //12hrs in ms

                                Layout.row: 0
                                Layout.column: 1
                                Layout.minimumWidth: dateLabelMetrics.width

                                text: startsToday || startedYesterdayLessThan12HoursAgo
                                        ? Qt.formatTime(modelData.startDateTime)
                                        : agenda.formatDateWithoutYear(modelData.startDateTime)
                                horizontalAlignment: Qt.AlignRight
                                visible: eventItem.hasTime
                            }

                            PlasmaComponents3.Label {
                                id: endTimeLabel

                                readonly property bool endsToday: modelData.endDateTime - monthView.currentDate <= 86400000 // 24hrs in ms
                                readonly property bool endsTomorrowInLessThan12Hours: modelData.endDateTime - monthView.currentDate <= 86400000 + 43200000 // 36hrs in ms

                                Layout.row: 1
                                Layout.column: 1
                                Layout.minimumWidth: dateLabelMetrics.width

                                text: endsToday || endsTomorrowInLessThan12Hours
                                        ? Qt.formatTime(modelData.endDateTime)
                                        : agenda.formatDateWithoutYear(modelData.endDateTime)
                                horizontalAlignment: Qt.AlignRight
                                opacity: 0.7

                                visible: eventItem.hasTime
                            }

                            PlasmaComponents3.Label {
                                id: eventTitle

                                Layout.row: 0
                                Layout.column: 2
                                Layout.fillWidth: true

                                elide: Text.ElideRight
                                text: modelData.title
                                verticalAlignment: Text.AlignVCenter
                                maximumLineCount: 2
                            }
                        }
                    }
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: holidaysView
                width: holidaysView.width - (PlasmaCore.Units.largeSpacing * 8)

                visible: holidaysList.count == 0

                iconName: "checkmark"
                text: monthView.isToday(monthView.currentDate) ? i18n("No events for today")
                                                               : i18n("No events for this day");
            }
        }

        // Horizontal separator line between events and time zones
        PlasmaCore.SvgItem {
            visible: worldClocks.visible && agenda.visible

            Layout.fillWidth: true
            Layout.preferredHeight: naturalSize.height

            elementId: "horizontal-line"
            svg: PlasmaCore.Svg {
                imagePath: "widgets/line"
            }
        }

        // Clocks stuff
        // ------------
        // Header text + button to change time & timezone
        PlasmaExtras.PlasmoidHeading {
            visible: worldClocks.visible
            leftInset: 0
            rightInset: 0
            rightPadding: PlasmaCore.Units.smallSpacing

            contentItem: RowLayout {
                PlasmaExtras.Heading {
                    Layout.leftMargin: calendar.paddings + PlasmaCore.Units.smallSpacing * 2
                    Layout.fillWidth: true

                    level: 2

                    text: i18n("Time Zones")
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }

                PlasmaComponents3.ToolButton {
                    id: switchTimeZoneButton

                    visible: KCMShell.authorize("kcm_clock.desktop").length > 0
                    text: i18n("Switch…")
                    Accessible.name: i18n("Switch to another timezone")
                    icon.name: "preferences-system-time"

                    Accessible.description: i18n("Switch to another timezone")
                    KeyNavigation.down: clocksList
                    Keys.onRightPressed: monthView.Keys.onDownPressed(event)

                    onClicked: KCMShell.openSystemSettings("kcm_clock")

                    PlasmaComponents3.ToolTip {
                        text: parent.Accessible.description
                    }
                }
            }
        }

        // Clocks view itself
        PlasmaComponents3.ScrollView {
            id: worldClocks
            visible: calendar.showClocks

            Layout.fillWidth: true
            Layout.fillHeight: !agenda.visible
            Layout.minimumHeight: visible ? PlasmaCore.Units.gridUnit * 7 : 0
            Layout.maximumHeight: agenda.visible ? PlasmaCore.Units.gridUnit * 10 : -1

            // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
            PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

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

                Keys.onRightPressed: switchTimeZoneButton.Keys.onRightPressed(event);

                // Can't use KeyNavigation.tab since the focus won't go to config button, instead it will be redirected to somewhere else because of
                // some existing code. Since now the header was in this file and this was not a problem. Now the header is also implicitly
                // inside the monthViewWrapper.
                Keys.onTabPressed: {
                    monthViewWrapper.focusChangedForClockListTab = true;
                    monthView.viewHeader.configureButton.forceActiveFocus(Qt.BacktabFocusReason);
                    monthViewWrapper.focusChangedForClockListTab = false;
                }

                model: {
                    let timezones = [];
                    for (let i = 0; i < Plasmoid.configuration.selectedTimeZones.length; i++) {
                        let thisTzData = Plasmoid.configuration.selectedTimeZones[i];

                        /* Don't add this item if it's the same as the local time zone, which
                         * would indicate that the user has deliberately added a dedicated entry
                         * for the city of their normal time zone. This is not an error condition
                         * because the user may have done this on purpose so that their normal
                         * local time zone shows up automatically while they're traveling and
                         * they've switched the current local time zone to something else. But
                         * with this use case, when they're back in their normal local time zone,
                         * the clocks list would show two entries for the same city. To avoid
                         * this, let's suppress the duplicate.
                         */
                        if (!(thisTzData !== "Local" && root.nameForZone(thisTzData) === root.nameForZone("Local"))) {
                            timezones.push(Plasmoid.configuration.selectedTimeZones[i]);
                        }
                    }
                    return timezones;
                }

                delegate: PlasmaComponents3.ItemDelegate {
                    id: listItem
                    readonly property bool isCurrentTimeZone: modelData === Plasmoid.configuration.lastSelectedTimezone
                    width: clocksList.width
                    padding: calendar.paddings
                    leftPadding: calendar.paddings + PlasmaCore.Units.smallSpacing * 2
                    rightPadding: calendar.paddings + PlasmaCore.Units.smallSpacing * 2
                    highlighted: ListView.isCurrentItem
                    Accessible.name: root.nameForZone(modelData)
                    Accessible.description: root.timeForZone(modelData)
                    hoverEnabled: false

                    contentItem: RowLayout {
                        PlasmaComponents3.Label {
                            text: root.nameForZone(modelData)
                            font.weight: listItem.isCurrentTimeZone ? Font.Bold : Font.Normal
                            maximumLineCount: 1
                            elide: Text.ElideRight
                        }

                        PlasmaComponents3.Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Qt.AlignRight
                            text: root.timeForZone(modelData)
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
    PlasmaCore.SvgItem {
        id: mainSeparator
        visible: leftColumn.visible
        anchors {
            right: monthViewWrapper.left
            top: parent.top
            bottom: parent.bottom
        }
        width: 1

        elementId: "vertical-line"
        svg: PlasmaCore.Svg {
            imagePath: "widgets/line"
        }
    }

    // Right column containing calendar
    // ===============================
    FocusScope {
        id: monthViewWrapper
        width: calendar.showAgenda || calendar.showClocks ? parent.width / 2 : parent.width
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        property bool focusChangedForClockListTab: false
        onActiveFocusChanged: if (activeFocus) {
            monthViewWrapper.nextItemInFocusChain().forceActiveFocus();
            if(!focusChangedForClockListTab) {
                monthView.Keys.onDownPressed(null);
            }
        }

        PlasmaCalendar.MonthView {
            id: monthView

            anchors {
                leftMargin: PlasmaCore.Units.smallSpacing
                rightMargin: PlasmaCore.Units.smallSpacing
                bottomMargin: PlasmaCore.Units.smallSpacing
            }

            borderOpacity: 0.25

            eventPluginsManager: eventPluginsManager
            today: root.tzDate
            firstDayOfWeek: Plasmoid.configuration.firstDayOfWeek > -1
                ? Plasmoid.configuration.firstDayOfWeek
                : Qt.locale().firstDayOfWeek
            showWeekNumbers: Plasmoid.configuration.showWeekNumbers

            showDigitalClockHeader: true
            digitalClock: Plasmoid.self
            eventButton: addEventButton

            KeyNavigation.left: KeyNavigation.tab
            KeyNavigation.tab: addEventButton.visible ? addEventButton : addEventButton.KeyNavigation.down
            Keys.onUpPressed: viewHeader.tabBar.currentItem.forceActiveFocus(Qt.BacktabFocusReason);
            onUpPressed: Keys.onUpPressed(event)
        }
    }
}
