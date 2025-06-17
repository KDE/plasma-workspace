/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.private.digitalclock
import org.kde.kirigami as Kirigami

MouseArea {
    id: main
    objectName: "digital-clock-compactrepresentation"

    property string timeFormat
    property string timeFormatWithSeconds

    // This is quite convoluted in Qt 6:
    // Qt.formatDate with locale only accepts Locale.FormatType as format type,
    // no Qt.DateFormat (ISODate) and no format string.
    // Locale.toString on the other hand only formats a date *with* time...
    readonly property var dateFormatter: {
        if (Plasmoid.configuration.dateFormat === "custom") {
            Plasmoid.configuration.customDateFormat; // create a binding dependency on this property.
            return (d) => {
                return Qt.locale().toString(d, Plasmoid.configuration.customDateFormat);
            };
        } else if (Plasmoid.configuration.dateFormat === "isoDate") {
            return (d) => {
                return Qt.formatDate(d, Qt.ISODate);
            };
        } else if (Plasmoid.configuration.dateFormat === "longDate") {
            return (d) => {
                return Qt.formatDate(d, Qt.locale(), Locale.LongFormat);
            };
        } else {
            return (d) => {
                return Qt.formatDate(d, Qt.locale(), Locale.ShortFormat);
            };
        }
    }

    property string lastDate: ""
    property int tzOffset

    // This is the index in the list of user selected time zones
    property int tzIndex: 0

    // if showing the date and the time in one line or
    // if the date/time zone cannot be fit with the smallest font to its designated space
    readonly property bool oneLineMode: {
        if (Plasmoid.configuration.dateDisplayFormat === 1) {
            // BesideTime
            return true;
        } else if (Plasmoid.configuration.dateDisplayFormat === 2) {
            // BelowTime
            return false;
        } else {
            // Adaptive
            return Plasmoid.formFactor === PlasmaCore.Types.Horizontal &&
                height <= 2 * Kirigami.Theme.smallFont.pixelSize &&
                (Plasmoid.configuration.showDate || timeZoneLabel.visible);
        }
    }

    property bool wasExpanded
    property int wheelDelta: 0

    Accessible.role: Accessible.Button
    Accessible.onPressAction: clicked(null)

    Connections {
        target: Plasmoid
        function onContextualActionsAboutToShow() {
            ClipboardMenu.secondsIncluded = (Plasmoid.configuration.showSeconds === 2);
            ClipboardMenu.currentDate = main.getCurrentTime();
        }
    }

    Connections {
        target: Plasmoid.configuration
        function onSelectedTimeZonesChanged() {
            // If the currently selected time zone was removed,
            // default to the first one in the list
            if (Plasmoid.configuration.selectedTimeZones.indexOf(Plasmoid.configuration.lastSelectedTimezone) === -1) {
                Plasmoid.configuration.lastSelectedTimezone = Plasmoid.configuration.selectedTimeZones[0];
            }

            main.setupLabels();
            main.setTimeZoneIndex();
        }

        function onDisplayTimezoneFormatChanged() {
            main.setupLabels();
        }

        function onLastSelectedTimezoneChanged() {
            main.timeFormatCorrection();
        }

        function onShowLocalTimezoneChanged() {
            main.timeFormatCorrection();
        }

        function onShowDateChanged() {
            main.timeFormatCorrection();
        }

        function onUse24hFormatChanged() {
            main.timeFormatCorrection();
        }
    }

    function getCurrentTime(): date {
        const data = dataSource.data[Plasmoid.configuration.lastSelectedTimezone];
        // The order of signal propagation is unspecified, so we might get
        // here before the dataSource has updated. Alternatively, a buggy
        // configuration view might set lastSelectedTimezone to a new time
        // zone before applying the new list, or it may just be set to
        // something invalid in the config file.
        if (data === undefined) {
            return new Date();
        }

        // get the time for the given time zone from the dataengine
        const now = data["DateTime"];
        // get current UTC time
        const msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
        // add the dataengine TZ offset to it
        const currentTime = new Date(msUTC + (data["Offset"] * 1000));
        return currentTime;
    }

    function pointToPixel(pointSize: int): int {
        const pixelsPerInch = Screen.pixelDensity * 25.4
        return Math.round(pointSize / 72 * pixelsPerInch)
    }

    states: [
        State {
            name: "horizontalPanel"
            when: Plasmoid.formFactor === PlasmaCore.Types.Horizontal && !main.oneLineMode

            PropertyChanges {
                target: main
                Layout.fillHeight: true
                Layout.fillWidth: false
                Layout.minimumWidth: contentItem.width
                Layout.maximumWidth: Layout.minimumWidth
            }

            PropertyChanges {
                target: contentItem

                height: timeLabel.height + (Plasmoid.configuration.showDate || timeZoneLabel.visible ? 0.8 * timeLabel.height : 0)
                width: Math.max(timeLabel.width + (Plasmoid.configuration.showDate ? timeZoneLabel.paintedWidth : 0),
                                timeZoneLabel.paintedWidth, dateLabel.paintedWidth) + Kirigami.Units.largeSpacing
            }

            PropertyChanges {
                target: labelsGrid

                rows: Plasmoid.configuration.showDate ? 1 : 2
            }

            AnchorChanges {
                target: labelsGrid

                anchors.horizontalCenter: contentItem.horizontalCenter
            }

            PropertyChanges {
                target: timeLabel

                height: sizehelper.height
                width: timeLabel.paintedWidth

                font.pixelSize: timeLabel.height
            }

            PropertyChanges {
                target: timeZoneLabel

                height: Plasmoid.configuration.showDate ? 0.7 * timeLabel.height : 0.8 * timeLabel.height
                width: Plasmoid.configuration.showDate ? timeZoneLabel.paintedWidth : timeLabel.width

                font.pixelSize: timeZoneLabel.height
            }

            PropertyChanges {
                target: dateLabel

                height: 0.8 * timeLabel.height
                width: dateLabel.paintedWidth
                verticalAlignment: Text.AlignVCenter

                font.pixelSize: dateLabel.height
            }

            AnchorChanges {
                target: dateLabel

                anchors.top: labelsGrid.bottom
                anchors.horizontalCenter: labelsGrid.horizontalCenter
            }

            PropertyChanges {
                target: sizehelper

                /*
                 * The value 0.71 was picked by testing to give the clock the right
                 * size (aligned with tray icons).
                 * Value 0.56 seems to be chosen rather arbitrary as well such that
                 * the time label is slightly larger than the date or time zone label
                 * and still fits well into the panel with all the applied margins.
                 */
                height: Math.min(Plasmoid.configuration.showDate || timeZoneLabel.visible ? main.height * 0.56 : main.height * 0.71,
                                 fontHelper.font.pixelSize)

                font.pixelSize: sizehelper.height
            }
        },

        State {
            name: "oneLineDate"
            // the one-line mode has no effect on a vertical panel because it would never fit
            when: Plasmoid.formFactor !== PlasmaCore.Types.Vertical && main.oneLineMode

            PropertyChanges {
                target: main
                Layout.fillHeight: true
                Layout.fillWidth: false
                Layout.minimumWidth: contentItem.width
                Layout.maximumWidth: Layout.minimumWidth

            }

            PropertyChanges {
                target: contentItem

                height: sizehelper.height
                width: (dateLabel.visible ? dateLabel.width + timeMetrics.advanceWidth(" ") * 2 + separator.width : 0) + labelsGrid.width
            }

            AnchorChanges {
                target: labelsGrid

                anchors.right: contentItem.right
            }

            PropertyChanges {
                target: dateLabel

                height: timeLabel.height
                width: dateLabel.paintedWidth

                // to debug font sizing issues
                // onHeightChanged: () => console.log("height", timeMetrics.font.pixelSize, dateLabel.height)
                // onWidthChanged: () => console.log("width", timeMetrics.advanceWidth("26"), dateLabel.width)

                font.pixelSize: 1024
                verticalAlignment: Text.AlignVCenter

                fontSizeMode: Text.VerticalFit
            }

            AnchorChanges {
                target: dateLabel

                anchors.left: contentItem.left
                anchors.verticalCenter: labelsGrid.verticalCenter
            }

            PropertyChanges {
                target: timeLabel

                height: sizehelper.height
                width: timeLabel.paintedWidth

                fontSizeMode: Text.VerticalFit
            }

            PropertyChanges {
                target: timeZoneLabel

                height: 0.7 * timeLabel.height
                width: timeZoneLabel.paintedWidth

                fontSizeMode: Text.VerticalFit
                horizontalAlignment: Text.AlignHCenter
            }

            PropertyChanges {
                target: sizehelper

                height: Math.min(main.height, fontHelper.contentHeight)

                fontSizeMode: Text.VerticalFit
                font.pixelSize: fontHelper.font.pixelSize
            }
        },

        State {
            name: "verticalPanel"
            when: Plasmoid.formFactor === PlasmaCore.Types.Vertical

            PropertyChanges {
                target: main
                Layout.fillHeight: false
                Layout.fillWidth: true
                Layout.maximumHeight: contentItem.height
                Layout.minimumHeight: Layout.maximumHeight
            }

            PropertyChanges {
                target: contentItem

                height: Plasmoid.configuration.showDate ? labelsGrid.height + dateLabel.contentHeight : labelsGrid.height
                width: main.width
            }

            PropertyChanges {
                target: labelsGrid

                rows: 2
            }

            PropertyChanges {
                target: timeLabel

                height: sizehelper.contentHeight
                width: main.width

                font.pixelSize: Math.min(timeLabel.height, fontHelper.font.pixelSize)
                fontSizeMode: Text.Fit
            }

            PropertyChanges {
                target: timeZoneLabel

                height: Math.max(0.7 * timeLabel.height, minimumPixelSize)
                width: main.width

                fontSizeMode: Text.Fit
                minimumPixelSize: dateLabel.minimumPixelSize
                elide: Text.ElideRight
            }

            PropertyChanges {
                target: dateLabel

                width: main.width
                //NOTE: in order for Text.Fit to work as intended, the actual height needs to be quite big, in order for the font to enlarge as much it needs for the available width, and then request a sensible height, for which contentHeight will need to be considered as opposed to height
                height: Kirigami.Units.gridUnit * 10

                fontSizeMode: Text.Fit
                verticalAlignment: Text.AlignTop
                // Those magic numbers are purely what looks nice as maximum size, here we have it the smallest
                // between slightly bigger than the default font (1.4 times) and a bit smaller than the time font
                font.pixelSize: Math.min(0.7 * timeLabel.height, Kirigami.Theme.defaultFont.pixelSize * 1.4)
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
            }

            AnchorChanges {
                target: dateLabel

                anchors.top: labelsGrid.bottom
                anchors.horizontalCenter: labelsGrid.horizontalCenter
            }

            PropertyChanges {
                target: sizehelper

                width: main.width

                fontSizeMode: Text.HorizontalFit
                font.pixelSize: fontHelper.font.pixelSize
            }
        },

        State {
            name: "other"
            when: Plasmoid.formFactor !== PlasmaCore.Types.Vertical && Plasmoid.formFactor !== PlasmaCore.Types.Horizontal

            PropertyChanges {
                target: main
                Layout.fillHeight: false
                Layout.fillWidth: false
                Layout.minimumWidth: Kirigami.Units.gridUnit * 3
                Layout.minimumHeight: Kirigami.Units.gridUnit * 3
            }

            PropertyChanges {
                target: contentItem

                height: main.height
                width: main.width
            }

            PropertyChanges {
                target: labelsGrid

                rows: 2
            }

            PropertyChanges {
                target: timeLabel

                height: sizehelper.height
                width: main.width

                fontSizeMode: Text.Fit
            }

            PropertyChanges {
                target: timeZoneLabel

                height: 0.7 * timeLabel.height
                width: main.width

                fontSizeMode: Text.Fit
                minimumPixelSize: 1
            }

            PropertyChanges {
                target: dateLabel

                height: 0.7 * timeLabel.height
                font.pixelSize: 1024
                width: Math.max(timeLabel.contentWidth, Kirigami.Units.gridUnit * 3)
                verticalAlignment: Text.AlignVCenter

                fontSizeMode: Text.Fit
                minimumPixelSize: 1
                wrapMode: Text.WordWrap
            }

            AnchorChanges {
                target: dateLabel

                anchors.top: labelsGrid.bottom
                anchors.horizontalCenter: labelsGrid.horizontalCenter
            }

            PropertyChanges {
                target: sizehelper

                height: {
                    if (Plasmoid.configuration.showDate) {
                        if (timeZoneLabel.visible) {
                            return 0.4 * main.height
                        }
                        return 0.56 * main.height
                    } else if (timeZoneLabel.visible) {
                        return 0.59 * main.height
                    }
                    return main.height
                }
                width: main.width

                fontSizeMode: Text.Fit
                font.pixelSize: 1024
            }
        }
    ]

    onPressed: wasExpanded = root.expanded
    onClicked: root.expanded = !wasExpanded
    onWheel: wheel => {
        if (!Plasmoid.configuration.wheelChangesTimezone) {
            return;
        }

        var delta = (wheel.inverted ? -1 : 1) * (wheel.angleDelta.y ? wheel.angleDelta.y : wheel.angleDelta.x);
        var newIndex = tzIndex;
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

        if (newIndex >= Plasmoid.configuration.selectedTimeZones.length) {
            newIndex = 0;
        } else if (newIndex < 0) {
            newIndex = Plasmoid.configuration.selectedTimeZones.length - 1;
        }

        if (newIndex !== tzIndex) {
            Plasmoid.configuration.lastSelectedTimezone = Plasmoid.configuration.selectedTimeZones[newIndex];
            tzIndex = newIndex;

            dataSource.dataChanged();
            setupLabels();
        }
    }

   /*
    * Visible elements
    *
    */
    Item {
        id: contentItem
        anchors.verticalCenter: main.verticalCenter

        Grid {
            id: labelsGrid

            rows: 1
            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter

            flow: Grid.TopToBottom
            // between time and timezone; timezone is styled differently, so
            // smallSpacing is more appropriate than a space
            columnSpacing: Kirigami.Units.smallSpacing

            PlasmaComponents.Label  {
                id: timeLabel

                font {
                    family: fontHelper.font.family
                    weight: fontHelper.font.weight
                    italic: fontHelper.font.italic
                    features: { "tnum": 1 }
                    pixelSize: 1024
                }
                minimumPixelSize: 1

                text: Qt.formatTime(main.getCurrentTime(), Plasmoid.configuration.showSeconds === 2 ? main.timeFormatWithSeconds : main.timeFormat)
                textFormat: Text.PlainText

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }

            PlasmaComponents.Label {
                id: timeZoneLabel

                font.weight: timeLabel.font.weight
                font.italic: timeLabel.font.italic
                font.pixelSize: 1024
                minimumPixelSize: 1

                visible: text.length > 0
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
        }

        Rectangle {
            id: separator

            property bool isOneLineMode: main.state == "oneLineDate"

            height: timeLabel.height * 0.8
            width: timeLabel.height / 16
            radius: width / 2
            color: Kirigami.Theme.textColor

            anchors.leftMargin: timeMetrics.advanceWidth(" ") + width / 2
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: dateLabel.right

            visible: isOneLineMode && Plasmoid.configuration.showDate
        }

        PlasmaComponents.Label {
            id: dateLabel

            visible: Plasmoid.configuration.showDate

            font.family: timeLabel.font.family
            font.weight: timeLabel.font.weight
            font.italic: timeLabel.font.italic
            font.pixelSize: 1024
            minimumPixelSize: 1

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            textFormat: Text.PlainText
        }
    }
    /*
     * end: Visible Elements
     *
     */

    PlasmaComponents.Label {
        id: sizehelper

        font.family: timeLabel.font.family
        font.weight: timeLabel.font.weight
        font.italic: timeLabel.font.italic
        minimumPixelSize: 1

        visible: false
        textFormat: Text.PlainText
    }

    // To measure Label.height for maximum-sized font in VerticalFit mode
    PlasmaComponents.Label {
        id: fontHelper

        height: 1024

        font.family: (Plasmoid.configuration.autoFontAndSize || Plasmoid.configuration.fontFamily.length === 0) ? Kirigami.Theme.defaultFont.family : Plasmoid.configuration.fontFamily
        font.weight: Plasmoid.configuration.autoFontAndSize ? Kirigami.Theme.defaultFont.weight : Plasmoid.configuration.fontWeight
        font.italic: Plasmoid.configuration.autoFontAndSize ? Kirigami.Theme.defaultFont.italic : Plasmoid.configuration.italicText
        font.pixelSize: Plasmoid.configuration.autoFontAndSize ? 3 * Kirigami.Theme.defaultFont.pixelSize : pointToPixel(Plasmoid.configuration.fontSize)
        fontSizeMode: Text.VerticalFit

        visible: false
        textFormat: Text.PlainText
    }

    FontMetrics {
        id: timeMetrics

        font.family: timeLabel.font.family
        font.weight: timeLabel.font.weight
        font.italic: timeLabel.font.italic

        font.pixelSize: dateLabel.contentHeight
    }

    // Qt's QLocale does not offer any modular time creating like Klocale did
    // eg. no "gimme time with seconds" or "gimme time without seconds and with time zone".
    // QLocale supports only two formats - Long and Short. Long is unusable in many situations
    // and Short does not provide seconds. So if seconds are enabled, we need to add it here.
    //
    // What happens here is that it looks for the delimiter between "h" and "m", takes it
    // and appends it after "mm" and then appends "ss" for the seconds.
    function timeFormatCorrection(timeFormatString = Qt.locale().timeFormat(Locale.ShortFormat)) {
        const regexp = /(hh*)(.+)(mm)/i
        const match = regexp.exec(timeFormatString);

        const hours = match[1];
        const delimiter = match[2];
        const minutes = match[3]
        const seconds = "ss";
        const amPm = "AP";
        const uses24hFormatByDefault = timeFormatString.toLowerCase().indexOf("ap") === -1;

        // because QLocale is incredibly stupid and does not convert 12h/24h clock format
        // when uppercase H is used for hours, needs to be h or hh, so toLowerCase()
        let result = hours.toLowerCase() + delimiter + minutes;

        let result_sec = result + delimiter + seconds;

        // add "AM/PM" either if the setting is the default and locale uses it OR if the user unchecked "use 24h format"
        if ((Plasmoid.configuration.use24hFormat === Qt.PartiallyChecked && !uses24hFormatByDefault) || Plasmoid.configuration.use24hFormat === Qt.Unchecked) {
            result += " " + amPm;
            result_sec += " " + amPm;
        }

        timeFormat = result;
        timeFormatWithSeconds = result_sec;
        setupLabels();
    }

    function setupLabels() {
        const lastSelectedData = dataSource.data[Plasmoid.configuration.lastSelectedTimezone];
        const localData = dataSource.data["Local"];
        // The order of signal propagation is unspecified, so we might get
        // here before the dataSource has updated. Alternatively, a buggy
        // configuration view might set lastSelectedTimezone to a new time
        // zone before applying the new list, or it may just be set to
        // something invalid in the config file.
        if (lastSelectedData === undefined || localData === undefined) {
            return;
        }

        const showTimezone = Plasmoid.configuration.showLocalTimezone
            || (Plasmoid.configuration.lastSelectedTimezone !== "Local"
                && lastSelectedData["Timezone City"] !== localData["Timezone City"]);

        let timezoneString = "";

        if (showTimezone) {
            // format time zone as tz code, city or UTC offset
            switch (Plasmoid.configuration.displayTimezoneFormat) {
            case 0: // Code
                timezoneString = lastSelectedData["Timezone Abbreviation"]
                break;
            case 1: // City
                timezoneString = TimeZonesI18n.i18nCity(lastSelectedData["Timezone"]);
                break;
            case 2: // Offset from UTC time
                const lastOffset = lastSelectedData["Offset"];
                const symbol = lastOffset > 0 ? '+' : '';
                const hours = Math.floor(lastOffset / 3600);
                const minutes = Math.floor(lastOffset % 3600 / 60);

                timezoneString = "UTC" + symbol + hours.toString().padStart(2, '0') + ":" + minutes.toString().padStart(2, '0');
                break;
            }
            if ((Plasmoid.configuration.showDate || oneLineMode) && Plasmoid.formFactor === PlasmaCore.Types.Horizontal) {
                timezoneString = `(${timezoneString})`;
            }
        }
        // an empty string clears the label and that makes it hidden
        timeZoneLabel.text = timezoneString;

        if (Plasmoid.configuration.showDate) {
            dateLabel.text = dateFormatter(getCurrentTime());
        } else {
            // clear it so it doesn't take space in the layout
            dateLabel.text = "";
        }

        // find widest character between 0 and 9
        let maximumWidthNumber = 0;
        let maximumAdvanceWidth = 0;
        for (let i = 0; i <= 9; i++) {
            const advanceWidth = timeMetrics.advanceWidth(i);
            if (advanceWidth > maximumAdvanceWidth) {
                maximumAdvanceWidth = advanceWidth;
                maximumWidthNumber = i;
            }
        }
        // replace all placeholders with the widest number (two digits)
        const format = timeFormat.replace(/(h+|m+|s+)/g, "" + maximumWidthNumber + maximumWidthNumber); // make sure maximumWidthNumber is formatted as string
        // build the time string twice, once with an AM time and once with a PM time
        const date = new Date(2000, 0, 1, 1, 0, 0);
        const timeAm = Qt.formatTime(date, format);
        const advanceWidthAm = timeMetrics.advanceWidth(timeAm);
        date.setHours(13);
        const timePm = Qt.formatTime(date, format);
        const advanceWidthPm = timeMetrics.advanceWidth(timePm);
        // set the sizehelper's text to the widest time string
        if (advanceWidthAm > advanceWidthPm) {
            sizehelper.text = timeAm;
        } else {
            sizehelper.text = timePm;
        }
        fontHelper.text = sizehelper.text
    }

    function dateTimeChanged() {
        let doCorrections = false;

        if (Plasmoid.configuration.showDate) {
            // If the date has changed, force size recalculation, because the day name
            // or the month name can now be longer/shorter, so we need to adjust applet size
            const currentDate = Qt.formatDateTime(getCurrentTime(), "yyyy-MM-dd");
            if (lastDate !== currentDate) {
                doCorrections = true;
                lastDate = currentDate
            }
        }

        const currentTimeZoneOffset = dataSource.data["Local"]["Offset"] / 60;
        if (currentTimeZoneOffset !== tzOffset) {
            doCorrections = true;
            tzOffset = currentTimeZoneOffset;
            Date.timeZoneUpdated(); // inform the QML JS engine about TZ change
        }

        if (doCorrections) {
            timeFormatCorrection();
        }
    }

    function setTimeZoneIndex() {
        tzIndex = Plasmoid.configuration.selectedTimeZones.indexOf(Plasmoid.configuration.lastSelectedTimezone);
    }

    Component.onCompleted: {
        // Sort the time zones according to their offset
        // Calling sort() directly on Plasmoid.configuration.selectedTimeZones
        // has no effect, so sort a copy and then assign the copy to it
        const byOffset = (a, b) => a.offset - b.offset;
        const sortedTimeZones = Plasmoid.configuration.selectedTimeZones
            .map(timeZone => ({
                timeZone,
                // If not found, move it to the bottom by giving it the highest offset as a fallback
                offset: dataSource.data[timeZone]?.["Offset"] ?? 86400,
            }));
        sortedTimeZones.sort(byOffset);
        Plasmoid.configuration.selectedTimeZones = sortedTimeZones
            .map(({ timeZone }) => timeZone);

        setTimeZoneIndex();
        tzOffset = -(new Date().getTimezoneOffset());
        dateTimeChanged();
        timeFormatCorrection();

        dataSource.dataChanged
            .connect(dateTimeChanged);

        dateFormatterChanged
            .connect(setupLabels);

        stateChanged
            .connect(setupLabels);
    }
}
