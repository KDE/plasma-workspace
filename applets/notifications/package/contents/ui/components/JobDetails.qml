/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

import org.kde.coreaddons as KCoreAddons

import QtCharts 2.0
import QtQuick.Controls

GridLayout {
    id: detailsGrid

    property ModelInterface modelInterface

    columns: 2
    rowSpacing: Math.round(Kirigami.Units.smallSpacing / 2)
    columnSpacing: Kirigami.Units.smallSpacing

    property int maxSpeed: 0
    property int minSpeed: 0

    property double initialProcessedSize
    property double initialTime
    property int averageSpeed

    property LineSeries speedSerie : LineSeries {}

    Component.onCompleted: () => {
        initialProcessedSize = jobDetails.processedBytes;
        speedSerie.append(initialProcessedSize, jobDetails.speed / 1000000);
        initialTime = new Date().getTime();
    }

    // once you use Layout.column/Layout.row *all* of the items in the Layout have to use them
    Repeater {
        model: [1, 2]

        PlasmaExtras.DescriptiveLabel {
            Layout.column: 0
            Layout.row: index
            Layout.alignment: Qt.AlignTop | Qt.AlignRight
            text: modelInterface.jobDetails["descriptionLabel" + modelData] && modelInterface.jobDetails["descriptionValue" + modelData]
                ? i18ndc("plasma_applet_org.kde.plasma.notifications", "Row description, e.g. Source", "%1:", modelInterface.jobDetails["descriptionLabel" + modelData]) : ""
            font: Kirigami.Theme.smallFont
            textFormat: Text.PlainText
            visible: text !== ""
        }
    }

    Repeater {
        model: [1, 2]

        PlasmaExtras.DescriptiveLabel {
            id: descriptionValueLabel
            Layout.column: 1
            Layout.row: index
            Layout.fillWidth: true
            font: Kirigami.Theme.smallFont
            elide: Text.ElideMiddle
            textFormat: Text.PlainText
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            verticalAlignment: Text.AlignTop
            maximumLineCount: 5
            visible: text !== ""

            // Only let the label grow, never shrink, to avoid repeatedly resizing the dialog when copying many files
            onImplicitHeightChanged: {
                if (implicitHeight > Layout.preferredHeight) {
                    Layout.preferredHeight = implicitHeight;
                }
            }

            Component.onCompleted: bindText()
            function bindText() {
                text = Qt.binding(function() {
                    return modelInterface.jobDetails["descriptionValue" + modelData] || "";
                });
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onPressed: mouse => {
                    // break binding so it doesn't update while the menu is opened
                    descriptionValueLabel.text = descriptionValueLabel.text;
                    descriptionValueMenu.open(mouse.x, mouse.y)
                }
            }

            EditContextMenu {
                id: descriptionValueMenu
                target: descriptionValueLabel
                // defer re-binding until after the "Copy" action in the menu has triggered
                onClosed: Qt.callLater(descriptionValueLabel.bindText)
            }
        }
    }

    Repeater {
        model: ["Bytes", "Files", "Directories", "Items"]

        PlasmaExtras.DescriptiveLabel {
            Layout.column: 1
            Layout.row: 2 + index
            Layout.fillWidth: true
            text: {
                var processed = modelInterface.jobDetails["processed" + modelData];
                var total = modelInterface.jobDetails["total" + modelData];

                if (processed > 0 || total > 1) {
                    if (processed > 0 && total > 0 && processed <= total) {
                        switch(modelData) {
                        case "Bytes":
                            return i18ndc("plasma_applet_org.kde.plasma.notifications", "How many bytes have been copied", "%2 of %1",
                                KCoreAddons.Format.formatByteSize(total),
                                KCoreAddons.Format.formatByteSize(processed))
                        case "Files":
                            return i18ndcp("plasma_applet_org.kde.plasma.notifications", "How many files have been copied", "%2 of %1 file", "%2 of %1 files",
                                          total, processed);
                        case "Directories":
                            return i18ndcp("plasma_applet_org.kde.plasma.notifications", "How many dirs have been copied", "%2 of %1 folder", "%2 of %1 folders",
                                         total, processed);
                        case "Items":
                            return i18ndcp("plasma_applet_org.kde.plasma.notifications", "How many items (that includes files and dirs) have been copied", "%2 of %1 item", "%2 of %1 items",
                                         total, processed);
                        }
                    } else {
                        switch(modelData) {
                        case "Bytes":
                            return KCoreAddons.Format.formatByteSize(processed || total)
                        case "Files":
                            return i18ndp("plasma_applet_org.kde.plasma.notifications", "%1 file", "%1 files", (processed || total));
                        case "Directories":
                            return i18ndp("plasma_applet_org.kde.plasma.notifications", "%1 folder", "%1 folders", (processed || total));
                        case "Items":
                            return i18ndp("plasma_applet_org.kde.plasma.notifications", "%1 item", "%1 items", (processed || total));
                        }
                    }
                }

                return "";
            }
            font: Kirigami.Theme.smallFont
            textFormat: Text.PlainText
            visible: text !== ""
        }
    }

    PlasmaExtras.DescriptiveLabel {
        Layout.column: 1
        Layout.row: 2 + 4
        Layout.fillWidth: true
        text: modelInterface.jobDetails.speed > 0 ? i18ndc("plasma_applet_org.kde.plasma.notifications", "Bytes per second", "%1/s",
                                           KCoreAddons.Format.formatByteSize(modelInterface.jobDetails.speed)) : ""
        font: Kirigami.Theme.smallFont
        textFormat: Text.PlainText
        visible: text !== ""
    }

    Connections {
        target: jobDetails

        function onProcessedBytesChanged() {
            var speedMBperSec = jobDetails.speed / 1000000;
            speedSerie.append(jobDetails.processedBytes, speedMBperSec);

            if (speedMBperSec > maxSpeed) {
                maxSpeed = speedMBperSec;
            }

            if (minSpeed == 0 || minSpeed > speedMBperSec ) {
                minSpeed = speedMBperSec;
            }

            if (averageLine.count > 0) {
                averageLine.removePoints(0, 2);
            }
            averageSpeed = (jobDetails.processedBytes - initialProcessedSize) / (new Date().getTime() - initialTime) / 1000;
            averageLine.append(valueAxis.min, averageSpeed);
            averageLine.append(valueAxis.max, averageSpeed);
        }
    }

    ChartView {
        id: chart
        antialiasing: true
        Layout.column: 0
        Layout.columnSpan: 2
        Layout.row: 2 + 4 + 1
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: 400
        backgroundColor: Kirigami.Theme.alternateBackgroundColor

        legend.visible: false

        ValueAxis {
            id: valueAxis
            min: initialProcessedSize
            max: jobDetails.totalBytes
            labelsVisible: false
        }

        ValueAxis {
            id: dataAxis
            min: Math.max(Math.min(averageSpeed - 5, minSpeed - 5), 0)
            max: Math.max(averageSpeed, maxSpeed) + 2
            labelsColor: Kirigami.Theme.textColor
            shadesVisible: false
        }

        AreaSeries {
            axisX: valueAxis
            axisY: dataAxis

            upperSeries: speedSerie

            onHovered: (point) => {
                console.log("clicked", averageSpeed,  point)
                var p = chart.mapToPosition(point)
                var text = i18ndc("plasma_applet_org.kde.plasma.notifications", "Bytes per second", "%1/s",
                                           KCoreAddons.Format.formatByteSize(point.y * 1000000))
                id_tooltip.x = p.x
                id_tooltip.y = p.y - id_tooltip.height
                id_tooltip.text = text
                id_tooltip.visible = true
            }
        }

        ToolTip {
            id: id_tooltip
            contentItem: Text {
                color: "#21be2b"
                text: id_tooltip.text
            }
            background: Rectangle {
                border.color: "#21be2b"
            }
        }

        LineSeries {
            id: averageLine
            useOpenGL: true

            color: "red"

            axisX: valueAxis
            axisY: dataAxis

            onHovered: (point) => {
                var p = chart.mapToPosition(point)
                var text = i18ndc("plasma_applet_org.kde.plasma.notifications", "average Bytes per second", "average %1/s",
                                           KCoreAddons.Format.formatByteSize(averageSpeed * 1000000))
                id_tooltip.x = p.x
                id_tooltip.y = p.y - id_tooltip.height
                id_tooltip.text = text
                id_tooltip.visible = true
            }
        }
    }
}
