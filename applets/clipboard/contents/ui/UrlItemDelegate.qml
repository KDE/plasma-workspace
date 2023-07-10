/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.kirigami 2.20 as Kirigami

Item {
    id: previewItem
    height: Kirigami.Units.gridUnit * 4 + Kirigami.Units.smallSpacing * 2

    Drag.active: dragHandler.active
    Drag.dragType: Drag.Automatic
    Drag.supportedActions: Qt.CopyAction
    Drag.mimeData: {
        "text/uri-list": DisplayRole.split(" "),
    }

    ListView {
        id: previewList
        model: DisplayRole.split(" ", maximumNumberOfPreviews)
        property int itemWidth: Kirigami.Units.gridUnit * 4
        property int itemHeight: Kirigami.Units.gridUnit * 4
        interactive: false

        spacing: Kirigami.Units.smallSpacing
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
                color: Kirigami.Theme.textColor
                opacity: 0.6
                height: Kirigami.Units.gridUnit
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
            }
            PlasmaComponents3.Label {
                font: Kirigami.Theme.smallestFont
                color: Kirigami.Theme.backgroundColor
                maximumLineCount: 1
                anchors {
                    verticalCenter: overlay.verticalCenter
                    left: overlay.left
                    right: overlay.right
                    leftMargin: Kirigami.Units.smallSpacing
                    rightMargin: Kirigami.Units.smallSpacing
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
            margins: Kirigami.Units.smallSpacing

        }
        verticalAlignment: Text.AlignBottom
        horizontalAlignment: Text.AlignCenter
        font: Kirigami.Theme.smallFont
    }
}
