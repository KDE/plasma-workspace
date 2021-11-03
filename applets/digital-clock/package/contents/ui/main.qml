/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.private.digitalclock 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.calendar 2.0 as PlasmaCalendar

Item {
    id: root

    width: PlasmaCore.Units.gridUnit * 10
    height: PlasmaCore.Units.gridUnit * 4
    property string dateFormatString: setDateFormatString()
    Plasmoid.backgroundHints: PlasmaCore.Types.ShadowBackground | PlasmaCore.Types.ConfigurableBackground
    property date tzDate: {
        // get the time for the given timezone from the dataengine
        var now = dataSource.data[plasmoid.configuration.lastSelectedTimezone]["DateTime"];
        // get current UTC time
        var msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
        // add the dataengine TZ offset to it
        return new Date(msUTC + (dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Offset"] * 1000));
    }

    function initTimezones() {
        var tz  = Array()
        if (plasmoid.configuration.selectedTimeZones.indexOf("Local") === -1) {
            tz.push("Local");
        }
        root.allTimezones = tz.concat(plasmoid.configuration.selectedTimeZones);
    }

    function timeForZone(zone) {
        var compactRepresentationItem = plasmoid.compactRepresentationItem;
        if (!compactRepresentationItem) {
            return "";
        }

        // get the time for the given timezone from the dataengine
        var now = dataSource.data[zone]["DateTime"];
        // get current UTC time
        var msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
        // add the dataengine TZ offset to it
        var dateTime = new Date(msUTC + (dataSource.data[zone]["Offset"] * 1000));

        var formattedTime = Qt.formatTime(dateTime, compactRepresentationItem.timeFormat);

        if (dateTime.getDay() !== dataSource.data["Local"]["DateTime"].getDay()) {
            formattedTime += " (" + Qt.formatDate(dateTime, compactRepresentationItem.dateFormat) + ")";
        }

        return formattedTime;
    }

    function nameForZone(zone) {
        // add the timezone string to the clock
        var timezoneString = plasmoid.configuration.displayTimezoneAsCode ? dataSource.data[zone]["Timezone Abbreviation"]
                                                                          : TimezonesI18n.i18nCity(dataSource.data[zone]["Timezone City"]);

        return timezoneString;
    }

    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: DigitalClock { }
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
        target: plasmoid.configuration
        function onSelectedTimeZonesChanged() { root.initTimezones(); }
    }

    PlasmaCore.DataSource {
        id: dataSource
        engine: "time"
        connectedSources: allTimezones
        interval: plasmoid.configuration.showSeconds ? 1000 : 60000
        intervalAlignment: plasmoid.configuration.showSeconds ? PlasmaCore.Types.NoAlignment : PlasmaCore.Types.AlignToMinute
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
        KCMShell.openSystemSettings("clock");
    }

    function action_formatskcm() {
        KCMShell.openSystemSettings("formats");
    }

    Component.onCompleted: {
        plasmoid.setAction("clipboard", i18n("Copy to Clipboard"), "edit-copy");
        ClipboardMenu.setupMenu(plasmoid.action("clipboard"));

        root.initTimezones();
        if (KCMShell.authorize("clock.desktop").length > 0) {
            plasmoid.setAction("clockkcm", i18n("Adjust Date and Time…"), "preferences-system-time");
        }
        if (KCMShell.authorize("formats.desktop").length > 0) {
            plasmoid.setAction("formatskcm", i18n("Set Time Format…"));
        }

        // Set the list of enabled plugins from config
        // to the manager
        PlasmaCalendar.EventPluginsManager.enabledPlugins = plasmoid.configuration.enabledCalendarPlugins;
    }
}
