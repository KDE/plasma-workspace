/*
 * Copyright 2013 Heena Mahour <heena393@gmail.com>
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 * Copyright 2013 Martin Klapetek <mklapetek@kde.org>
 * Copyright 2014 David Edmundson <davidedmundson@kde.org>
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

import QtQuick 2.2
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.plasma.private.digitalclock 1.0

Item {
    id: main

    property string timeFormat
    property date currentTime

    property bool showSeconds: plasmoid.configuration.showSeconds
    property bool showLocalTimezone: plasmoid.configuration.showLocalTimezone
    property bool showDate: plasmoid.configuration.showDate
    property int dateFormat: plasmoid.configuration.dateFormat == "longDate" ? Locale.LongFormat : Locale.ShortFormat

    property string lastSelectedTimezone: plasmoid.configuration.lastSelectedTimezone
    property bool displayTimezoneAsCode: plasmoid.configuration.displayTimezoneAsCode

    property string lastDate: ""
    property int tzOffset

    // This is the index in the list of user selected timezones
    property int tzIndex: 0

    onDisplayTimezoneAsCodeChanged: { setupLabels(); }
    onStateChanged: { setupLabels(); }

    onLastSelectedTimezoneChanged: { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }
    onShowSecondsChanged:          { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }
    onShowLocalTimezoneChanged:    { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }
    onDateFormatChanged:           { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }
    onShowDateChanged:             { timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)) }

    states: [
        State {
            name: "horizontalPanel"
            when: plasmoid.formFactor == PlasmaCore.Types.Horizontal

            PropertyChanges {
                target: main
                Layout.fillHeight: true
                Layout.fillWidth: false
                Layout.minimumWidth: Math.max(main.showDate ? sizehelper.paintedWidth + timezoneLabel.paintedWidth : Math.max(sizehelper.paintedWidth, timezoneLabel.paintedWidth), dateLabel.width)
                Layout.maximumWidth: Layout.minimumWidth

            }

            PropertyChanges {
                target: timeLabel

                height: sizehelper.height
                width: timeLabel.paintedWidth

                wrapMode: Text.NoWrap
                fontSizeMode: Text.VerticalFit
            }

            PropertyChanges {
                target: timezoneLabel

                height: main.showDate ? timeLabel.height : Math.round(2 * (timeLabel.height / 3))
                width: main.showDate ? timezoneLabel.paintedWidth : timeLabel.width

                fontSizeMode: Text.VerticalFit
                minimumPixelSize: 0
                elide: Text.ElideNone
                horizontalAlignment: Text.AlignHCenter
            }

            PropertyChanges {
                target: dateLabel

                height: Math.round(2 * (main.height / 5))
                width: dateLabel.paintedWidth

                anchors.horizontalCenter: main.horizontalCenter
            }

            PropertyChanges {
                target: labelsFlow

                flow: main.showDate ? Flow.LeftToRight : Flow.TopToBottom
            }

            PropertyChanges {
                target: sizehelper

                height: main.showDate || timezoneLabel.visible ?  Math.round(3 * (main.height / 5)) : main.height
                width: sizehelper.paintedWidth

                fontSizeMode: Text.VerticalFit
            }
        },

        State {
            name: "verticalPanel"
            when: plasmoid.formFactor == PlasmaCore.Types.Vertical

            PropertyChanges {
                target: main
                Layout.fillHeight: false
                Layout.fillWidth: true
                Layout.maximumHeight: main.showDate ? labelsFlow.height + dateLabel.height : labelsFlow.height
                Layout.minimumHeight: Layout.maximumHeight
            }

            PropertyChanges {
                target: timeLabel

                height: sizehelper.paintedHeight
                width: main.width

                fontSizeMode: Text.VerticalFit
                wrapMode: Text.WordWrap
            }

            PropertyChanges {
                target: timezoneLabel

                height: Math.max(sizehelper.lineCount > 1 ?  2 * Math.round(timeLabel.height / 6) : 2 * Math.round(timeLabel.height / 3), theme.smallestFont.pixelSize)
                width: main.width

                fontSizeMode: Text.VerticalFit
                minimumPixelSize: theme.smallestFont.pixelSize
                elide: Text.ElideRight
            }

            PropertyChanges {
                target: dateLabel

                height: timezoneLabel.height
                width: timezoneLabel.width

                minimumPixelSize: theme.smallestFont.pixelSize
                elide: Text.ElideRight
            }

            PropertyChanges {
                target: sizehelper

                height: sizehelper.paintedHeight
                width: main.width

                fontSizeMode: Text.HorizontalFit
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        },

        State {
            name: "other"
            when: plasmoid.formFactor != PlasmaCore.Types.Vertical && plasmoid.formFactor != PlasmaCore.Types.Horizontal

            PropertyChanges {
                target: main
                Layout.fillHeight: false
                Layout.fillWidth: false
                Layout.minimumWidth: units.gridUnit * 3
                Layout.maximumWidth: Math.max(labelsFlow.width, dateLabel.width)
                Layout.minimumHeight: units.gridUnit * 3
                Layout.maximumHeight: main.showDate ? labelsFlow.height + dateLabel.height : labelsFlow.height
            }

            PropertyChanges {
                target: timeLabel

                height: sizehelper.height
                width: main.width

                wrapMode: Text.NoWrap
                fontSizeMode: Text.Fit
            }

            PropertyChanges {
                target: timezoneLabel

                height: dateLabel.visible ? Math.round(1 * (main.height / 5)) : Math.round(2 * (main.height / 5))
                width: main.width

                fontSizeMode: Text.Fit
                minimumPixelSize: 0
                elide: Text.ElideRight
            }

            PropertyChanges {
                target: dateLabel

                height: timezoneLabel.visible ? Math.round(1 * (main.height / 5)) : Math.round(2 * (main.height / 5))
                width: main.width

                anchors.horizontalCenter: main.horizontalCenter
                fontSizeMode: Text.Fit
            }

            PropertyChanges {
                target: sizehelper

                height: main.showDate || timezoneLabel.visible ?  Math.round(3 * (main.height / 5)) : main.height
                width: main.width

                fontSizeMode: Text.Fit
            }
        }

    ]

    MouseArea {
        id: mouseArea

        anchors.fill: parent

        onClicked: plasmoid.expanded = !plasmoid.expanded

        onWheel: {
            if (!plasmoid.configuration.wheelChangesTimezone) {
                return;
            }

            var delta = wheel.angleDelta.y || wheel.angleDelta.x
            var newIndex = main.tzIndex;

            if (delta < 0) {
                newIndex--;
            } else if (delta > 0) {
                newIndex++;
            }

            if (newIndex >= plasmoid.configuration.selectedTimeZones.length) {
                newIndex = 0;
            } else if (newIndex < 0) {
                newIndex = plasmoid.configuration.selectedTimeZones.length - 1;
            }

            plasmoid.configuration.lastSelectedTimezone = plasmoid.configuration.selectedTimeZones[newIndex];
            main.tzIndex = newIndex;

            dataSource.dataChanged();
            setupLabels();
        }
    }

    Flow {
        id: labelsFlow

        anchors.horizontalCenter: main.horizontalCenter

        flow: Flow.TopToBottom
        spacing: flow == Flow.LeftToRight && timezoneLabel.visible ? units.smallSpacing : 0

        Components.Label  {
            id: timeLabel

            font {
                weight: plasmoid.configuration.boldText ? Font.Bold : Font.Normal
                italic: plasmoid.configuration.italicText
                pixelSize: 1024
                pointSize: 1024
            }

            text: {
                // get the time for the given timezone from the dataengine
                var now = dataSource.data[plasmoid.configuration.lastSelectedTimezone]["DateTime"];
                // get current UTC time
                var msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
                // add the dataengine TZ offset to it
                var currentTime = new Date(msUTC + (dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Offset"] * 1000));

                main.currentTime = currentTime;
                return Qt.formatTime(currentTime, main.timeFormat);
            }

            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }

        Components.Label {
            id: timezoneLabel

            font.weight: timeLabel.font.weight
            font.italic: timeLabel.font.italic
            font.pixelSize: 1024
            font.pointSize: 1024
            minimumPixelSize: 0

            visible: text.length > 0
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Components.Label {
        id: dateLabel

        anchors.top: labelsFlow.bottom
        visible: main.showDate

        font.weight: timeLabel.font.weight
        font.italic: timeLabel.font.italic
        font.pixelSize: 1024
        font.pointSize: 1024
        minimumPixelSize: 0

        fontSizeMode: Text.VerticalFit

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Components.Label {
        id: sizehelper

        font.weight: timeLabel.font.weight
        font.italic: timeLabel.font.italic
        font.pixelSize: 1024
        font.pointSize: 1024

        verticalAlignment: Text.AlignVCenter

        visible: false
    }

    // Qt's QLocale does not offer any modular time creating like Klocale did
    // eg. no "gimme time with seconds" or "gimme time without seconds and with timezone".
    // QLocale supports only two formats - Long and Short. Long is unusable in many situations
    // and Short does not provide seconds. So if seconds are enabled, we need to add it here.
    //
    // What happens here is that it looks for the delimiter between "h" and "m", takes it
    // and appends it after "mm" and then appends "ss" for the seconds. Also it checks
    // if the format string already does not contain the seconds part.
    //
    // It can happen that Qt uses the 'C' locale (it's a fallback) and that locale
    // has always ":ss" part in ShortFormat, so we need to remove it.
    function timeFormatCorrection(timeFormatString) {
        if (main.showSeconds && timeFormatString.indexOf('s') == -1) {
            timeFormatString = timeFormatString.replace(/^(hh*)(.+)(mm)(.*?)/i,
                                                        function(match, firstPart, delimiter, secondPart, rest, offset, original) {
                return firstPart + delimiter + secondPart + delimiter + "ss" + rest
            });
        } else if (!main.showSeconds && timeFormatString.indexOf('s') != -1) {
            timeFormatString = timeFormatString.replace(/.ss?/i, "");
        }

        main.timeFormat = timeFormatString;
        setupLabels();
    }

    function setupLabels() {
        var st = Qt.formatTime(new Date(2000, 0, 1, 22, 0, 0), main.timeFormat);
        var showTimezone = main.showLocalTimezone || (plasmoid.configuration.lastSelectedTimezone != "Local"
                                                        && dataSource.data["Local"]["Timezone City"] != dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Timezone City"]);

        var timezoneString = "";

        if (showTimezone) {
            timezoneString = plasmoid.configuration.displayTimezoneAsCode ? dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Timezone Abbreviation"]
                                                                          : TimezonesI18n.i18nCity(dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Timezone City"]);
            timezoneLabel.text = main.showDate && plasmoid.formFactor == PlasmaCore.Types.Horizontal ? "(" + timezoneString + ")" : timezoneString;
        } else {
            // this clears the label and that makes it hidden
            timezoneLabel.text = timezoneString;
        }


        if (main.showDate) {
            dateLabel.text = Qt.formatDate(main.currentTime, Qt.locale().dateFormat(main.dateFormat));
        } else {
            // clear it so it doesn't take space in the layout
            dateLabel.text = "";
        }

        if (sizehelper.text != st) {
            sizehelper.text = st;
        }
    }

    function updateToolTip() {
        var timezoneString = Qt.formatDate(dataSource.data["Local"]["DateTime"], dateFormatString);
        if (plasmoid.configuration.selectedTimeZones.length > 1) {
            timezoneString += "<br />";
            for (var i = 0; i < plasmoid.configuration.selectedTimeZones.length; ++i) {
                timezoneString += "<br />" + timeForZone(i, false);
            }
        }
        plasmoid.toolTipSubText = timezoneString;
    }

    function timeForZone(zone, addlinebreaks) {
        var returnString = "";

        // get the time for the given timezone from the dataengine
        var now = dataSource.data[plasmoid.configuration.selectedTimeZones[zone]]["DateTime"];
        // get current UTC time
        var msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
        // add the dataengine TZ offset to it
        var dateTime = new Date(msUTC + (dataSource.data[plasmoid.configuration.selectedTimeZones[zone]]["Offset"] * 1000));

        // add the timezone string to the clock
        if (plasmoid.configuration.selectedTimeZones[zone] != "Local" || main.showLocalTimezone) {
            var timezoneString = plasmoid.configuration.displayTimezoneAsCode ? dataSource.data[plasmoid.configuration.selectedTimeZones[zone]]["Timezone Abbreviation"]
                                                                              : TimezonesI18n.i18nCity(dataSource.data[plasmoid.configuration.selectedTimeZones[zone]]["Timezone City"]);

            if (addlinebreaks) {
                returnString += (showDate ? i18nc("This composes time and a timezone into one string that's displayed in the clock applet (the main clock in the panel). "
                                                + "%1 is the current time and %2 is either the timezone city or timezone code, depending on user settings",
                                                "%1 (%2)", Qt.formatTime(dateTime, main.timeFormat), timezoneString)
                                        : i18nc("This composes time and a timezone into one string that's displayed in the clock applet (the main clock in the panel). "
                                                + "From the previous case it's different that it puts the timezone name into separate new line without using ()s",
                                                "%1<br/>%2", Qt.formatTime(dateTime, main.timeFormat), timezoneString));
            } else {
                // Without line breaks, and put the timezone first
                returnString += i18nc("This composes time and a timezone into one string that's displayed in the tooltip of the clock. "
                                      + "%1 is the timezone city or timezone code depending on user settings and %2 is the current time",
                                      "%1 %2", timezoneString, Qt.formatTime(dateTime, main.timeFormat));
            }
        } else {
            // return only the time
            returnString += Qt.formatTime(dateTime, main.timeFormat);
        }

        if (showDate) {
            returnString += (addlinebreaks ? "<br/>" + Qt.formatDate(dateTime, Qt.locale().dateFormat(main.dateFormat))
                                           : " " + Qt.formatDate(dateTime, Qt.locale().dateFormat(main.dateFormat)));
        }

        return returnString;
    }

    function dateTimeChanged()
    {
        var doCorrections = false;

        if (main.showDate) {
            // If the date has changed, force size recalculation, because the day name
            // or the month name can now be longer/shorter, so we need to adjust applet size
            var currentDate = Qt.formatDateTime(dataSource.data["Local"]["DateTime"], "yyyy-mm-dd");
            if (main.lastDate != currentDate) {
                doCorrections = true;
                main.lastDate = currentDate
            }
        }

        var currentTZOffset = dataSource.data["Local"]["Offset"] / 60;
        if (currentTZOffset != tzOffset) {
            doCorrections = true;
            tzOffset = currentTZOffset;
            Date.timeZoneUpdated(); // inform the QML JS engine about TZ change
        }

        if (doCorrections) {
            timeFormatCorrection(main.timeFormat);
        }
    }

    Component.onCompleted: {
        for (var i = 0; i < plasmoid.configuration.selectedTimeZones.length; i++) {
            if (plasmoid.configuration.selectedTimeZones[i] == plasmoid.configuration.lastSelectedTimezone) {
                main.tzIndex = i;
                break;
            }
        }

        tzOffset = new Date().getTimezoneOffset();
        dateTimeChanged();
        timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat));
        updateToolTip();
        dataSource.onDataChanged.connect(dateTimeChanged);
        dataSource.onDataChanged.connect(updateToolTip);
    }
}
