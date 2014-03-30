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
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import "plasmapackage:/code/logic.js" as Logic

Item {
    id: analogclock
    Layout.minimumWidth: 20
    Layout.minimumHeight: 20

    property int hours
    property int minutes
    property int seconds
    property bool showSecondsHand: plasmoid.configuration.showSecondHand
    property bool showTimezone: plasmoid.configuration.showTimezoneString


    PlasmaCore.DataSource {
        id: dataSource
        engine: "time"
        connectedSources: "Local"
        interval: 1000
        onDataChanged: {
            var date = new Date(data["Local"]["Time"]);
            hours = date.getHours();
            minutes = date.getMinutes();
            seconds = date.getSeconds();
            timezoneText.text = data["Local"]["Timezone"];
        }
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
        anchors.centerIn: timezoneText
        visible: showTimezone
        Text {
            id: timezoneText
            x: timezoneBg.margins.left
            y: timezoneBg.margins.top
        }
    }
        

    PlasmaCore.Dialog {
        id: calendar
        flags: Qt.Popup
        //FIXME Temporarily disabled until Calendar becomes available
//         mainItem: Calendar {
//             firstDayOfMonth: 4
//             today: "2011-12-07"
//             year: Logic.getYear(today)
//             month: Logic.getMonth(today)
//             day: Logic.getDate(today)
//         }
    }

    MouseArea {
        anchors.fill: parent
//         onClicked: {
//             if (!calendar.visible) {
//                 var pos = calendar.popupPosition(analogclock, Qt.AlignCenter);
//                 calendar.x = pos.x;
//                 calendar.y = pos.y;
//             }
//             calendar.visible = !calendar.visible;
//         }
    }

    Component.onCompleted: {
        plasmoid.backgroundHints = "NoBackground";

    }
}
