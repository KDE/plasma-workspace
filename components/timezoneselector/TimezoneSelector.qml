/*
    SPDX-FileCopyrightText: 2024 Niccolò Venerandi <niccolo@venerandi.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import QtPositioning
import QtLocation
import QtCore

import org.kde.kirigami as Kirigami
import org.kde.plasma.workspace.timezoneselector as Workspace
import org.kde.kirigamiaddons.components as Components
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * @brief An element that shows all available timezones through a map and comboboxes, and allows you to select one.
 * @inherit QtQuick.Item
 */
Item {
    id: root

    implicitHeight: Kirigami.Settings.isMobile ? mobileFormCard.implicitHeight : 0

//BEGIN properties
    /**
     * @brief This property holds the selected timezone.
     *
     * This timezone will be highlighted on the map and shown
     * in the comboboxes. You can both read and write to this
     * property.
     * @property string selectedTimeZone
     */
    property string selectedTimeZone: ""

    readonly property var bandData: Workspace.TimeZoneUtils.bandData()

    readonly property var allOutlinePaths: {
        const outlines = root.bandData.outlines
        if (!outlines) return []
        const allPaths = []
        for (const groupKey of Object.keys(outlines)) {
            const rings = outlines[groupKey]
            for (const ring of rings) {
                const path = []
                for (const pt of ring) {
                    path.push(QtPositioning.coordinate(pt[1], pt[0]))
                }
                allPaths.push(path)
            }
        }
        return allPaths
    }

    readonly property var selectedOutlinePaths: {
        const tzData = root.bandData.tzids[root.selectedTimeZone]
        if (!tzData) return []
        const groupKey = tzData.bandGroup
        const outlines = root.bandData.outlines[groupKey]
        if (!outlines) return []
        const paths = []
        for (const ring of outlines) {
            const path = []
            for (const pt of ring) {
                path.push(QtPositioning.coordinate(pt[1], pt[0]))
            }
            paths.push(path)
        }
        return paths
    }

    readonly property var zoneLabelModel: {
        const tzids = root.bandData.tzids
        if (!tzids) return []
        const labels = []
        for (const tzid of Object.keys(root.bandData.tzids)) {
            const data = root.bandData.tzids[tzid]
            const slash = tzid.lastIndexOf('/')
            const name = slash >= 0 ? tzid.slice(slash + 1).replace(/_/g, ' ') : tzid
            labels.push({ tzid, name, coordinate: QtPositioning.coordinate(data.centerLat, data.centerLon) })
        }
        return labels
    }
//END properties

    property var availableMapTimeZones: geoDatabase.model[0].data.map(zone => zone?.properties?.tzid)

    GeoJsonData {
        id: geoDatabase
        // GeoJsonData does not support qrc paths, so we have to install
        // the timezones file into the shared folder.
        sourceUrl: StandardPaths.locate(StandardPaths.GenericDataLocation, "timezonefiles", StandardPaths.LocateDirectory)  + "/timezones.json"
    }

    readonly property var availableTimeZones: Workspace.TimeZoneUtils.availableTimeZoneIds()
    property var areasByRegion: {
        const result = {}

        availableTimeZones.forEach(str => {
            let tzid = String(str)
            if (tzid.includes('/')) {

                let [prefix, suffix] = root.split(tzid)

                if (!result[prefix]) {
                    result[prefix] = []
                }

                result[prefix].push(suffix)
            }
        });

        return result
    }
    property var regionsModel: Object.keys(areasByRegion)

    // This is to convert between the technical name of a timezone,
    // e.g. something/some_thing, to one we can display in the UI.
    // Note that doing this only to the displayText would only
    // apply it to the selected element, all other items in the combobox
    // would remain "technical"
    // HACK: Replace .split(X).join(Y) with .replaceAll(X, Y) as soon
    // as Javascript implements that.
    function understandable(id: string): string {
        return id.split("/").join(", ").split("_").join(" ")
    }
    function technical(id: string): string {
        return id.split(", ").join("/").split(" ").join("_")
    }
    // This splits "a/b/c_d" to ["a", "b, c d"]
    function split(id: string): list<string> {
        let i = id.indexOf("/")
        return [root.understandable(id.slice(0, i)), root.understandable(id.slice(i + 1))]
    }

    Kirigami.InlineMessage {
        position: Kirigami.InlineMessage.Header
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
        }
        text: i18ndc("plasmashellprivateplugin", "@info", "The currently selected time zone is not available for visualization on this map")
        visible: !Kirigami.Settings.isMobile && !root.availableMapTimeZones.includes(root.selectedTimeZone) && root.selectedTimeZone
        Kirigami.OverlayZStacking.layer: Kirigami.OverlayZStacking.FullScreen
        z: Kirigami.OverlayZStacking.z
    }

    // On mobile, show stacked FormCard comboboxes without the map.
    FormCard.FormCard {
        id: mobileFormCard
        visible: Kirigami.Settings.isMobile
        width: parent.width

        FormCard.FormComboBoxDelegate {
            id: mobileRegionComboBox

            property string chooseText: i18ndc("plasmashellprivateplugin", "Placeholder for empty time zone combobox selector", "Choose…")

            text: i18ndc("plasmashellprivateplugin", "@label:listbox In the context of time zone selection", "Region:")
            model: [chooseText, ...regionsModel]

            Connections {
                target: root
                function onSelectedTimeZoneChanged() {
                    mobileRegionComboBox.currentIndex = Math.max(mobileRegionComboBox.model.indexOf(root.split(root.selectedTimeZone)[0]), 0)
                }
            }

            onActivated: {
                if (mobileRegionComboBox.currentText === chooseText) return;
                if (mobileRegionComboBox.currentText !== root.split(root.selectedTimeZone)[0]) {
                    mobileLocationComboBox.model = root.areasByRegion[mobileRegionComboBox.currentText]
                }
            }
        }

        FormCard.FormComboBoxDelegate {
            id: mobileLocationComboBox

            visible: mobileRegionComboBox.currentText !== mobileRegionComboBox.chooseText
            text: i18ndc("plasmashellprivateplugin", "@label:listbox In the context of time zone selection", "Time zone:")

            Connections {
                target: root
                function onSelectedTimeZoneChanged() {
                    let [prefix, suffix] = root.split(root.selectedTimeZone)
                    mobileLocationComboBox.model = root.areasByRegion[prefix]
                    mobileLocationComboBox.currentIndex = mobileLocationComboBox.model.indexOf(suffix)
                }
            }

            onActivated: {
                root.selectedTimeZone = root.technical(mobileRegionComboBox.currentText) + '/' + root.technical(mobileLocationComboBox.currentText)
            }
        }
    }

    MapView {
        id: view

        visible: !Kirigami.Settings.isMobile
        anchors.fill: parent

        // HACK: to work around the Qt bug QTBUG-136711,
        // we use the "target" property instead of
        // "NumberAnimotion on map.zoomLevel"
        NumberAnimation {
            target: view
            property: "map.zoomLevel"
            id: zoomLevelAnimation
            running: false
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.InOutCubic
        }
        CoordinateAnimation {
            id: coordAnimation
            target: view
            property: "map.center"
            // HACK: The Map QML element has a bug that sometimes resets
            // its center to the default value when assigned a new
            // (valid) coordinate. To avoid this, we make the animation
            // always last at least one frame, which effectively acts
            // as a timer and re-sets the coordinate to the correct one
            // after that frame. This is not visible by the user but
            // works around the map bug.
            duration: Kirigami.Units.shortDuration + 1
            easing.type: Easing.InOutCubic
            running: false
        }
        function animateCenterTo(coordinate) {
            coordAnimation.to = coordinate
            coordAnimation.running = true
        }

        map {
            plugin: Plugin {
                name: "osm"
                PluginParameter {
                    name: 'osm.mapping.offline.directory'
                    value: ":/plasma-workspace/timezone/offline_tiles"
                }
                PluginParameter {
                    name: "osm.mapping.providersrepository.disabled"
                    value: true
                }
            }
            zoomLevel: 0
            minimumZoomLevel: 0
            // Weirdly enough, the included offline maps of zoom level 0-4
            // only work until a zoom level of ~4.90, whereas zoom level
            // 5 (or even 4.99) would require offline maps for zoom level 5.
            maximumZoomLevel: 4.90
            maximumTilt: 0
            // No maximumBearing property exists, apparently
            onBearingChanged: {
                view.map.bearing = 0
            }
            activeMapType: view.map.supportedMapTypes[0]

            onCopyrightLinkActivated: (link) => { Qt.openUrlExternally(link); }
        }

        property variant referenceSurface: QtLocation.ReferenceSurface.Map

        MapItemView {
            parent: view.map
            model: geoDatabase.model
            delegate: GeoJsonDelegate {}
        }

        MapItemView {
            parent: view.map
            model: root.allOutlinePaths
            delegate: MapPolygon {
                color: "transparent"
                border.width: 1
                border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.15)
                autoFadeIn: false
                z: 90
                path: modelData
            }
        }

        MapItemView {
            parent: view.map
            model: root.selectedOutlinePaths
            delegate: MapPolygon {
                color: "transparent"
                border.width: 2
                border.color: Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.6)
                autoFadeIn: false
                z: 100
                path: modelData
            }
        }

        // Always-visible timezone name labels, centered horizontally
        // relative to the zone center and placed just below it.
        // Clicking a label selects the corresponding timezone.
        MapItemView {
            parent: view.map
            model: root.zoneLabelModel
            delegate: MapQuickItem {
                id: tzLabelItem
                z: 200
                visible: view.map.zoomLevel > 3
                coordinate: modelData.coordinate
                anchorPoint: Qt.point((sourceItem as Text).implicitWidth / 2, 0)
                sourceItem: Text {
                    text: modelData.name
                    color: Kirigami.Theme.textColor
                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                    style: Text.Outline
                    styleColor: Kirigami.Theme.backgroundColor
                    horizontalAlignment: Text.AlignHCenter
                }
                TapHandler {
                    onTapped: root.selectedTimeZone = modelData.tzid
                }
            }
        }

        MapQuickItem {
            id: selectionDot
            parent: view.map
            anchorPoint: Qt.point(Kirigami.Units.iconSizes.large / 2, Kirigami.Units.iconSizes.large)
            z: 200
            visible: false
            sourceItem: Kirigami.Icon {
                width: Kirigami.Units.iconSizes.large
                height: Kirigami.Units.iconSizes.large
                source: "mark-location-symbolic"
                color: Kirigami.Theme.negativeTextColor
            }
        }

        function updateSelection(): void {
            selectionDot.visible = false

            const tzData = root.bandData.tzids[root.selectedTimeZone]
            if (!tzData) return

            selectionDot.coordinate = QtPositioning.coordinate(tzData.centerLat, tzData.centerLon)
            selectionDot.visible = true
        }

        Connections {
            target: root
            function onSelectedTimeZoneChanged() {
                view.updateSelection()
            }
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            anchors {
                right: parent.right
                rightMargin: Kirigami.Units.largeSpacing
                bottom: parent.bottom
                bottomMargin: Kirigami.Units.largeSpacing
            }

            QQC2.Button {
                id: zoomInButton

                text: i18ndc("plasmashellprivateplugin", "@action:button", "Zoom in")
                display: QQC2.AbstractButton.IconOnly
                icon.name: "zoom-in-map-symbolic"
                enabled: view.map.zoomLevel < view.map.maximumZoomLevel

                onClicked: view.map.zoomLevel += 0.5
                onDoubleClicked: view.map.zoomLevel += 0.5

                QQC2.ToolTip.text: zoomInButton.text
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }

            QQC2.Button {
                id: zoomOutButton

                text: i18ndc("plasmashellprivateplugin", "@action:button", "Zoom out")
                display: QQC2.AbstractButton.IconOnly
                icon.name: "zoom-out-map-symbolic"
                enabled: view.map.zoomLevel > view.map.minimumZoomLevel

                onClicked: view.map.zoomLevel -= 0.5
                onDoubleClicked: view.map.zoomLevel -= 0.5

                QQC2.ToolTip.text: zoomOutButton.text
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }

        Kirigami.ContextualHelpButton {
            anchors {
                left: parent.left
                leftMargin: Kirigami.Units.largeSpacing
                bottom: parent.bottom
                bottomMargin: Kirigami.Units.largeSpacing
            }
            toolTipText: i18ndc("plasmashellprivateplugin", "@info:tooltip", "The boundaries shown on this map only represent time zone boundaries, not country borders.")
        }

        Components.FloatingToolBar {

            anchors.bottom: parent.bottom
            anchors.bottomMargin: Kirigami.Units.gridUnit
            anchors.horizontalCenter: parent.horizontalCenter

            width: mainLayout.implicitWidth + Kirigami.Units.largeSpacing * 2
            height: mainLayout.implicitHeight + Kirigami.Units.largeSpacing * 2

            contentItem: RowLayout {
                id: mainLayout
                anchors.centerIn: parent
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    text: i18ndc("plasmashellprivateplugin", "@label:listbox In the context of time zone selection", "Region:")
                    textFormat: Text.PlainText
                }

                QQC2.ComboBox {
                    id: regionComboBox
                    model: [chooseText, ...regionsModel]

                    property string chooseText: i18ndc("plasmashellprivateplugin", "Placeholder for empty time zone combobox selector", "Choose…")

                    displayText: currentText

                    Accessible.name: i18nd("plasmashellprivateplugin", "Timezone region selector")

                    Connections {
                        target: root
                        function onSelectedTimeZoneChanged() {
                            regionComboBox.currentIndex = Math.max(regionComboBox.model.indexOf(root.split(root.selectedTimeZone)[0]), 0)
                        }
                    }

                    onActivated: {
                        if (regionComboBox.currentText === chooseText) return;
                        if (regionComboBox.currentText !== root.split(root.selectedTimeZone)[0]) {
                            let locations = root.areasByRegion[regionComboBox.currentText]
                            locationComboBox.forceActiveFocus();
                            locationComboBox.model = locations
                            locationComboBox.popup.visible = true
                        }
                    }
                }

                QQC2.Label {
                    text: i18ndc("plasmashellprivateplugin", "@label:listbox In the context of time zone selection", "Time zone:")
                    visible: locationComboBox.visible
                    textFormat: Text.PlainText
                }

                QQC2.ComboBox {
                    id: locationComboBox

                    visible: regionComboBox.currentText !== regionComboBox.chooseText
                    displayText: currentText

                    Accessible.name: i18nd("plasmashellprivateplugin", "Timezone location selector")

                    Connections {
                        target: root
                        function onSelectedTimeZoneChanged() {
                            let [prefix, suffix] = root.split(root.selectedTimeZone)
                            locationComboBox.model = root.areasByRegion[prefix]
                            locationComboBox.currentIndex = locationComboBox.model.indexOf(suffix)
                        }
                    }

                    onActivated: {
                        root.selectedTimeZone = root.technical(regionComboBox.currentText) + '/' + root.technical(locationComboBox.currentText)
                    }
                }
            }
        }
    }

}
