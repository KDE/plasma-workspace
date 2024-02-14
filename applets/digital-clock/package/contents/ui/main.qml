/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support 2.0 as P5Support
import org.kde.plasma.private.digitalclock 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.20 as Kirigami

import org.kde.kcmutils // KCMLauncher
import org.kde.config // KAuthorized

PlasmoidItem {
    id: root

    width: Kirigami.Units.gridUnit * 10
    height: Kirigami.Units.gridUnit * 4
    property string dateFormatString: setDateFormatString()
    Plasmoid.backgroundHints: PlasmaCore.Types.ShadowBackground | PlasmaCore.Types.ConfigurableBackground
    property date tzDate: {
        const data = dataSource.data[Plasmoid.configuration.lastSelectedTimezone];
        if (data === undefined) {
            return new Date();
        }
        // get the time for the given timezone from the dataengine
        const now = data["DateTime"];
        // get current UTC time
        const msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
        // add the dataengine TZ offset to it
        return new Date(msUTC + (data["Offset"] * 1000));
    }

    function initTimezones() {
        const tz = []
        if (Plasmoid.configuration.selectedTimeZones.indexOf("Local") === -1) {
            tz.push("Local");
        }
        root.allTimezones = tz.concat(Plasmoid.configuration.selectedTimeZones);
    }

    function timeForZone(zone, showSecondsForZone) {
        if (!compactRepresentationItem) {
            return "";
        }

        // get the time for the given timezone from the dataengine
        const now = dataSource.data[zone]["DateTime"];
        // get current UTC time
        const msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
        // add the dataengine TZ offset to it
        const dateTime = new Date(msUTC + (dataSource.data[zone]["Offset"] * 1000));

        let formattedTime;
        if (showSecondsForZone) {
            formattedTime = Qt.formatTime(dateTime, compactRepresentationItem.timeFormatWithSeconds);
        } else {
            formattedTime = Qt.formatTime(dateTime, compactRepresentationItem.timeFormat);
        }

        if (dateTime.getDay() !== dataSource.data["Local"]["DateTime"].getDay()) {
            formattedTime += " (" + compactRepresentationItem.dateFormatter(dateTime) + ")";
        }

        return formattedTime;
    }

    function nameForZone(zone) {
        // add the timezone string to the clock
        if (Plasmoid.configuration.displayTimezoneAsCode) {
            return dataSource.data[zone]["Timezone Abbreviation"];
        } else {
            return TimezonesI18n.i18nCity(dataSource.data[zone]["Timezone"]);
        }
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
    property var allTimezones
    Connections {
        target: Plasmoid.configuration
        function onSelectedTimeZonesChanged() { root.initTimezones(); }
    }

    Binding {
        target: root
        property: "hideOnWindowDeactivate"
        value: !Plasmoid.configuration.pin
        restoreMode: Binding.RestoreBinding
    }

    P5Support.DataSource {
        id: dataSource
        engine: "time"
        connectedSources: allTimezones
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
            visible: KAuthorized.authorize("kcm_clock")
            onTriggered: KCMLauncher.openSystemSettings("kcm_clock")
        },
        PlasmaCore.Action {
            text: i18n("Set Time Format…")
            icon.name: "gnumeric-format-thousand-separator"
            visible: KAuthorized.authorizeControlModule("kcm_regionandlang")
            onTriggered: KCMLauncher.openSystemSettings("kcm_regionandlang")
        }
    ]

    Component.onCompleted: {
        ClipboardMenu.setupMenu(clipboardAction);

        root.initTimezones();
    }
}
