/*
 * Copyright 2013  Heena Mahour <heena393@gmail.com>
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
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.private.digitalclock 1.0
//import org.kde.plasma.calendar 2.0

Item {
    id: root

    width: units.gridUnit * 10
    height: units.gridUnit * 4
    property string dateFormatString: setDateFormatString()
    property date tzDate: {
        // get the time for the given timezone from the dataengine
        var now = dataSource.data[plasmoid.configuration.lastSelectedTimezone]["DateTime"];
        // get current UTC time
        var msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
        // add the dataengine TZ offset to it
        return new Date(msUTC + (dataSource.data[plasmoid.configuration.lastSelectedTimezone]["Offset"] * 1000));
    }

    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: DigitalClock { }
    Plasmoid.fullRepresentation: CalendarView { }

    Plasmoid.toolTipMainText: Qt.formatDate(tzDate,"dddd")
    Plasmoid.toolTipSubText:  Qt.formatDate(tzDate, dateFormatString)

    PlasmaCore.DataSource {
        id: dataSource
        engine: "time"
        connectedSources: {
            var timeZones = [];
            for (var i = 0; i < plasmoid.configuration.selectedTimeZones.length; i++) {
                if (plasmoid.configuration.selectedTimeZones[i] == "") {
                    continue;
                }

                timeZones.push(plasmoid.configuration.selectedTimeZones[i]);
            }
            timeZones.push("Local");
            return timeZones;
        }
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

    function action_formatskcm() {
        KCMShell.open("formats");
    }

    Component.onCompleted: {
        plasmoid.setAction("formatskcm", i18n("Set Time Format..."));
    }
}
