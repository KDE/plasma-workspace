/*
 * Copyright 2013  Heena Mahour <heena393@gmail.com>
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 * Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>
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

import org.kde.plasma.calendar 2.0

Item {
    Plasmoid.switchWidth: units.gridUnit * 12
    Plasmoid.switchHeight: units.gridUnit * 12

    Plasmoid.toolTipMainText: Qt.formatDate(dataSource.data.Local.DateTime, "dddd")
    Plasmoid.toolTipSubText: {
        // this logic is taken from digital-clock:
        // remove "dddd" from the locale format string
        // /all/ locales in LongFormat have "dddd" either
        // at the beginning or at the end. so we just
        // remove it + the delimiter and space
        var format = Qt.locale().dateFormat(Locale.LongFormat);
        format = format.replace(/(^dddd.?\s)|(,?\sdddd$)/, "");
        return Qt.formatDate(dataSource.data.Local.DateTime, format)
    }

    Layout.minimumWidth: units.iconSizes.large
    Layout.minimumHeight: units.iconSizes.large

    PlasmaCore.DataSource {
        id: dataSource
        engine: "time"
        connectedSources: ["Local"]
        interval: 60000
        intervalAlignment: PlasmaCore.Types.AlignToMinute
    }

    Plasmoid.compactRepresentation: MouseArea {
        onClicked: plasmoid.expanded = !plasmoid.expanded

        PlasmaCore.IconItem {
            anchors.fill: parent
            source: Qt.resolvedUrl("../images/mini-calendar.svgz")

            PlasmaComponents.Label {
                anchors {
                    fill: parent
                    margins: Math.round(parent.width * 0.1)
                }
                color: "#000000"
                height: undefined
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 1000
                minimumPointSize: theme.smallestFont.pointSize
                text: {
                    var d = new Date(dataSource.data.Local.DateTime)
                    var format = plasmoid.configuration.compactDisplay

                    if (format === "w") {
                        return plasmoid.nativeInterface.weekNumber(d)
                    }

                    return Qt.formatDate(d, format)
                }
                fontSizeMode: Text.Fit
            }
        }
    }

    Plasmoid.fullRepresentation: Item {

        // sizing taken from digital clock
        readonly property int _minimumWidth: calendar.showWeekNumbers ? Math.round(_minimumHeight * 1.75) : Math.round(_minimumHeight * 1.5)
        readonly property int _minimumHeight: units.gridUnit * 14

        Layout.preferredWidth: _minimumWidth
        Layout.preferredHeight: Math.round(_minimumHeight * 1.5)

        MonthView {
            id: calendar
            today: dataSource.data["Local"]["DateTime"]
            showWeekNumbers: plasmoid.configuration.showWeekNumbers

            anchors.fill: parent
        }
    }
}
