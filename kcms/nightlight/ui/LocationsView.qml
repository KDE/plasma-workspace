/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

Kirigami.FormLayout {
    id: root

    /* Equirectangular projection maps (x, y) to (lat, long) cleanly */
    function longitudeToX(longitude: double): double {
        return (longitude + 180) * (mapImage.width  / 360);
    }

    function latitudeToY(latitude: double): double {
        return (90 - latitude) * (mapImage.height / 180);
    }

    function xToLongitude(x: double): double {
        return (x / (mapImage.width  / 360)) - 180;
    }

    function yToLatitude(y: double): double {
        return 90 - (y / (mapImage.height / 180));
    }

    function scaledMapCentrePosition(): point {
        const centre = Qt.point(
            mapImage.width / 2,
            mapImage.height / 2,
        );
        return Qt.point(
            centre.x * mapZoom.xScale,
            centre.y * mapZoom.yScale,
        );
    }

    function zoomIn(): void {
        if (mapRect.currentScale < 5) {
            const centre = scaledMapCentrePosition();
            mapFlick.contentX -= (1 - mapRect.zoomFactor) * centre.x;
            mapFlick.contentY -= (1 - mapRect.zoomFactor) * centre.y;
            mapRect.currentScale *= mapRect.zoomFactor;
        }
    }

    function zoomOut(): void {
        if (mapRect.currentScale > 1) {
            const centre = scaledMapCentrePosition();
            mapFlick.contentX -= (1 - (1 / mapRect.zoomFactor)) * centre.x;
            mapFlick.contentY -= (1 - (1 / mapRect.zoomFactor)) * centre.y;
            mapRect.currentScale *= 1 / mapRect.zoomFactor;
        }
    }

    signal manualLocationChanged()

    ColumnLayout {

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

            property double zoomFactor: 1.2
            property double currentScale: 1.0

            /* Zoom in/out buttons */
            RowLayout {
                anchors {
                    right: parent.right
                    rightMargin: Kirigami.Units.smallSpacing * 2
                    bottom: parent.bottom
                    bottomMargin: Kirigami.Units.smallSpacing * 2
                }

                // Always show above thumbnail content
                z: 9999

                QQC2.Button {
                    id: zoomInButton

                    text: i18n("Zoom in")
                    display: QQC2.AbstractButton.IconOnly
                    icon.name: "zoom-in-map-symbolic"
                    activeFocusOnTab: false

                    onClicked: root.zoomIn()
                    onDoubleClicked: root.zoomIn()

                    QQC2.ToolTip {
                        text: zoomInButton.text
                    }
                }

                QQC2.Button {
                    id: zoomOutButton

                    text: i18n("Zoom out")
                    display: QQC2.AbstractButton.IconOnly
                    icon.name: "zoom-out-map-symbolic"
                    activeFocusOnTab: false

                    onClicked: root.zoomOut()
                    onDoubleClicked: root.zoomOut()

                    QQC2.ToolTip {
                        text: zoomOutButton.text
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
                    source: Qt.resolvedUrl("./worldmap.png") // loaded using QRC
                    transform: Scale {
                        id: mapZoom
                        xScale: mapRect.currentScale
                        yScale: mapRect.currentScale
                    }

                    Kirigami.Icon {
                        z: 9999
                        readonly property double rawX: root.longitudeToX(root.autoLocation ? root.autoLongitude : kcm.nightLightSettings.longitudeFixed)
                        readonly property double rawY: root.latitudeToY(root.autoLocation ? root.autoLatitude : kcm.nightLightSettings.latitudeFixed)
                        x: rawX - (width / 2) / mapRect.currentScale
                        y: rawY - (height - 4) / mapRect.currentScale
                        width: Kirigami.Units.iconSizes.medium
                        height: Kirigami.Units.iconSizes.medium
                        source: "mark-location"
                        color: "#232629"
                        transform: Scale {
                            xScale: 1 / mapRect.currentScale
                            yScale: 1 / mapRect.currentScale
                        }
                    }

                    TapHandler {
                        onTapped: event => {
                            if (root.autoLocation) {
                                return;
                            }
                            const clickPos = event.position;
                            kcm.nightLightSettings.longitudeFixed = root.xToLongitude(clickPos.x);
                            kcm.nightLightSettings.latitudeFixed = root.yToLatitude(clickPos.y);
                        }
                    }

                    WheelHandler {
                        acceptedModifiers: Qt.ControlModifier
                        onWheel: event => {
                            const wheelPos = point.position;
                            const realX = wheelPos.x * mapZoom.xScale;
                            const realY = wheelPos.y * mapZoom.yScale;
                            const clicks = event.angleDelta.y / 120;
                            if (clicks > 0 && mapRect.currentScale < 5) {
                                mapFlick.contentX -= (1 - mapRect.zoomFactor) * realX;
                                mapFlick.contentY -= (1 - mapRect.zoomFactor) * realY;
                                mapRect.currentScale *= mapRect.zoomFactor;
                            } else if (clicks < 0 && mapRect.currentScale > 1) {
                                mapFlick.contentX -= (1 - (1 / mapRect.zoomFactor)) * realX;
                                mapFlick.contentY -= (1 - (1 / mapRect.zoomFactor)) * realY;
                                mapRect.currentScale *= (1 / mapRect.zoomFactor);
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
                cursorShape: mapAttributionLabel.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }

        RowLayout {
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.alignment: Qt.AlignHCenter

            Connections {
                target: kcm.nightLightSettings
                function onLatitudeFixedChanged() {
                    latitudeFixedField.backend = kcm.nightLightSettings.latitudeFixed;
                }
                function onLongitudeFixedChanged() {
                    longitudeFixedField.backend = kcm.nightLightSettings.longitudeFixed;
                }
            }

            QQC2.Label {
                text: i18nc("@label: textbox", "Latitude:")
                textFormat: Text.PlainText
            }
            NumberField {
                id: latitudeFixedField
                readOnly: root.autoLocation
                validator: DoubleValidator {
                    bottom: -90
                    top: 90
                    decimals: 10
                }
                backend: root.autoLocation ? root.autoLatitude : kcm.nightLightSettings.latitudeFixed
                onBackendChanged: {
                    if (!root.autoLocation) {
                        kcm.nightLightSettings.latitudeFixed = backend;
                    }
                }
                KCM.SettingStateBinding {
                    configObject: kcm.nightLightSettings
                    settingName: "LatitudeFixed"
                    extraEnabledConditions: kcm.nightLightSettings.active
                }
                onEditingFinished: {
                    root.manualLocationChanged();
                }
            }

            QQC2.Label {
                text: i18nc("@label: textbox", "Longitude:")
                textFormat: Text.PlainText
            }
            NumberField {
                id: longitudeFixedField
                readOnly: root.autoLocation
                validator: DoubleValidator {
                    bottom: -180
                    top: 180
                    decimals: 10
                }
                backend: root.autoLocation ? root.autoLongitude : kcm.nightLightSettings.longitudeFixed
                onBackendChanged: {
                    if (!root.autoLocation) {
                        kcm.nightLightSettings.longitudeFixed = backend;
                    }
                }
                KCM.SettingStateBinding {
                    configObject: kcm.nightLightSettings
                    settingName: "LongitudeFixed"
                    extraEnabledConditions: kcm.nightLightSettings.active
                }
                onEditingFinished: {
                    root.manualLocationChanged();
                }
            }
        }
    }
}
