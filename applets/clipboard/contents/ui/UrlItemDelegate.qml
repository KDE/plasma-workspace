/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

Item {
    id: previewItem
    height: PlasmaCore.Units.gridUnit * 4 + PlasmaCore.Units.smallSpacing * 2

    ListView {
        id: previewList
        model: DisplayRole.split(" ", maximumNumberOfPreviews)
        property int itemWidth: PlasmaCore.Units.gridUnit * 4
        property int itemHeight: PlasmaCore.Units.gridUnit * 4
        interactive: false

        spacing: PlasmaCore.Units.smallSpacing
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
                color: PlasmaCore.Theme.textColor
                opacity: 0.6
                height: PlasmaCore.Units.gridUnit
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
            }
            PlasmaComponents3.Label {
                font: PlasmaCore.Theme.smallestFont
                color: PlasmaCore.Theme.backgroundColor
                maximumLineCount: 1
                anchors {
                    verticalCenter: overlay.verticalCenter
                    left: overlay.left
                    right: overlay.right
                    leftMargin: PlasmaCore.Units.smallSpacing
                    rightMargin: PlasmaCore.Units.smallSpacing
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
    PlasmaComponents3.Label {
        property int additionalItems: DisplayRole.split(" ").length - maximumNumberOfPreviews
        visible: additionalItems > 0
        opacity: 0.6
        text: i18nc("Indicator that there are more urls in the clipboard than previews shown", "+%1", additionalItems)
        anchors {
            left: previewList.right
            right: parent.right
            bottom: parent.bottom
            margins: PlasmaCore.Units.smallSpacing

        }
        verticalAlignment: Text.AlignBottom
        horizontalAlignment: Text.AlignCenter
        font: PlasmaCore.Theme.smallestFont
    }
}
