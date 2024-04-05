/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

Kirigami.FormLayout {

    /* Equirectangular projection maps (x, y) to (lat, long) cleanly */
    function longitudeToX(lon) {
        return (lon + 180) * (mapImage.width  / 360);
    }
    function latitudeToY(lat) {
        return (90 - lat) * (mapImage.height / 180);
    }

    function xToLongitude(x) {
        return (x / (mapImage.width  / 360)) - 180;
    }
    function yToLatitude(y) {
        return 90 - (y / (mapImage.height / 180));
    }

    ColumnLayout {
        QQC2.Label {
            id: mapLabel
            wrapMode: Text.Wrap
            Layout.maximumWidth: mapRect.width
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignHCenter
            text: Kirigami.Settings.tabletMode
                ? i18nc("@label:chooser Tap should be translated to mean touching using a touchscreen", "Tap to choose your location on the map.")
                : i18nc("@label:chooser Click should be translated to mean clicking using a mouse", "Click to choose your location on the map.")
            textFormat: Text.PlainText
            font: Kirigami.Theme.smallFont
        }

        Kirigami.ShadowedRectangle {
            id: mapRect
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: Kirigami.Units.gridUnit * 30
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            implicitHeight: Kirigami.Units.gridUnit * 15
            Layout.maximumHeight: Kirigami.Units.gridUnit * 15
            radius: Kirigami.Units.cornerRadius
            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            color: Kirigami.Theme.backgroundColor
            shadow.xOffset: 0
            shadow.yOffset: 2
            shadow.size: 10
            shadow.color: Qt.rgba(0, 0, 0, 0.3)

            property double zoomFactor: 1.2;
            property double currentScale: 1.0;

            /* Zoom in/out buttons */
            RowLayout {
                anchors {
                    right: parent.right
                    rightMargin: Kirigami.Units.smallSpacing*2
                    bottom: parent.bottom
                    bottomMargin: Kirigami.Units.smallSpacing*2
                }

                // Always show above thumbnail content
                z: 9999

                QQC2.Button {
                    // HACK: using list-add and list-remove for more obvious/standard zoom icons till we change the Breeze ones
                    // https://bugs.kde.org/show_bug.cgi?id=435671
                    text: i18n("Zoom in")
                    display: QQC2.AbstractButton.IconOnly
                    icon.name: kcm.isIconThemeBreeze() ? "list-add" : "zoom-in"
                    activeFocusOnTab: false
                    onClicked: {
                        if (mapRect.currentScale < 5) {
                            let centrePos = { x: mapImage.width / 2, y: mapImage.height / 2 };
                            var realX = centrePos.x * mapZoom.xScale
                            var realY = centrePos.y * mapZoom.yScale
                            mapFlick.contentX -= (1-mapRect.zoomFactor)*realX
                            mapFlick.contentY -= (1-mapRect.zoomFactor)*realY
                            mapRect.currentScale *= mapRect.zoomFactor;
                        }
                    }
                    onDoubleClicked: {
                        onClicked();
                    }
                    QQC2.ToolTip {
                        text: parent.text
                    }
                }

                QQC2.Button {
                    // HACK: using list-add and list-remove for more obvious/standard zoom icons till we change the Breeze ones
                    // https://bugs.kde.org/show_bug.cgi?id=435671
                    text: i18n("Zoom in")
                    display: QQC2.AbstractButton.IconOnly
                    icon.name: kcm.isIconThemeBreeze() ? "list-remove" : "zoom-out"
                    activeFocusOnTab: false
                    onClicked: {
                        if (mapRect.currentScale > 1) {
                            let centrePos = { x: mapImage.width / 2, y: mapImage.height / 2 };
                            var realX = centrePos.x * mapZoom.xScale
                            var realY = centrePos.y * mapZoom.yScale
                            mapFlick.contentX -= (1-(1/mapRect.zoomFactor))*realX
                            mapFlick.contentY -= (1-(1/mapRect.zoomFactor))*realY
                            mapRect.currentScale *= (1/mapRect.zoomFactor)
                        }
                    }
                    onDoubleClicked: {
                        onClicked();
                    }
                    QQC2.ToolTip {
                        text: parent.text
                    }
                }
            }

            Flickable {
                id: mapFlick
                anchors {
                    fill: parent
                    margins: Kirigami.Units.smallSpacing
                }
                contentWidth: mapImage.width * mapRect.currentScale
                contentHeight: mapImage.height * mapRect.currentScale

                clip: true
                Image {
                    id: mapImage
                    source: "worldmap.png" // loaded using QRC
                    transform: Scale {
                        id: mapZoom
                        xScale: mapRect.currentScale
                        yScale: mapRect.currentScale
                    }

                    Kirigami.Icon {
                        z: 9999
                        readonly property double rawX: longitudeToX(kcm.nightLightSettings.longitudeFixed)
                        readonly property double rawY: latitudeToY(kcm.nightLightSettings.latitudeFixed)
                        x: rawX - (width/2)/mapRect.currentScale
                        y: rawY - (height - 4)/mapRect.currentScale
                        width: Kirigami.Units.iconSizes.medium
                        height: Kirigami.Units.iconSizes.medium
                        source: "mark-location"
                        color: "#232629"
                        transform: Scale {
                            xScale: 1/mapRect.currentScale
                            yScale: 1/mapRect.currentScale
                        }
                    }

                    TapHandler {
                        onTapped: event => {
                            const clickPos = event.position;
                            kcm.nightLightSettings.longitudeFixed = xToLongitude(clickPos.x);
                            kcm.nightLightSettings.latitudeFixed = yToLatitude(clickPos.y);
                        }
                    }

                    WheelHandler {
                        acceptedModifiers: Qt.ControlModifier
                        onWheel: event => {
                            let wheelPos = mapImage.mapFromItem(root, point.scenePosition);
                            var realX = wheelPos.x * mapZoom.xScale
                            var realY = wheelPos.y * mapZoom.yScale
                            let clicks = event.angleDelta.y / 120;
                            if (clicks > 0 && mapRect.currentScale < 5) {
                                mapFlick.contentX -= (1-mapRect.zoomFactor)*realX
                                mapFlick.contentY -= (1-mapRect.zoomFactor)*realY
                                mapRect.currentScale *= mapRect.zoomFactor;
                            } else if (clicks < 0 && mapRect.currentScale > 1) {
                                mapFlick.contentX -= (1-(1/mapRect.zoomFactor))*realX
                                mapFlick.contentY -= (1-(1/mapRect.zoomFactor))*realY
                                mapRect.currentScale *= (1/(mapRect.zoomFactor));
                            }
                        }
                    }

                    Component.onCompleted: {
                        width = mapFlick.width;
                        height = mapFlick.height;
                    }

                }
            }
        }

        TextEdit {
            id: mapAttributionLabel
            textFormat: TextEdit.RichText
            wrapMode: Text.Wrap
            readOnly: true
            color: Kirigami.Theme.textColor
            selectedTextColor: Kirigami.Theme.highlightedTextColor
            selectionColor: Kirigami.Theme.highlightColor
            font: Kirigami.Theme.smallFont
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.maximumWidth: mapRect.width
            Layout.alignment: Qt.AlignHCenter
            text: xi18nc("@info", "Modified from <link url='https://commons.wikimedia.org/wiki/File:World_location_map_(equirectangular_180).svg'>World location map</link> by TUBS / Wikimedia Commons / <link url='https://creativecommons.org/licenses/by-sa/3.0'>CC BY-SA 3.0</link>")
            onLinkActivated: (url) => Qt.openUrlExternally(url)
            HoverHandler {
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }

        RowLayout {
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignHCenter

            QQC2.Label {
                text: i18nc("@label: textbox", "Latitude:")
                textFormat: Text.PlainText
            }
            Connections {
                target: kcm.nightLightSettings
                function onLatitudeFixedChanged() {
                    latitudeFixedField.backend = kcm.nightLightSettings.latitudeFixed;
                }
                function onLongitudeFixedChanged() {
                    longitudeFixedField.backend = kcm.nightLightSettings.longitudeFixed;
                }
            }
            NumberField {
                id: latitudeFixedField
                validator: DoubleValidator {bottom: -90; top: 90; decimals: 10}
                backend: kcm.nightLightSettings.latitudeFixed
                onBackendChanged: {
                    kcm.nightLightSettings.latitudeFixed = backend;
                }
                KCM.SettingStateBinding {
                    configObject: kcm.nightLightSettings
                    settingName: "LatitudeFixed"
                    extraEnabledConditions: kcm.nightLightSettings.active
                }
            }

            QQC2.Label {
                text: i18nc("@label: textbox", "Longitude:")
                textFormat: Text.PlainText
            }
            NumberField {
                id: longitudeFixedField
                validator: DoubleValidator {bottom: -180; top: 180; decimals: 10}
                backend: kcm.nightLightSettings.longitudeFixed
                onBackendChanged: {
                    kcm.nightLightSettings.longitudeFixed = backend;
                }
                KCM.SettingStateBinding {
                    configObject: kcm.nightLightSettings
                    settingName: "LongitudeFixed"
                    extraEnabledConditions: kcm.nightLightSettings.active
                }
            }
        }
    }
}
