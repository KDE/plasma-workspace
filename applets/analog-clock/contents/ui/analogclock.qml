/*
 *   Copyright 2012 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2012 Marco Martin <mart@kde.org>
 *   Copyright 2013 David Edmundson <davidedmundson@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.calendar 2.0 as PlasmaCalendar
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import "plasmapackage:/code/logic.js" as Logic

Item {
    id: analogclock

    width: units.gridUnit * 15
    height: units.gridUnit * 15
    property int hours
    property int minutes
    property int seconds
    property bool showSecondsHand: plasmoid.configuration.showSecondHand
    property bool showTimezone: plasmoid.configuration.showTimezoneString

    Plasmoid.backgroundHints: "NoBackground";
    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation

    PlasmaCore.DataSource {
        id: dataSource
        engine: "time"
        connectedSources: "Local"
        interval: plasmoid.configuration.showSecondHand ? 1000 : 30000
        onDataChanged: {
            var date = new Date(data["Local"]["DateTime"]);
            hours = date.getHours();
            minutes = date.getMinutes();
            seconds = date.getSeconds();
        }
    }

    Plasmoid.compactRepresentation: Item {
        id: representation
        Layout.minimumWidth: plasmoid.formFactor != PlasmaCore.Types.Vertical ? representation.height : units.gridUnit
        Layout.minimumHeight: plasmoid.formFactor == PlasmaCore.Types.Vertical ? representation.width : units.gridUnit

        MouseArea {
            anchors.fill: parent
            onClicked: plasmoid.expanded = !plasmoid.expanded
        }


        PlasmaCore.Svg {
            id: clockSvg
            imagePath: "widgets/clock"
        }

        Item {
            id: clock
            width: parent.width
            anchors {
                top: parent.top
                bottom: showTimezone ? timezoneBg.top : parent.bottom
            }

            PlasmaCore.SvgItem {
                id: face
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: Math.min(parent.width, parent.height)
                svg: clockSvg
                elementId: "ClockFace"
            }

            PlasmaCore.SvgItem {
                anchors.fill: face
                svg: clockSvg
                elementId: "Glass"
                width: naturalSize.width * face.width / face.naturalSize.width
                height: naturalSize.height * face.width / face.naturalSize.width
            }
        }

        Hand {
            anchors.topMargin: 3
            elementId: "HourHandShdow"
            rotation: 180 + hours * 30 + (minutes/2)
            svgScale: face.width / face.naturalSize.width

        }
        Hand {
            elementId: "HourHand"
            rotation: 180 + hours * 30 + (minutes/2)
            svgScale: face.width / face.naturalSize.width
        }

        Hand {
            anchors.topMargin: 3
            elementId: "MinuteHandShadow"
            rotation: 180 + minutes * 6
            svgScale: face.width / face.naturalSize.width
        }
        Hand {
            elementId: "MinuteHand"
            rotation: 180 + minutes * 6
            svgScale: face.width / face.naturalSize.width
        }

        Hand {
            anchors.topMargin: 3
            elementId: "SecondHandShadow"
            rotation: 180 + seconds * 6
            visible: showSecondsHand
            svgScale: face.width / face.naturalSize.width
        }
        Hand {
            elementId: "SecondHand"
            rotation: 180 + seconds * 6
            visible: showSecondsHand
            svgScale: face.width / face.naturalSize.width
        }

        PlasmaCore.SvgItem {
            id: center
            width: naturalSize.width * face.width / face.naturalSize.width
            height: naturalSize.height * face.width / face.naturalSize.width
            anchors.centerIn: clock
            svg: clockSvg
            elementId: "HandCenterScrew"
            z: 1000
        }

        PlasmaCore.FrameSvgItem {
            id: timezoneBg
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: 10
            }
            imagePath: "widgets/background"
            width: childrenRect.width + margins.right + margins.left
            height: childrenRect.height + margins.top + margins.bottom
            visible: showTimezone
            Text {
                id: timezoneText
                x: timezoneBg.margins.left
                y: timezoneBg.margins.top
                text: dataSource.data["Local"]["Timezone"]
            }
        }
    }
    Plasmoid.fullRepresentation: PlasmaCalendar.MonthView {
        Layout.minimumWidth: units.gridUnit * 20
        Layout.minimumHeight: units.gridUnit * 20

        today: dataSource.data["Local"]["DateTime"]
    }

}
