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
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.calendar 2.0

Item {
    id: main
    //     Layout.minimumWidth: units.gridUnit * 90
    //     Layout.minimumHeight: units.gridUnit * 30

//     Layout.minimumWidth: 500
//     Layout.minimumHeight: 500
    property int formFactor: plasmoid.formFactor

//     PlasmaCore.DataSource {
//         id: dataSource
//         engine: "time"
//         connectedSources: ["Local"]
//         interval: 300000
//     }
//
//     Component.onCompleted: {
//         var toolTipData = new Object;
//         toolTipData["image"] = "preferences-system-time";
//         toolTipData["mainText"] ="Current Time"
//         toolTipData["subText"] = Qt.formatDate( dataSource.data["Local"]["Date"],"dddd dd MMM yyyy" )+"\n"+Qt.formatTime( dataSource.data["Local"]["Time"], "HH:MM")
//         plasmoid.popupIconToolTip = toolTipData;
//     }

    MonthView {
        id: calendar
    }

//     Plasmoid.compactRepresentation: Item {
//         width: main.minimumWidth
//         height: main.minimumHeight
//         Image {
//             anchors.fill: parent
//             source: "plasmapackage:/images/mini-calendar.svgz"
//         }
//         PlasmaCore.SvgItem {
//             anchors.fill: parent
//             elementId: "mini-calendar"
//             svg: PlasmaCore.Svg {
//                 imagePath: "plasmapackage:/images/mini-calendar.svgz"
//
//             }
//
//             PlasmaComponents.Label {
//                 text: "14"
//                 font: theme.smallestFont
//                 anchors.centerIn: parent
//             }
//         }
//     }
}
