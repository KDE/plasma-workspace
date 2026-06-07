/*
 *  SPDX-FileCopyrightText: 2024 Niccolò Venerandi <niccolo@venerandi.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtPositioning
import QtLocation
import Qt.labs.qmlmodels

import org.kde.kirigami as Kirigami

DelegateChooser {
    id: delegateChooser
    role: "type"

    property color defaultColor: Kirigami.Theme.highlightColor
    property real defaultOpacity: 0.1

    function suggestedZoomOf(bbox): var {
        const deltaLat = Math.abs(bbox.maxLat - bbox.minLat);
        const deltaLon = Math.abs(bbox.maxLon - bbox.minLon);
        const delta = Math.max(deltaLat, deltaLon);
        let zoom = Math.round(Math.log2(360 / delta)) * 0.7;
        zoom = Math.min(Math.max(zoom, 1), view.map.maximumZoomLevel);
        if (Math.abs(zoom - view.map.zoomLevel) < 1) {
            return view.map.zoomLevel
        }
        return zoom
    }

    function animateZoomLevel(target: var): void {
        zoomLevelAnimation.to = target
        zoomLevelAnimation.running = true
    }


    DelegateChoice {
        roleValue: "Polygon"
        delegate: MapPolygon {
            readonly property string tzid: modelData?.properties?.tzid || parent.tzid || ""

            property bool thisItemSelected: root.selectedTimeZone == modelData?.properties?.tzid
            onThisItemSelectedChanged: {
                if (!thisItemSelected) return;
                const props = modelData?.properties || {}
                view.animateCenterTo(QtPositioning.coordinate(props.centerLat || 0, props.centerLon || 0))
                delegateChooser.animateZoomLevel(delegateChooser.suggestedZoomOf({
                    minLat: props.bboxMinLat, maxLat: props.bboxMaxLat,
                    minLon: props.bboxMinLon, maxLon: props.bboxMaxLon,
                }))
            }

            geoShape: modelData.data
            opacity: delegateChooser.defaultOpacity
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
        }
    }

    DelegateChoice {
        roleValue: "MultiPolygon"
        delegate: MapItemView {
            required property var modelData
            property string tzid: modelData?.properties?.tzid
            model: modelData.data
            delegate: delegateChooser

            property bool thisItemSelected: root.selectedTimeZone === tzid
            onThisItemSelectedChanged: {
                if (!thisItemSelected) return;
                const props = modelData?.properties || {}
                view.animateCenterTo(QtPositioning.coordinate(props.centerLat || 0, props.centerLon || 0))
                delegateChooser.animateZoomLevel(delegateChooser.suggestedZoomOf({
                    minLat: props.bboxMinLat, maxLat: props.bboxMaxLat,
                    minLon: props.bboxMinLon, maxLon: props.bboxMaxLon,
                }))
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
