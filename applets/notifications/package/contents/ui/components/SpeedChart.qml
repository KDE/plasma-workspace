/*
    SPDX-FileCopyrightText: 2024 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQml

import org.kde.kirigami as Kirigami
import org.kde.coreaddons as KCoreAddons

import QtCharts 2.0
import QtQuick.Controls

Item {
    id: root

    property ModelInterface modelInterface

    property int maxSpeed: 0
    property int minSpeed: 0
    property int averageSpeed

    property LineSeries speedSerie : LineSeries {}

    Component.onCompleted: () => {
        speedSerie.append(0, modelInterface.jobDetails.processedBytes / (1000 * modelInterface.jobDetails.elapsedTime));
    }

    Connections {
        target: modelInterface.jobDetails

        function onProcessedBytesChanged() {
            var speedMBperSec = modelInterface.jobDetails.speed / 1000000;

            speedSerie.append(modelInterface.jobDetails.processedBytes, speedMBperSec);

            if (speedMBperSec > maxSpeed) {
                maxSpeed = speedMBperSec;
            }

            if (minSpeed == 0 || minSpeed > speedMBperSec ) {
                minSpeed = speedMBperSec;
            }

            if (averageLine.count > 0) {
                averageLine.removePoints(0, 2);
            }
            averageSpeed = modelInterface.jobDetails.processedBytes / (1000 * modelInterface.jobDetails.elapsedTime);
            averageLine.append(valueAxis.min, averageSpeed);
            averageLine.append(valueAxis.max, averageSpeed);
        }
    }

    ToolTip {
        id: tooltip
        contentItem: Text {
            color: Kirigami.Theme.textColor
            text: tooltip.text
        }
        background: Rectangle {
            color: Kirigami.Theme.backgroundColor
            border.color: Kirigami.Theme.alternateBackgroundColor
        }
    }

    ChartView {
        id: chart
        anchors.fill: root

        margins { top: 0; bottom: 0; left: 0; right: 0 }

        antialiasing: true
        backgroundColor: Kirigami.Theme.backgroundColor

        legend.visible: false

        ValueAxis {
            id: valueAxis
            min: 0
            max: root.modelInterface.jobDetails.totalBytes
            labelsVisible: false
        }

        ValueAxis {
            id: dataAxis
            min: Math.max(Math.min(root.averageSpeed - 5, minSpeed - 5), 0)
            max: Math.max(root.averageSpeed, root.maxSpeed) + 2
            labelsColor: Kirigami.Theme.textColor
            shadesVisible: false
            labelFormat: i18ndc("plasma_applet_org.kde.plasma.notifications", "MBytes per second as printf format", "%u MiB/s")
        }

        AreaSeries {
            axisX: valueAxis
            axisY: dataAxis
            useOpenGL: true

            upperSeries: root.speedSerie
            color: Kirigami.Theme.highlightColor

            onHovered: (point, hovered) => {
                if (!hovered) {
                    tooltip.visible = false;
                    return;
                }
                var p = chart.mapToPosition(point)
                var text = i18ndc("plasma_applet_org.kde.plasma.notifications", "Bytes per second", "%1/s",
                                            KCoreAddons.Format.formatByteSize(point.y * 1000000))
                tooltip.x = p.x
                tooltip.y = p.y - tooltip.height
                tooltip.text = text
                tooltip.visible = true
            }
        }

        LineSeries {
            id: averageLine
            useOpenGL: true

            color: "red"
            width: 3

            axisX: valueAxis
            axisY: dataAxis

            onHovered: (point) => {
                var p = chart.mapToPosition(point)
                var text = i18ndc("plasma_applet_org.kde.plasma.notifications", "Average Bytes per second", "Average: %1/s",
                                            KCoreAddons.Format.formatByteSize(averageSpeed * 1000000))
                tooltip.x = p.x
                tooltip.y = p.y - tooltip.height
                tooltip.text = text
                tooltip.visible = true
            }
        }
    }
}
