/*
 *  SPDX-FileCopyrightText: 2024 Niccolò Venerandi <niccolo@venerandi.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtPositioning
import QtLocation
import Qt.labs.qmlmodels

import org.kde.kirigami as Kirigami

DelegateChooser {
    id: root

    required property string selectedTimeZone
    required property string hoveredTimeZone

    property color defaultColor: Kirigami.Theme.highlightColor
    property real selectedOpacity: 0.8
    property real hoveredOpacity: 0.6
    property real defaultOpacity: 0.1

    signal timeZoneselected(timeZoneId: string, centroid: var, bounds: var)

    signal timeZoneHovered(timeZoneId: string, hovered: bool)

    role: "type"

    DelegateChoice {
        roleValue: "Polygon"
        delegate: MapPolygon {
            id: mapPolygon
            required property var modelData
            readonly property string tzid: modelData?.properties?.tzid || parent.tzid || ""
            readonly property var centroid: {
                let raw = modelData?.properties?.centroid || parent?.modelData?.properties?.centroid || [0, 0]
                return QtPositioning.coordinate(raw[1], raw[0])
            }
            readonly property var bounds: {
                let raw = modelData?.properties?.bounds || parent?.modelData?.properties?.bounds || [0, 0, 0, 0]
                return QtPositioning.rectangle(QtPositioning.coordinate(raw[3], raw[0]), QtPositioning.coordinate(raw[1], raw[2]))
            }

            readonly property bool tzidselected: tzid.length > 0 && root.selectedTimeZone === tzid
            readonly property bool tzidhover: tzid.length > 0 && root.hoveredTimeZone === tzid

            geoShape: modelData.data
            opacity: tzidselected ? root.selectedOpacity : (tzidhover ? root.hoveredOpacity : root.defaultOpacity)
            color: root.defaultColor
            border {
                width: 2
                color: Qt.darker(color)
            }
            autoFadeIn: false
            referenceSurface: QtLocation.ReferenceSurface.Map

            TapHandler {
                enabled: !!mapPolygon.tzid
                onTapped: root.timeZoneselected(mapPolygon.tzid, mapPolygon.centroid, mapPolygon.bounds)
            }

            HoverHandler {
                enabled: !!mapPolygon.tzid
                onHoveredChanged: root.timeZoneHovered(mapPolygon.tzid, hovered)
            }
        }
    }

    DelegateChoice {
        roleValue: "MultiPolygon"
        delegate: MapItemView {
            id: mapMultiPoly
            required property var modelData
            readonly property string tzid: modelData?.properties?.tzid
            model: modelData.data
            delegate: root
        }
    }

    DelegateChoice {
        roleValue: "FeatureCollection"
        delegate: MapItemView {
            required property var modelData
            model: modelData.data
            delegate: root
        }
    }
}
