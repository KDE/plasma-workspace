/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Shapes 1.12

import org.kde.kirigami 2.12 as Kirigami

Item {
    property alias colorTimer: colorChangeBackTimer
    
    Timer {
        id: colorChangeBackTimer
        interval: 500
        onTriggered: {
            iconColorAnimation.to = Kirigami.Theme.textColor
            iconColorAnimation.start();
            circleColorAnimation.to = Kirigami.Theme.highlightColor
            circleColorAnimation.start();
        }
    }
    
    Connections {
        target: fingerprintModel
        function onScanSuccess() {
            iconColorAnimation.to = Kirigami.Theme.highlightColor
            iconColorAnimation.start();
            colorChangeBackTimer.restart();
        }
        function onScanFailure() {
            iconColorAnimation.to = Kirigami.Theme.negativeTextColor
            iconColorAnimation.start();
            colorChangeBackTimer.restart();
        }
        function onScanComplete() {
            iconColorAnimation.to = Kirigami.Theme.positiveTextColor
            iconColorAnimation.start();
        }
    }
    
    Kirigami.Icon {
        id: fingerprintEnrollFeedback
        source: "fingerprint"

        anchors.fill: parent
        anchors.topMargin: Kirigami.Units.smallSpacing
        anchors.leftMargin: Kirigami.Units.smallSpacing
        anchors.rightMargin: Kirigami.Units.smallSpacing
        anchors.bottomMargin: Kirigami.Units.smallSpacing

        ColorAnimation on color {
            id: iconColorAnimation
            easing.type: Easing.InOutQuad
            duration: 150
        }
    }

    Shape {
        id: progressCircle
        layer.enabled: true
        layer.samples: 40
        anchors.fill: parent
        
        property int rawAngle: fingerprintModel.enrollProgress * 360
        property int renderedAngle: 0
        NumberAnimation on renderedAngle {
            id: elapsedAngleAnimation
            easing.type: Easing.InOutQuad
            duration: 500
        }
        onRawAngleChanged: {
            elapsedAngleAnimation.to = rawAngle;
            elapsedAngleAnimation.start();
        }
        
        ShapePath {
            strokeColor: "lightgrey"
            fillColor: "transparent"
            strokeWidth: 3
            capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: progressCircle.width / 2
                centerY: progressCircle.height / 2
                radiusX: (progressCircle.width - Kirigami.Units.gridUnit) / 2
                radiusY: radiusX
                startAngle: 0
                sweepAngle: 360
            }
        }
        ShapePath {
            strokeColor: Kirigami.Theme.highlightColor
            fillColor: "transparent"
            strokeWidth: 3
            capStyle: ShapePath.RoundCap

            ColorAnimation on strokeColor {
                id: circleColorAnimation
                easing.type: Easing.InOutQuad
                duration: 200
            }

            PathAngleArc {
                centerX: progressCircle.width / 2
                centerY: progressCircle.height / 2
                radiusX: (progressCircle.width - Kirigami.Units.gridUnit) / 2
                radiusY: radiusX
                startAngle: -90
                sweepAngle: progressCircle.renderedAngle
            }
        }
    }
}
