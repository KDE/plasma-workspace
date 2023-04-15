/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.private.digitalclock 1.0
import org.kde.plasma.workspace.calendar 2.0 as PlasmaCalendar

Item {
    id: root

    width: PlasmaCore.Units.gridUnit * 10
    height: PlasmaCore.Units.gridUnit * 4
    property string dateFormatString: setDateFormatString()
    Plasmoid.backgroundHints: PlasmaCore.Types.ShadowBackground | PlasmaCore.Types.ConfigurableBackground

    function initTimezones() {
        var tz  = Array()
        if (Plasmoid.configuration.selectedTimeZones.indexOf("Local") === -1) {
            tz.push("Local");
        }
        root.allTimezones = tz.concat(Plasmoid.configuration.selectedTimeZones);
    }

    function formatTime(dateTime, showSecondsForZone) {
        var compactRepresentationItem = Plasmoid.compactRepresentationItem;
        if (!compactRepresentationItem) {
            return "";
        }

        var formattedTime;
        if (showSecondsForZone) {
            formattedTime = Qt.formatTime(dateTime, compactRepresentationItem.timeFormatWithSeconds);
        } else {
            formattedTime = Qt.formatTime(dateTime, compactRepresentationItem.timeFormat);
        }

        // FIXME
        // if (dateTime.getDay() !== dataSource.data["Local"]["DateTime"].getDay()) {
        //     formattedTime += " (" + Qt.formatDate(dateTime, compactRepresentationItem.dateFormat) + ")";
        // }

        return formattedTime;
    }
/*
    function nameForZone(zone) {
        // add the timezone string to the clock
        var timezoneString = Plasmoid.configuration.displayTimezoneAsCode ? dataSource.data[zone]["Timezone Abbreviation"]
                                                                          : TimezonesI18n.i18nCity(dataSource.data[zone]["Timezone City"]);

        return timezoneString;
    }*/

    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: DigitalClock {
        activeFocusOnTab: true
        hoverEnabled: true

        Accessible.name: tooltipLoader.item.Accessible.name
        Accessible.description: tooltipLoader.item.Accessible.description
        Accessible.role: Accessible.Button
    }
    Plasmoid.fullRepresentation: CalendarView { }

    Plasmoid.toolTipItem: Loader {
        id: tooltipLoader

        Layout.minimumWidth: item ? item.implicitWidth : 0
        Layout.maximumWidth: item ? item.implicitWidth : 0
        Layout.minimumHeight: item ? item.implicitHeight : 0
        Layout.maximumHeight: item ? item.implicitHeight : 0

        source: "Tooltip.qml"
    }

    //We need Local to be *always* present, even if not disaplayed as
    //it's used for formatting in ToolTip.dateTimeChanged()
    property var allTimezones
    Connections {
        target: Plasmoid.configuration
        function onSelectedTimeZonesChanged() { root.initTimezones(); }
    }


    function setDateFormatString() {
        // remove "dddd" from the locale format string
        // /all/ locales in LongFormat have "dddd" either
        // at the beginning or at the end. so we just
        // remove it + the delimiter and space
        var format = Qt.locale().dateFormat(Locale.LongFormat);
        format = format.replace(/(^dddd.?\s)|(,?\sdddd$)/, "");
        return format;
    }

    function action_clockkcm() {
        KCMShell.openSystemSettings("kcm_clock");
    }

    function action_formatskcm() {
        KCMShell.openSystemSettings("kcm_regionandlang");
    }

    Component.onCompleted: {
        Plasmoid.setAction("clipboard", i18n("Copy to Clipboard"), "edit-copy");
        ClipboardMenu.setupMenu(Plasmoid.action("clipboard"));

        root.initTimezones();
        if (KCMShell.authorize("kcm_clock.desktop").length > 0) {
            Plasmoid.setAction("clockkcm", i18n("Adjust Date and Time…"), "clock");
        }
        if (KCMShell.authorize("kcm_regionandlang.desktop").length > 0) {
            Plasmoid.setAction("formatskcm", i18n("Set Time Format…"), "gnumeric-format-thousand-separator");
        }
    }
}
