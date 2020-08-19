/*
 * Copyright 2013 Heena Mahour <heena393@gmail.com>
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 * Copyright 2013 Martin Klapetek <mklapetek@kde.org>
 * Copyright 2014 David Edmundson <davidedmundson@kde.org>
 * Copyright 2020 Carson Black <uhhadd@gmail.com>
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

import QtQuick 2.6
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components // Date label height breaks on vertical panel with PC3 version
import org.kde.plasma.private.digitalclock 1.0
import org.kde.plasma.plasmoid 2.0

Item {
    id: main

    property string timeFormat
    property date currentTime

    property var item: switch (main.state) {
        case "horizontalSmall": return horizontalSmall
        case "horizontal": return horizontal
        case "vertical": return vertical
        case "verticalSmall": return verticalSmall
        case "desktop": return desktop
        default: console.log("unreachable!")
    }

    Component { id: horizontal; HorizontalClock {} }
    Component { id: horizontalSmall; SmallHorizontalClock {} }
    Component { id: vertical; VerticalClock {} }
    Component { id: verticalSmall; VerticalClockSmall {} }
    Component { id: desktop; DesktopClock {} }

    property bool showSeconds: plasmoid.configuration.showSeconds
    property bool showLocalTimezone: plasmoid.configuration.showLocalTimezone
    property bool showDate: plasmoid.configuration.showDate
    property var dateFormat: {
        if (main.state == "verticalSmall") {
            return Qt.SystemLocaleShortDate
        }

        if (plasmoid.configuration.dateFormat === "custom") {
            return plasmoid.configuration.customDateFormat; // str
        } else if (plasmoid.configuration.dateFormat === "longDate") {
            return Qt.SystemLocaleLongDate; // int
        } else if (plasmoid.configuration.dateFormat === "isoDate") {
            return Qt.ISODate; // int
        } else { // "shortDate"
            return Qt.SystemLocaleShortDate; // int
        }
    }

    property string lastSelectedTimezone: plasmoid.configuration.lastSelectedTimezone
    property bool displayTimezoneAsCode: plasmoid.configuration.displayTimezoneAsCode
    property int use24hFormat: plasmoid.configuration.use24hFormat

    property string lastDate: ""
    property int tzOffset

    // This is the index in the list of user selected timezones
    property int tzIndex: 0

    onLastSelectedTimezoneChanged: { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }
    onShowSecondsChanged:          { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }
    onShowLocalTimezoneChanged:    { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }
    onShowDateChanged:             { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }
    onUse24hFormatChanged:         { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }

    Connections {
        target: plasmoid
        function onContextualActionsAboutToShow() {
            ClipboardMenu.secondsIncluded = main.showSeconds;
            ClipboardMenu.currentDate = main.currentTime;
        }
    }

    Connections {
        target: plasmoid.configuration
        function onSelectedTimeZonesChanged() {
            // If the currently selected timezone was removed,
            // default to the first one in the list
            var lastSelectedTimezone = plasmoid.configuration.lastSelectedTimezone;
            if (plasmoid.configuration.selectedTimeZones.indexOf(lastSelectedTimezone) === -1) {
                plasmoid.configuration.lastSelectedTimezone = plasmoid.configuration.selectedTimeZones[0];
            }

            setTimezoneIndex();
        }
    }

    states: [
        State {
            name: "horizontalSmall"
            when: plasmoid.formFactor === PlasmaCore.Types.Horizontal && plasmoid.height < PlasmaCore.Units.gridUnit * 2
        },
        State {
            name: "horizontal"
            when: plasmoid.formFactor === PlasmaCore.Types.Horizontal
        },
        State {
            name: "verticalSmall"
            when: plasmoid.formFactor === PlasmaCore.Types.Vertical && plasmoid.width < PlasmaCore.Units.gridUnit * 3
        },
        State {
            name: "vertical"
            when: plasmoid.formFactor === PlasmaCore.Types.Vertical
        },
        State {
            name: "desktop"
            when: plasmoid.formFactor !== PlasmaCore.Types.Vertical && plasmoid.formFactor !== PlasmaCore.Types.Horizontal
        }
    ]

    MouseArea {
        anchors.fill: parent
        onClicked: plasmoid.expanded = !plasmoid.expanded
        onWheel: {
            if (!plasmoid.configuration.wheelChangesTimezone) {
                return;
            }

            var delta = wheel.angleDelta.y || wheel.angleDelta.x
            var newIndex = main.tzIndex;
            wheelDelta += delta;
            // magic number 120 for common "one click"
            // See: https://doc.qt.io/qt-5/qml-qtquick-wheelevent.html#angleDelta-prop
            while (wheelDelta >= 120) {
                wheelDelta -= 120;
                newIndex--;
            }
            while (wheelDelta <= -120) {
                wheelDelta += 120;
                newIndex++;
            }

            if (newIndex >= plasmoid.configuration.selectedTimeZones.length) {
                newIndex = 0;
            } else if (newIndex < 0) {
                newIndex = plasmoid.configuration.selectedTimeZones.length - 1;
            }

            if (newIndex !== main.tzIndex) {
                plasmoid.configuration.lastSelectedTimezone = plasmoid.configuration.selectedTimeZones[newIndex];
                main.tzIndex = newIndex;

                dataSource.dataChanged();
            }
        }
    }

    property bool showTimezone: main.showLocalTimezone || (plasmoid.configuration.lastSelectedTimezone !== "Local" && dataSource.data["Local"]["Timezone City"] !== dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Timezone City"]);
    property string timeText: {
        // get the time for the given timezone from the dataengine
        let now = dataSource.data[plasmoid.configuration.lastSelectedTimezone]["DateTime"]
        // get current UTC time
        let msUTC = now.getTime() + (now.getTimezoneOffset() * 60000)
        // add the dataengine TZ offset to it
        let currentTime = new Date(msUTC + (dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Offset"] * 1000))

        main.currentTime = currentTime
        return Qt.formatTime(currentTime, main.timeFormat)
    }
    property string tzText: {
        if (plasmoid.configuration.displayTimezoneAsCode) {
            return dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Timezone Abbreviation"]
        }
        else {
            return TimezonesI18n.i18nCity(dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Timezone City"])
        }
    }
    property string dateText: Qt.formatDate(main.currentTime, main.dateFormat)

    // Qt's QLocale does not offer any modular time creating like Klocale did
    // eg. no "gimme time with seconds" or "gimme time without seconds and with timezone".
    // QLocale supports only two formats - Long and Short. Long is unusable in many situations
    // and Short does not provide seconds. So if seconds are enabled, we need to add it here.
    //
    // What happens here is that it looks for the delimiter between "h" and "m", takes it
    // and appends it after "mm" and then appends "ss" for the seconds.
    function timeFormatCorrection(timeFormatString) {
        var regexp = /(hh*)(.+)(mm)/i
        var match = regexp.exec(timeFormatString);

        var hours = match[1];
        var delimiter = match[2];
        var minutes = match[3]
        var seconds = "ss";
        var amPm = "AP";
        var uses24hFormatByDefault = timeFormatString.toLowerCase().indexOf("ap") === -1;

        // because QLocale is incredibly stupid and does not convert 12h/24h clock format
        // when uppercase H is used for hours, needs to be h or hh, so toLowerCase()
        var result = hours.toLowerCase() + delimiter + minutes;

        if (main.showSeconds) {
            result += delimiter + seconds;
        }

        // add "AM/PM" either if the setting is the default and locale uses it OR if the user unchecked "use 24h format"
        if ((main.use24hFormat == Qt.PartiallyChecked && !uses24hFormatByDefault) || main.use24hFormat == Qt.Unchecked) {
            result += " " + amPm;
        }

        main.timeFormat = result;
    }

    function dateTimeChanged()
    {
        var doCorrections = false;

        if (main.showDate) {
            // If the date has changed, force size recalculation, because the day name
            // or the month name can now be longer/shorter, so we need to adjust applet size
            var currentDate = Qt.formatDateTime(dataSource.data["Local"]["DateTime"], "yyyy-mm-dd");
            if (main.lastDate !== currentDate) {
                doCorrections = true;
                main.lastDate = currentDate
            }
        }

        var currentTZOffset = dataSource.data["Local"]["Offset"] / 60;
        if (currentTZOffset !== tzOffset) {
            doCorrections = true;
            tzOffset = currentTZOffset;
            Date.timeZoneUpdated(); // inform the QML JS engine about TZ change
        }

        if (doCorrections) {
            timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat));
        }
    }

    function setTimezoneIndex() {
        for (var i = 0; i < plasmoid.configuration.selectedTimeZones.length; i++) {
            if (plasmoid.configuration.selectedTimeZones[i] === plasmoid.configuration.lastSelectedTimezone) {
                main.tzIndex = i;
                break;
            }
        }
    }

    Component.onCompleted: {
        // Sort the timezones according to their offset
        // Calling sort() directly on plasmoid.configuration.selectedTimeZones
        // has no effect, so sort a copy and then assign the copy to it
        var sortArray = plasmoid.configuration.selectedTimeZones;
        sortArray.sort(function(a, b) {
            return dataSource.data[a]["Offset"] - dataSource.data[b]["Offset"];
        });
        plasmoid.configuration.selectedTimeZones = sortArray;

        setTimezoneIndex();
        tzOffset = -(new Date().getTimezoneOffset());
        dateTimeChanged();
        timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat));
        dataSource.onDataChanged.connect(dateTimeChanged);
    }
}
