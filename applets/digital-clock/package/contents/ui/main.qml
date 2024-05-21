/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.private.digitalclock
import org.kde.kirigami as Kirigami
import org.kde.config as KConfig
import org.kde.kcmutils as KCMUtils

PlasmoidItem {
    id: root

    width: Kirigami.Units.gridUnit * 10
    height: Kirigami.Units.gridUnit * 4

    Plasmoid.backgroundHints: PlasmaCore.Types.ShadowBackground | PlasmaCore.Types.ConfigurableBackground

    readonly property string dateFormatString: setDateFormatString()

    readonly property date currentDateTimeInSelectedTimeZone: {
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
        const nowUtcMilliseconds = now.getTime() + (now.getTimezoneOffset() * 60000);
        const selectedTimeZoneOffsetMilliseconds = data["Offset"] * 1000;
        // add the selected time zone's offset to it
        return new Date(nowUtcMilliseconds + selectedTimeZoneOffsetMilliseconds);
    }

    function initTimeZones() {
        const timeZones = [];
        if (Plasmoid.configuration.selectedTimeZones.indexOf("Local") === -1) {
            timeZones.push("Local");
        }
        root.allTimeZones = timeZones.concat(Plasmoid.configuration.selectedTimeZones);
    }

    function timeForZone(timeZone: string, showSeconds: bool): string {
        if (!compactRepresentationItem) {
            return "";
        }

        const data = dataSource.data[timeZone];
        if (data === undefined) {
            return "";
        }

        // get the time for the given time zone from the dataengine
        const now = data["DateTime"];
        // get current UTC time
        const msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
        // add the dataengine TZ offset to it
        const dateTime = new Date(msUTC + (data["Offset"] * 1000));

        let formattedTime;
        if (showSeconds) {
            formattedTime = Qt.formatTime(dateTime, compactRepresentationItem.timeFormatWithSeconds);
        } else {
            formattedTime = Qt.formatTime(dateTime, compactRepresentationItem.timeFormat);
        }

        if (dateTime.getDay() !== dataSource.data["Local"]["DateTime"].getDay()) {
            formattedTime += " (" + compactRepresentationItem.dateFormatter(dateTime) + ")";
        }

        return formattedTime;
    }

    function displayStringForTimeZone(timeZone: string): string {
        const data = dataSource.data[timeZone];
        if (data === undefined) {
            return timeZone;
        }

        // add the time zone string to the clock
        if (Plasmoid.configuration.displayTimezoneAsCode) {
            return data["Timezone Abbreviation"];
        } else {
            return TimeZonesI18n.i18nCity(data["Timezone"]);
        }
    }

    function selectedTimeZonesDeduplicatingExplicitLocalTimeZone():/* [string] */var {
        const displayStringForLocalTimeZone = displayStringForTimeZone("Local");
        /*
         * Don't add this item if it's the same as the local time zone, which
         * would indicate that the user has deliberately added a dedicated entry
         * for the city of their normal time zone. This is not an error condition
         * because the user may have done this on purpose so that their normal
         * local time zone shows up automatically while they're traveling and
         * they've switched the current local time zone to something else. But
         * with this use case, when they're back in their normal local time zone,
         * the clocks list would show two entries for the same city. To avoid
         * this, let's suppress the duplicate.
         */
        const isLiterallyLocalOrResolvesToSomethingOtherThanLocal = timeZone =>
            timeZone === "Local" || displayStringForTimeZone(timeZone) !== displayStringForLocalTimeZone;

        return Plasmoid.configuration.selectedTimeZones
            .filter(isLiterallyLocalOrResolvesToSomethingOtherThanLocal);
    }

    function timeZoneResolvesToLastSelectedTimeZone(timeZone: string): bool {
        return timeZone === Plasmoid.configuration.lastSelectedTimezone
            || displayStringForTimeZone(timeZone) === displayStringForTimeZone(Plasmoid.configuration.lastSelectedTimezone);
    }

    preferredRepresentation: compactRepresentation
    compactRepresentation: DigitalClock {
        activeFocusOnTab: true
        hoverEnabled: true

        Accessible.name: tooltipLoader.item.Accessible.name
        Accessible.description: tooltipLoader.item.Accessible.description
    }
    fullRepresentation: CalendarView { }

    toolTipItem: Loader {
        id: tooltipLoader

        Layout.minimumWidth: item ? item.implicitWidth : 0
        Layout.maximumWidth: item ? item.implicitWidth : 0
        Layout.minimumHeight: item ? item.implicitHeight : 0
        Layout.maximumHeight: item ? item.implicitHeight : 0

        source: Qt.resolvedUrl("Tooltip.qml")
    }

    //We need Local to be *always* present, even if not disaplayed as
    //it's used for formatting in ToolTip.dateTimeChanged()
    property list<string> allTimeZones

    Connections {
        target: Plasmoid.configuration
        function onSelectedTimeZonesChanged() {
            root.initTimeZones();
        }
    }

    hideOnWindowDeactivate: !Plasmoid.configuration.pin

    P5Support.DataSource {
        id: dataSource
        engine: "time"
        connectedSources: allTimeZones
        interval: intervalAlignment === P5Support.Types.NoAlignment ? 1000 : 60000
        intervalAlignment: {
            if (Plasmoid.configuration.showSeconds === 2
                || (Plasmoid.configuration.showSeconds === 1
                    && compactRepresentationItem
                    && compactRepresentationItem.containsMouse)) {
                return P5Support.Types.NoAlignment;
            } else {
                return P5Support.Types.AlignToMinute;
            }
        }
    }

    function setDateFormatString() {
        // remove "dddd" from the locale format string
        // /all/ locales in LongFormat have "dddd" either
        // at the beginning or at the end. so we just
        // remove it + the delimiter and space
        let format = Qt.locale().dateFormat(Locale.LongFormat);
        format = format.replace(/(^dddd.?\s)|(,?\sdddd$)/, "");
        return format;
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            id: clipboardAction
            text: i18n("Copy to Clipboard")
            icon.name: "edit-copy"
        },
        PlasmaCore.Action {
            text: i18n("Adjust Date and Time…")
            icon.name: "clock"
            visible: KConfig.KAuthorized.authorize("kcm_clock")
            onTriggered: KCMUtils.KCMLauncher.openSystemSettings("kcm_clock")
        },
        PlasmaCore.Action {
            text: i18n("Set Time Format…")
            icon.name: "gnumeric-format-thousand-separator"
            visible: KConfig.KAuthorized.authorizeControlModule("kcm_regionandlang")
            onTriggered: KCMUtils.KCMLauncher.openSystemSettings("kcm_regionandlang")
        }
    ]

    Component.onCompleted: {
        ClipboardMenu.setupMenu(clipboardAction);

        initTimeZones();
    }
}
