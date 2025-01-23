/*
 *  SPDX-FileCopyrightText: 2024 Niccolò Venerandi <niccolo@venerandi.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtPositioning
import QtLocation
import Qt.labs.qmlmodels 1.0

import org.kde.kirigami as Kirigami

DelegateChooser {
    id: delegateChooser
    role: "type"

    property color defaultColor: Kirigami.Theme.highlightColor
    property real selectedOpacity: 0.8
    property real hoveredOpacity: 0.6
    property real defaultOpacity: 0.1

    property var timezoneRectById: ({})

    function centerOf(rect) {
        return QtPositioning.coordinate((rect.minLat + rect.maxLat) / 2,
                                        (rect.minLon + rect.maxLon) / 2)
    }

    function suggestedZoomOf(rect) {
        let delta = Math.max(Math.abs(rect.minLat - rect.maxLat),
                             Math.abs(rect.minLon - rect.maxLon))
        return view.map.maximumZoomLevel * (30 - delta) / 30 + 1
    }

    function animateZoomLevel(target: var): void {
        zoomLevelAnimation.to = target
        zoomLevelAnimation.running = true
    }

    DelegateChoice {
        roleValue: "Polygon"
        delegate: MapPolygon {
            id: mapPolygon
            readonly property string tzid: modelData?.properties?.tzid || parent.tzid || ""
            readonly property string region: tzid ? tzid.split('/')[0] : ""
            readonly property string area: tzid ? tzid.split('/')[1] : ""

            readonly property var rect: {
                let rect = {minLat: Infinity, maxLat: -Infinity, minLon: Infinity, maxLon: -Infinity}
                path.forEach(point => {
                    rect = {
                        minLat: Math.min(rect.minLat, point.latitude),
                        maxLat: Math.max(rect.maxLat, point.latitude),
                        minLon: Math.min(rect.minLon, point.longitude),
                        maxLon: Math.max(rect.maxLon, point.longitude)
                    }
                })
                return rect
            }

            property bool thisItemSelected: root.selectedTimeZone == modelData?.properties?.tzid
            onThisItemSelectedChanged: {
                if (!thisItemSelected) return;
                view.map.center = delegateChooser.centerOf(rect)
                delegateChooser.animateZoomLevel(delegateChooser.suggestedZoomOf(rect))
            }

            property bool tzidhover: root.hoveredTimeZone == tzid
            property bool tzidselected: root.selectedTimeZone == tzid

            geoShape: modelData.data
            opacity: tzidselected ? selectedOpacity : (tzidhover ? delegateChooser.hoveredOpacity : delegateChooser.defaultOpacity)
            color: delegateChooser.defaultColor
            border {
                width: 2
                color: Qt.darker(color)
            }
            autoFadeIn: false
            referenceSurface: view.referenceSurface

            TapHandler {
                onTapped: {
                    root.selectedTimeZone = tzid
                }
            }

            HoverHandler {
                enabled: !!tzid
                onHoveredChanged: {
                    if (hovered) {
                        root.hoveredTimeZone = tzid
                    } else if (root.hoveredTimeZone == tzid) {
                        root.hoveredTimeZone = ""
                    }
                }
            }
        }
    }

    DelegateChoice {
        roleValue: "MultiPolygon"
        delegate: MapItemView {
            required property var modelData
            property string tzid: modelData?.properties?.tzid
            model: modelData.data
            delegate: delegateChooser

            property var rect: {
                let rect = {minLat: Infinity, maxLat: -Infinity, minLon: Infinity, maxLon: -Infinity}
                modelData.data.forEach(polygon => {
                    polygon.data.perimeter.forEach(point => {
                        rect = {
                            minLat: Math.min(rect.minLat, point.latitude),
                            maxLat: Math.max(rect.maxLat, point.latitude),
                            minLon: Math.min(rect.minLon, point.longitude),
                            maxLon: Math.max(rect.maxLon, point.longitude)
                        }
                    })
                })
                return rect
            }

            property bool thisItemSelected: root.selectedTimeZone === tzid
            onThisItemSelectedChanged: {
                if (!thisItemSelected) return;
                view.map.center = delegateChooser.centerOf(rect)
                delegateChooser.animateZoomLevel(delegateChooser.suggestedZoomOf(rect))
            }
        }
    }

    DelegateChoice {
        roleValue: "FeatureCollection"
        delegate: MapItemView {
            model: modelData.data
            delegate: delegateChooser
        }
    }
}
