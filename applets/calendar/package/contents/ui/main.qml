/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.12
import QtQuick.Layouts 1.12

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

import org.kde.plasma.workspace.calendar 2.0

Item {
    Plasmoid.switchWidth: PlasmaCore.Units.gridUnit * 12
    Plasmoid.switchHeight: PlasmaCore.Units.gridUnit * 12

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

    Layout.minimumWidth: PlasmaCore.Units.iconSizes.large
    Layout.minimumHeight: PlasmaCore.Units.iconSizes.large

    PlasmaCore.DataSource {
        id: dataSource
        engine: "time"
        connectedSources: ["Local"]
        interval: 60000
        intervalAlignment: PlasmaCore.Types.AlignToMinute
    }

    Plasmoid.compactRepresentation: MouseArea {
        onClicked: Plasmoid.expanded = !Plasmoid.expanded

        PlasmaCore.IconItem {
            anchors.fill: parent

            source: Qt.resolvedUrl("../images/mini-calendar.svgz")

            PlasmaComponents3.Label {
                id: monthLabel
                y: parent.y + parent.height * 0.05;
                x: 0
                width: parent.width
                height: parent.height * 0.2

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
                fontSizeMode: Text.Fit
                minimumPointSize: 1

                /* color must be black because it's set on top of a white icon */
                color: "black"

                text: {
                    var d = new Date(dataSource.data.Local.DateTime);
                    return Qt.formatDate(d, "MMM");
                }
                visible: parent.width > PlasmaCore.Units.gridUnit * 3
            }

            PlasmaComponents3.Label {
                anchors.top: monthLabel.bottom
                x: 0
                width: parent.width
                height: parent.height * 0.6
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
                minimumPointSize: 1
                font.pixelSize: 1000

                fontSizeMode: Text.Fit

                /* color must be black because it's set on top of a white icon */
                color: "black"
                text: {
                    var d = new Date(dataSource.data.Local.DateTime)
                    var format = Plasmoid.configuration.compactDisplay

                    if (format === "w") {
                        return Plasmoid.nativeInterface.weekNumber(d)
                    }

                    return Qt.formatDate(d, format)
                }
            }
        }
    }

    Plasmoid.fullRepresentation: Item {

        // sizing taken from digital clock
        readonly property int _minimumWidth: calendar.showWeekNumbers ? Math.round(_minimumHeight * 1.75) : Math.round(_minimumHeight * 1.5)
        readonly property int _minimumHeight: PlasmaCore.Units.gridUnit * 14
        readonly property var appletInterface: Plasmoid.self

        Layout.minimumWidth: _minimumWidth
        Layout.maximumWidth: PlasmaCore.Units.gridUnit * 80
        Layout.minimumHeight: _minimumHeight
        Layout.maximumHeight: PlasmaCore.Units.gridUnit * 40

        MonthView {
            id: calendar
            today: dataSource.data["Local"]["DateTime"]
            showWeekNumbers: Plasmoid.configuration.showWeekNumbers

            anchors.fill: parent
        }
    }
}
