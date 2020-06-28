/*
 *   Copyright 2015 by Martin Klapetek <mklapetek@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: tooltipContentItem

    property int preferredTextWidth: units.gridUnit * 20

    width: childrenRect.width + units.gridUnit
    height: childrenRect.height + units.gridUnit

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    PlasmaCore.ColorScope.colorGroup: PlasmaCore.Theme.NormalColorGroup
    PlasmaCore.ColorScope.inherit: false

    ColumnLayout {
        anchors {
            left: parent.left
            top: parent.top
            margins: units.gridUnit / 2
        }

        spacing: 0

        PlasmaExtras.Heading {
            id: tooltipMaintext
            level: 3
            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth
            elide: Text.ElideRight
            text: clocks.visible ? Qt.formatDate(tzDate, Locale.LongFormat) : Qt.formatDate(tzDate,"dddd")
        }

        PlasmaComponents3.Label {
            id: tooltipSubtext
            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth
            text: Qt.formatDate(tzDate, dateFormatString)
            opacity: 0.6
            visible: !clocks.visible
        }

        GridLayout {
            id: clocks
            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth
            Layout.maximumHeight: childrenRect.height
            columns: 2
            visible: plasmoid.configuration.selectedTimeZones.length > 1
            rowSpacing: 0

            Repeater {
                model: {
                    // The timezones need to be duplicated in the array
                    // because we need their data twice - once for the name
                    // and once for the time and the Repeater delegate cannot
                    // be one Item with two Labels because that wouldn't work
                    // in a grid then
                    var timezones = [];
                    for (var i = 0; i < plasmoid.configuration.selectedTimeZones.length; i++) {
                        timezones.push(plasmoid.configuration.selectedTimeZones[i]);
                        timezones.push(plasmoid.configuration.selectedTimeZones[i]);
                    }

                    return timezones;
                }

                PlasmaComponents3.Label {
                    id: timezone
                    // Layout.fillWidth is buggy here
                    Layout.alignment: index % 2 === 0 ? Qt.AlignRight : Qt.AlignLeft

                    wrapMode: Text.NoWrap
                    text: index % 2 == 0 ? nameForZone(modelData) : timeForZone(modelData)
                    font.weight: modelData === plasmoid.configuration.lastSelectedTimezone ? Font.Bold : Font.Normal
                    elide: Text.ElideNone
                    opacity: 0.6
                }
            }
        }
    }
}
