/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.12
import QtQuick.Layouts 1.12

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.plasma5support 2.0 as P5Support
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami as Kirigami
import org.kde.ksvg as KSvg

import org.kde.plasma.workspace.calendar 2.0

PlasmoidItem {
    id: root
    switchWidth: Kirigami.Units.gridUnit * 12
    switchHeight: Kirigami.Units.gridUnit * 12

    toolTipMainText: Qt.formatDate(dataSource.data.Local.DateTime, "dddd")
    toolTipSubText: {
        // this logic is taken from digital-clock:
        // remove "dddd" from the locale format string
        // /all/ locales in LongFormat have "dddd" either
        // at the beginning or at the end. so we just
        // remove it + the delimiter and space
        var format = Qt.locale().dateFormat(Locale.LongFormat);
        format = format.replace(/(^dddd.?\s)|(,?\sdddd$)/, "");
        return Qt.formatDate(dataSource.data.Local.DateTime, format)
    }

    Layout.minimumWidth: Kirigami.Units.iconSizes.large
    Layout.minimumHeight: Kirigami.Units.iconSizes.large

    P5Support.DataSource {
        id: dataSource
        engine: "time"
        connectedSources: ["Local"]
        interval: 60000
        intervalAlignment: P5Support.Types.AlignToMinute
    }

    // Only exists because the default CompactRepresentation doesn't expose a
    // generic way to overlay something on top of the icon.
    // TODO remove once it gains that feature.
    compactRepresentation: MouseArea {
        onClicked: root.expanded = !root.expanded

        KSvg.SvgItem {
            anchors.centerIn: parent
            width: Kirigami.Units.iconSizes.roundedIconSize(Math.min(parent.width, parent.height))
            height: width

            imagePath: Qt.resolvedUrl("../images/mini-calendar.svgz")

            PlasmaComponents3.Label {
                id: monthLabel
                y: parent.height * 0.05;
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
                textFormat: Text.PlainText
                visible: parent.width > Kirigami.Units.gridUnit * 3
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
                        return Plasmoid.weekNumber(d)
                    }

                    return Qt.formatDate(d, format)
                }
                textFormat: Text.PlainText
            }
        }
    }

    fullRepresentation: Item {

        // sizing taken from digital clock
        readonly property int _minimumWidth: calendar.showWeekNumbers ? Math.round(_minimumHeight * 1.75) : Math.round(_minimumHeight * 1.5)
        readonly property int _minimumHeight: Kirigami.Units.gridUnit * 14
        readonly property var appletInterface: root

        Layout.minimumWidth: _minimumWidth
        Layout.maximumWidth: Kirigami.Units.gridUnit * 80
        Layout.minimumHeight: _minimumHeight
        Layout.maximumHeight: Kirigami.Units.gridUnit * 40

        MonthView {
            id: calendar
            today: dataSource.data["Local"]["DateTime"]
            showWeekNumbers: Plasmoid.configuration.showWeekNumbers

            anchors.fill: parent
        }
    }
}
