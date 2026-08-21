/*
    SPDX-FileCopyrightText: 2025 Méven Car <meven@kde.org>
    SPDX-FileCopyrightText: 2025 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQml
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.coreaddons as KCoreAddons
import org.kde.notificationmanager as NotificationManager
import org.kde.plasma.components as PlasmaComponents3

import org.kde.quickcharts as Charts
import org.kde.quickcharts.controls as ChartsControls

Item {
    id: root

    property ModelInterface modelInterface
    property bool expanded

    readonly property int speed: modelInterface.jobDetails.speed
    readonly property int averageSpeed: modelInterface.jobDetails.elapsedTime > 0
        ? modelInterface.jobDetails.processedBytes / modelInterface.jobDetails.elapsedTime * 1000
        : 0

    // The job keeps the readings itself, from the moment it starts, so there is a chart to show
    // even the first time anyone looks at it.
    readonly property var speedHistory: modelInterface.jobDetails.speedHistory

    readonly property real xRange: 100

    Layout.minimumHeight: chartContainer.active ? Kirigami.Units.gridUnit * 10 :
        // Even when indeterminate, we want to reserve the height for the text, otherwise it's too tightly spaced
        progressText.implicitHeight

    Layout.fillWidth: true

    PlasmaComponents3.Label {
        id: metricsLabel
        visible: false
        font: Kirigami.Theme.smallFont
        // Measure 888.8 KiB/s
        text: i18ndc("plasma_applet_org.kde.plasma.notifications", "Bytes per second", "%1/s", 910131)
        textFormat: Text.PlainText
    }

    Loader {
        id: chartContainer

        active: root.speedHistory.length >= 2 && root.expanded
        visible: active

        anchors.fill: parent

        sourceComponent: Item {
            ChartsControls.AxisLabels {
                id: axisLabels

                anchors {
                    left: parent.left
                    top: chart.top
                    bottom: chart.bottom
                }

                width: metricsLabel.implicitWidth
                constrainToBounds: false
                direction: ChartsControls.AxisLabels.VerticalBottomTop

                delegate: PlasmaComponents3.Label {
                    text:  i18ndc("plasma_applet_org.kde.plasma.notifications", "Bytes per second", "%1/s", KCoreAddons.Format.formatByteSize(ChartsControls.AxisLabels.label))
                    font: Kirigami.Theme.smallFont
                    textFormat: Text.PlainText
                }

                source: Charts.ChartAxisSource {
                    chart: chart
                    axis: Charts.ChartAxisSource.YAxis
                    itemCount: 5
                }
            }

            ChartsControls.GridLines {
                anchors.fill: chart
                direction: ChartsControls.GridLines.Vertical
                minor.visible: false
                major.count: 3
                major.lineWidth: 1
                // Same calculation as Kirigami Separator
                major.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.4)
            }

            Charts.LineChart {
                id: chart

                anchors {
                    left: axisLabels.right
                    leftMargin: Kirigami.Units.smallSpacing
                    right: parent.right
                    top: parent.top
                    topMargin: Math.round(metricsLabel.implicitHeight / 2) + Kirigami.Units.smallSpacing
                    bottom: legend.top
                    bottomMargin: Math.round(metricsLabel.implicitHeight / 2) + Kirigami.Units.smallSpacing
                }

                xRange.from: 0
                xRange.to: root.xRange
                xRange.automatic: false

                lineWidth: 1
                interpolate: true

                valueSources: Charts.ArraySource {
                    array: root.speedHistory
                }

                nameSource: Charts.SingleValueSource {
                    value: i18nd("plasma_applet_org.kde.plasma.notifications", "Speed")
                }

                colorSource: Charts.SingleValueSource {
                    value: Kirigami.Theme.highlightColor
                }

                fillColorSource: Charts.SingleValueSource {
                    value: Qt.lighter(Kirigami.Theme.highlightColor, 1.5)
                }

                Accessible.role: Accessible.Chart
            }

            ChartsControls.Legend {
                id: legend

                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }

                chart: chart

                spacing: Kirigami.Units.largeSpacing
                delegate: RowLayout {
                    id: legendDelegate

                    required property string name
                    required property string color

                    spacing: Kirigami.Units.smallSpacing

                    ChartsControls.LegendLayout.maximumWidth: implicitWidth

                    Rectangle {
                        color: legendDelegate.color
                        width: Kirigami.Units.smallSpacing
                        height: legendLabel.height
                    }
                    PlasmaComponents3.Label {
                        id: legendLabel
                        font: Kirigami.Theme.smallFont
                        text: legendDelegate.name
                        textFormat: Text.PlainText
                    }
                    PlasmaComponents3.Label {
                        font: Kirigami.Theme.smallFont
                        text: i18ndc("plasma_applet_org.kde.plasma.notifications", "Bytes per second", "%1/s", KCoreAddons.Format.formatByteSize(root.speed))
                        textFormat: Text.PlainText
                    }
                }

                RowLayout {
                    PlasmaComponents3.Label {
                        font: Kirigami.Theme.smallFont
                        text: i18nd("plasma_applet_org.kde.plasma.notifications", "Average Speed")
                        textFormat: Text.PlainText
                    }
                    PlasmaComponents3.Label {
                        font: Kirigami.Theme.smallFont
                        text: i18ndc("plasma_applet_org.kde.plasma.notifications", "Bytes per second", "%1/s", KCoreAddons.Format.formatByteSize(root.averageSpeed))
                        textFormat: Text.PlainText
                    }
                }
            }
        }
    }

    RowLayout {
        id: progressRow
        visible: !chartContainer.visible
        anchors.fill: root
        // We want largeSpacing between the progress bar and the label
        spacing: Kirigami.Units.largeSpacing

        PlasmaComponents3.ProgressBar {
            id: progressBar

            Layout.fillWidth: true

            from: 0
            to: 100
            value: root.modelInterface.percentage
            // TODO do we actually need the window visible check? perhaps I do because it can be in popup or expanded plasmoid
            indeterminate: visible && Window.window && Window.window.visible && root.modelInterface.percentage < 1
                           && root.modelInterface.jobState === NotificationManager.Notifications.JobStateRunning
                           // is this too annoying?
                           && (root.modelInterface.jobDetails.processedBytes === 0 || root.modelInterface.jobDetails.totalBytes === 0)
                           && root.modelInterface.jobDetails.processedFiles === 0
                           //&& modelInterface.jobDetails.processedDirectories === 0
        }

        PlasmaComponents3.Label {
            id: progressText

            visible: !progressBar.indeterminate
            // the || "0" is a workaround for the fact that 0 as number is falsey, and is wrongly considered a missing argument
            // BUG: 451807
            text: i18ndc("plasma_applet_org.kde.plasma.notifications", "Percentage of a job", "%1%", root.modelInterface.percentage || "0")
            textFormat: Text.PlainText
        }
    }
}
