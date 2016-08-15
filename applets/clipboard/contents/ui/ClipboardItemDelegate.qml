/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright     2014 Sebastian Kügler <sebas@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0

import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

PlasmaComponents.ListItem {
    id: menuItem

    property alias supportsBarcodes: barcodeToolButton.visible
    property int maximumNumberOfPreviews: Math.floor(width / (units.gridUnit * 4 + units.smallSpacing))
    readonly property real gradientThreshold: (label.width - toolButtonsLayout.width) / label.width

    signal itemSelected(string uuid)
    signal remove(string uuid)
    signal edit(string uuid)
    signal barcode(string uuid)
    signal action(string uuid)

    height: Math.max(label.height, toolButtonsLayout.implicitHeight) + 2 * units.smallSpacing

    enabled: true

    onClicked: {
        menuItem.itemSelected(UuidRole);
        plasmoid.expanded = false;
    }
    onContainsMouseChanged: {
        if (containsMouse) {
            menuListView.currentIndex = index
        } else {
            menuListView.currentIndex = -1
        }
    }

    ListView.onIsCurrentItemChanged: {
        if (ListView.isCurrentItem) {
            labelMask.source = label // calculate on demand
        }
    }

    // this stuff here is used so we can fade out the text behind the tool buttons
    Item {
        id: labelMaskSource
        anchors.fill: label
        visible: false

        Rectangle {
            anchors.centerIn: parent
            rotation: LayoutMirroring.enabled ? 90 : -90 // you cannot even rotate gradients without QtGraphicalEffects
            width: parent.height
            height: parent.width

            gradient: Gradient {
                GradientStop { position: 0.0; color: "white" }
                GradientStop { position: gradientThreshold - 0.25; color: "white"}
                GradientStop { position: gradientThreshold; color: "transparent"}
                GradientStop { position: 1; color: "transparent"}
            }
        }
    }

    OpacityMask {
        id: labelMask
        anchors.fill: label
        cached: true
        maskSource: labelMaskSource
        visible: !!source && menuItem.ListView.isCurrentItem
    }

    Item {
        id: label
        height: childrenRect.height
        visible: !menuItem.ListView.isCurrentItem
        anchors {
            left: parent.left
            leftMargin: units.gridUnit / 2 - listMargins.left
            right: parent.right
            verticalCenter: parent.verticalCenter
        }
        PlasmaComponents.Label {
            anchors {
                left: parent.left
                right: parent.right
            }
            maximumLineCount: 3
            text: {
                var highlightFontTag = "<font color='" + theme.highlightColor + "'>%1</font>"

                var text = DisplayRole

                // first escape any HTML characters to prevent privacy issues
                text = text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")

                // color code leading or trailing whitespace
                // the first regex is basically "trim"
                text = text.replace(/^\s+|\s+$/g, function(match) {
                    // then inside the trimmed characters ("match") we replace each one individually
                    match = match.replace(/ /g, "␣") // space
                                 .replace(/\t/g, "↹") // tab
                                 .replace(/\n/g, "↵") // return
                    return highlightFontTag.arg(match)
                })

                return text
            }
            visible: TypeRole == 0 // TypeRole: 0: Text, 1: Image, 2: Url
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            textFormat: Text.StyledText
        }
        KQuickControlsAddons.QPixmapItem {
            id: previewPixmap
            width: parent.width
            height: Math.round(width * (nativeHeight/nativeWidth) + units.smallSpacing * 2)
            pixmap: DecorationRole
            visible: TypeRole == 1
            fillMode: KQuickControlsAddons.QPixmapItem.PreserveAspectFit
        }
        Item {
            id: previewItem
            visible: TypeRole == 2
            // visible updates recursively, our label becomes invisible when hovering, hence no visible check here
            height: TypeRole == 2 ? (units.gridUnit * 4 + units.smallSpacing * 2) : 0
            width: parent.width

            ListView {
                id: previewList
                model: TypeRole == 2 ? DisplayRole.split(" ", maximumNumberOfPreviews) : 0
                property int itemWidth: units.gridUnit * 4
                property int itemHeight: units.gridUnit * 4
                interactive: false

                spacing: units.smallSpacing
                orientation: Qt.Horizontal
                width: (itemWidth + spacing) * model.length
                anchors {
                    top: parent.top
                    left: parent.left
                    bottom: parent.bottom
                }

                delegate: Item {
                    width: previewList.itemWidth
                    height:  previewList.itemHeight
                    y: Math.round((parent.height - previewList.itemHeight) / 2)
                    clip: true

                    KQuickControlsAddons.QPixmapItem {
                        id: previewPixmap

                        anchors.centerIn: parent

                        Component.onCompleted: {
                            function result(job) {
                                if (!job.error) {
                                    pixmap = job.result.preview;
                                    previewPixmap.width = job.result.previewWidth
                                    previewPixmap.height = job.result.previewHeight
                                }
                            }
                            var service = clipboardSource.serviceForSource(UuidRole)
                            var operation = service.operationDescription("preview");
                            operation.url = modelData;
                            // We request a bigger size and then clip out a square in the middle
                            // so we get uniform delegate sizes without distortion
                            operation.previewWidth = previewList.itemWidth * 2;
                            operation.previewHeight = previewList.itemHeight * 2;
                            var serviceJob = service.startOperationCall(operation);
                            serviceJob.finished.connect(result);
                        }
                    }
                    Rectangle {
                        id: overlay
                        color: theme.textColor
                        opacity: 0.6
                        height: units.gridUnit
                        anchors {
                            left: parent.left
                            right: parent.right
                            bottom: parent.bottom
                        }
                    }
                    PlasmaComponents.Label {
                        font.pointSize: theme.smallestFont.pointSize
                        color: theme.backgroundColor
                        maximumLineCount: 1
                        anchors {
                            verticalCenter: overlay.verticalCenter
                            left: overlay.left
                            right: overlay.right
                            leftMargin: units.smallSpacing
                            rightMargin: units.smallSpacing
                        }
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        text: {
                            var u = modelData.split("/");
                            return decodeURIComponent(u[u.length - 1]);
                        }
                    }
                }
            }
            PlasmaComponents.Label {
                property int additionalItems: DisplayRole.split(" ").length - maximumNumberOfPreviews
                visible: additionalItems > 0
                opacity: 0.6
                text: i18nc("Indicator that there are more urls in the clipboard than previews shown", "+%1", additionalItems)
                anchors {
                    left: previewList.right
                    right: parent.right
                    bottom: parent.bottom
                    margins: units.smallSpacing

                }
                verticalAlignment: Text.AlignBottom
                horizontalAlignment: Text.AlignCenter
                font.pointSize: theme.smallestFont.pointSize
            }
        }
    }

    Row {
        id: toolButtonsLayout
        anchors {
            right: label.right
            verticalCenter: parent.verticalCenter
        }
        visible: menuItem.ListView.isCurrentItem

        PlasmaComponents.ToolButton {
            // TODO: only show for items supporting actions?
            iconSource: "system-run"
            tooltip: i18n("Invoke action")
            onClicked: menuItem.action(UuidRole)
        }
        PlasmaComponents.ToolButton {
            id: barcodeToolButton
            iconSource: "view-barcode"
            tooltip: i18n("Show barcode")
            onClicked: menuItem.barcode(UuidRole)
        }
        PlasmaComponents.ToolButton {
            iconSource: "document-edit"
            enabled: !clipboardSource.editing
            visible: TypeRole === 0
            tooltip: i18n("Edit contents")
            onClicked: menuItem.edit(UuidRole)
        }
        PlasmaComponents.ToolButton {
            iconSource: "edit-delete"
            tooltip: i18n("Remove from history")
            onClicked: menuItem.remove(UuidRole)
        }
    }
}
