/*
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.calendar 2.0 as PlasmaCalendar
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: calendar

    Layout.minimumWidth: _minimumWidth
    Layout.minimumHeight: _minimumHeight

    // The "sensible" values
    property int _minimumWidth: _minimumHeight * 1.5 + (monthView.showWeekNumbers ? Math.round(_minimumHeight * 1.75) : Math.round(_minimumHeight * 1.5))
    property int _minimumHeight: units.gridUnit * 14
    Layout.preferredWidth: _minimumWidth
    Layout.preferredHeight: _minimumHeight * 1.5

    property int avWidth: (parent.width - (3 * units.largeSpacing)) / 2
    property int avHeight: parent.height - (2 * units.largeSpacing)

    //anchors.margins: units.largeSpacing
    property int spacing: units.largeSpacing
    property alias borderWidth: monthView.borderWidth
    property alias monthView: monthView

    property bool debug: false

    property bool isExpanded: plasmoid.expanded

    onIsExpandedChanged: {
        if (!isExpanded) {
            // clear all the selections when the plasmoid is hiding
            monthView.date = null;
            monthView.resetToToday();
        }
    }

    Item {
        id: agenda

        width: avWidth
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            leftMargin: spacing
            topMargin: spacing
            bottomMargin: spacing
        }

        function dateString(format) {
            return Qt.formatDate(monthView.currentDate, format);
        }

        Connections {
            target: monthView

            onCurrentDateChanged: {
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

            onAgendaUpdated: {
                // Checks if the dates are the same, comparing the date objects
                // directly won't work and this does a simple integer subtracting
                // so should be fastest. One of the JS weirdness.
                if (updatedDate - monthView.currentDate === 0) {
                    holidaysList.model = null;
                    holidaysList.model = monthView.daysModel.eventsForDate(monthView.currentDate);
                }
            }
        }

        Connections {
            target: plasmoid.configuration

            onEnabledCalendarPluginsChanged: {
                PlasmaCalendar.EventPluginsManager.enabledPlugins = plasmoid.configuration.enabledCalendarPlugins;
            }
        }

        PlasmaComponents.Label {
            id: dayLabel
            height: dayHeading.height + dateHeading.height
            width: paintedWidth
            font.pixelSize: height
            font.weight: Font.Light
            text: agenda.dateString("dd")
            opacity: 0.6
        }

        PlasmaExtras.Heading {
            id: dayHeading
            anchors {
                top: parent.top
                left: dayLabel.right
                right: parent.right
                leftMargin: spacing / 2
            }
            level: 1
            elide: Text.ElideRight
            text: agenda.dateString("dddd")
        }
        PlasmaComponents.Label {
            id: dateHeading
            anchors {
                top: dayHeading.bottom
                left: dayLabel.right
                right: parent.right
                leftMargin: spacing / 2
            }
            elide: Text.ElideRight
            text: Qt.locale().standaloneMonthName(monthView.currentDate.getMonth())
                             + agenda.dateString(" yyyy")
        }

        ListView {
            id: holidaysList
            anchors {
                top: dateHeading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            delegate: Item {
                id: eventItem
                width: holidaysList.width
                height: eventTitle.paintedHeight
                property bool hasTime: {
                    var startIsMidnight = modelData.startDateTime.getHours() == 0
                                       && modelData.startDateTime.getMinutes() == 0;

                    var endIsMidnight = modelData.endDateTime.getHours() == 0
                                     && modelData.endDateTime.getMinutes() == 0;

                    var sameDay = modelData.startDateTime.getDate() == modelData.endDateTime.getDate()
                               && modelData.startDateTime.getDay() == modelData.endDateTime.getDay()

                    if (startIsMidnight && endIsMidnight && sameDay) {
                        return false
                    }

                    return true;
                }

                PlasmaComponents.Label {
                    text: {
                        if (modelData.startDateTime - modelData.endDateTime === 0) {
                            return Qt.formatTime(modelData.startDateTime);
                        } else {
                            return Qt.formatTime(modelData.startDateTime) + " - " + Qt.formatTime(modelData.endDateTime);
                        }
                    }
                    visible: eventItem.hasTime
                }
                PlasmaComponents.Label {
                    id: eventTitle
                    width: eventItem.hasTime ? parent.width * 0.7 : parent.width
                    anchors.right: parent.right
                    text: modelData.title
                }
            }

            section.property: "modelData.eventType"
            section.delegate: PlasmaExtras.Heading {
                level: 3
                elide: Text.ElideRight
                text: section
            }
        }
    }
    Item {
        id: cal
        width: avWidth
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
            margins: spacing
        }

        PlasmaCalendar.MonthView {
            id: monthView
            borderOpacity: 0.25
            today: root.tzDate
            showWeekNumbers: plasmoid.configuration.showWeekNumbers
            anchors.fill: parent
        }

    }

    // Allows the user to keep the calendar open for reference
    PlasmaComponents.ToolButton {
        anchors.right: parent.right
        width: Math.round(units.gridUnit * 1.25)
        height: width
        checkable: true
        iconSource: "window-pin"
        onCheckedChanged: plasmoid.hideOnWindowDeactivate = !checked
    }

    Component.onCompleted: {
        // Set the list of enabled plugins from config
        // to the manager
        PlasmaCalendar.EventPluginsManager.enabledPlugins = plasmoid.configuration.enabledCalendarPlugins;
    }

}
