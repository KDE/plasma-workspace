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
import org.kde.plasma.private.digitalclock
import org.kde.kirigami as Kirigami
import org.kde.config as KConfig
import org.kde.kcmutils as KCMUtils
import org.kde.plasma.clock

PlasmoidItem {
    id: root

    width: Kirigami.Units.gridUnit * 10
    height: Kirigami.Units.gridUnit * 4

    Plasmoid.backgroundHints: PlasmaCore.Types.ShadowBackground | PlasmaCore.Types.ConfigurableBackground

    readonly property string dateFormatString: setDateFormatString()
    readonly property var currentTime: currentClock.dateTime
    readonly property string currentTimezone: currentClock.timeZone

    Clock {
        id: currentClock
        timeZone: Plasmoid.configuration.lastSelectedTimezone
    }

    Clock {
        id: systemClock
        // not defining a timezone keeps it up to date with the system timezone
    }

    function initTimeZones() {
        const timeZones = [];
        if (Plasmoid.configuration.selectedTimeZones.indexOf("Local") === -1) {
            timeZones.push("Local");
        }
        root.allTimeZones = timeZones.concat(Plasmoid.configuration.selectedTimeZones);
    }

    function formatTime(dateTime: date, showSeconds: bool): string {
        if (!compactRepresentationItem) {
            return "";
        }

        let formattedTime;
        if (showSeconds) {
            formattedTime = Qt.locale().toString(dateTime, compactRepresentationItem.item.timeFormatWithSeconds);
        } else {
            formattedTime = Qt.locale().toString(dateTime, compactRepresentationItem.item.timeFormat);
        }

        if (dateTime.getDay() !== currentClock.dateTime.getDay()) {
            formattedTime += " (" + compactRepresentationItem.item.dateFormatter(dateTime) + ")";
        }

        return formattedTime;
    }

    function selectedTimeZonesDeduplicatingExplicitLocalTimeZone():/* [string] */var {
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
            timeZone == "Local" || timeZone != systemClock.timeZone

        return TimeZoneUtils.sortedTimeZones(Plasmoid.configuration.selectedTimeZones
            .filter(isLiterallyLocalOrResolvesToSomethingOtherThanLocal));
    }

    preferredRepresentation: compactRepresentation

    fullRepresentation: CalendarView { }

    compactRepresentation: Loader {
        id: conditionalLoader

        property bool containsMouse: (item as MouseArea)?.containsMouse ?? false
        Layout.minimumWidth: (item as Item)?.Layout.minimumWidth
        Layout.minimumHeight: (item as Item)?.Layout.minimumHeight
        Layout.preferredWidth: (item as Item)?.Layout.preferredWidth
        Layout.preferredHeight: (item as Item)?.Layout.preferredHeight
        Layout.maximumWidth: (item as Item)?.Layout.maximumWidth
        Layout.maximumHeight: (item as Item)?.Layout.maximumHeight

        sourceComponent: !currentClock.valid ? noTimezoneComponent : digitalClockComponent
    }

    Component {
        id: digitalClockComponent
        DigitalClock {
            activeFocusOnTab: true
            hoverEnabled: true

            Accessible.name: (tooltipLoader.item as Item)?.Accessible.name
            Accessible.description: (tooltipLoader.item as Item)?.Accessible.description
        }
    }

    Component {
        id: noTimezoneComponent
        NoTimezoneWarning { }
    }

    toolTipItem: Loader {
        id: tooltipLoader

        Layout.minimumWidth: item ? (item as Tooltip).implicitWidth : 0
        Layout.maximumWidth: item ? (item as Tooltip).implicitWidth : 0
        Layout.minimumHeight: item ? (item as Tooltip).implicitHeight : 0
        Layout.maximumHeight: item ? (item as Tooltip).implicitHeight : 0

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
        }
    ]

    Component.onCompleted: {
        ClipboardMenu.setupMenu(clipboardAction);

        initTimeZones();
    }
}
