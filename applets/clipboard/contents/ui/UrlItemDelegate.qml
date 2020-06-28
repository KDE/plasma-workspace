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

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

Item {
    id: previewItem
    height: units.gridUnit * 4 + units.smallSpacing * 2

    ListView {
        id: previewList
        model: DisplayRole.split(" ", maximumNumberOfPreviews)
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
            PlasmaComponents3.Label {
                font: theme.smallestFont
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
    PlasmaComponents3.Label {
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
        font: theme.smallestFont
    }
}
