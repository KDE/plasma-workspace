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
import org.kde.plasma.private.digitalclock 1.0
//import org.kde.plasma.calendar 2.0

Item {
    id: main

    width: units.gridUnit * 10
    height: units.gridUnit * 4
    property string dateFormatString: setDateFormatString()

    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: DigitalClock { }
    Plasmoid.fullRepresentation: CalendarView { }

    Plasmoid.toolTipMainText: Qt.formatDate(dataSource.data["Local"]["DateTime"],"dddd")
    Plasmoid.toolTipSubText:  Qt.formatDate(dataSource.data["Local"]["DateTime"], dateFormatString)

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
        interval: plasmoid.configuration.showSeconds ? 1000 : 30000
    }

    function setDateFormatString() {
        // remove "dddd" from the locale format string
        // /all/ locales in LongFormat have "dddd" either
        // at the beginning or at the end. so we just
        // remove it + the delimiter and space
        var format = Qt.locale().dateFormat(Locale.LongFormat);
        format = format.replace(/(^dddd.?\s)|(,?\sdddd$)/, "");
        main.dateFormatString = format;
    }

    ProcessRunner {
        id: processRunner
    }

    function action_formatskcm() {
        processRunner.runFormatsKCM();
    }

    Component.onCompleted: {
        plasmoid.setAction("formatskcm", i18n("Set Time Format..."));
    }
}
