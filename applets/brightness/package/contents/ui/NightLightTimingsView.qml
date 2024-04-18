/*
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root

    property int dayTransitionOn
    property int dayTransitionOff
    property int nightTransitionOn
    property int nightTransitionOff

    property bool alwaysOn: false
    property int dayTemperature: 6500
    property int nightTemperature: 4200

    readonly property bool show24h: true // TODO: Get from user's prefernces
    readonly property bool singleColor: root.alwaysOn || !root.enabled

    Rectangle {
        id: view

        Layout.fillWidth: true
        Layout.preferredHeight: Kirigami.Units.gridUnit * 2
        // Align the edges with the center of the first and last labels
        Layout.leftMargin: axis.width / 50 - border.width
        Layout.rightMargin: Layout.leftMargin
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.2)
        radius: 3

        gradient: root.singleColor ? null : grad
        // The color only get used when there's no gradient
        color: root.enabled ? grad.nightColor : Kirigami.Theme.backgroundColor

        Gradient {
            id: grad
            orientation: Gradient.Horizontal

            readonly property color dayColor: colorForTemp(root.dayTemperature)
            readonly property color nightColor: colorForTemp(root.nightTemperature)
            readonly property color edgeColor: (root.nightTransitionOn < root.dayTransitionOff) ? dayColor : nightColor

            GradientStop { position: 0; color: grad.edgeColor }
            GradientStop { position: dayTransitionOn/1440; color: grad.nightColor }
            GradientStop { position: dayTransitionOff/1440; color: grad.dayColor }
            GradientStop { position: nightTransitionOn/1440; color: grad.dayColor }
            GradientStop { position: nightTransitionOff/1440; color: grad.nightColor }
            GradientStop { position: 1; color: grad.edgeColor }
        }

        Repeater {
            model: (root.singleColor) ? [ { left: 0, right: 1440, isNight: true } ] : [
                { left: 0, right: Math.min(root.dayTransitionOff, root.nightTransitionOn), isNight: root.dayTransitionOn < root.nightTransitionOn },
                { left: root.dayTransitionOff, right: root.nightTransitionOn, isNight: root.dayTransitionOn > root.nightTransitionOn },
                { left: Math.max(root.dayTransitionOff, root.nightTransitionOff), right: 1440, isNight: root.dayTransitionOff < root.nightTransitionOff },
            ]

            Kirigami.Icon {
                source: {
                    if (!root.enabled) {
                        return 'redshift-status-off'
                    }
                    return modelData.isNight ? 'redshift-status-on' : 'redshift-status-day'
                }
                width: Kirigami.Units.iconSizes.medium
                height: width
                x: (modelData.left + modelData.right)/2/1440 * parent.width - width/2
                y: (parent.height - height) / 2
                visible: Math.abs(modelData.right - modelData.left) > 2 * width
                color: 'black'
            }
        }
    }

    RowLayout {
        id: axis
        Layout.fillWidth: true

        QQC2.Label {
            text: timingsView.dayTransitionOn
            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            horizontalAlignment: Text.AlignRight
            color: Kirigami.Theme.textColor
        }
    }

    function isThemeDark() {
        const bg = Kirigami.Theme.backgroundColor;
        const gray = (bg.r + bg.g + bg.b) / 3;
        return (gray < 192);
    }

    function colorForTemp(temp) {
        // from KWin colortemperature.h
        /**
            * Whitepoint values for temperatures at 100K intervals.
            * These will be interpolated for the actual temperature.
            * This table was provided by Ingo Thies, 2013.
            * See the following file for more information:
            * https://github.com/jonls/redshift/blob/master/README-colorramp
            */
        const temp2RGB = [
            [1.00000000, 0.18172716, 0.00000000],     /* 1000K */
            [1.00000000, 0.25503671, 0.00000000],     /* 1100K */
            [1.00000000, 0.30942099, 0.00000000],     /* 1200K */
            [1.00000000, 0.35357379, 0.00000000],     /*  ...  */
            [1.00000000, 0.39091524, 0.00000000],
            [1.00000000, 0.42322816, 0.00000000],
            [1.00000000, 0.45159884, 0.00000000],
            [1.00000000, 0.47675916, 0.00000000],
            [1.00000000, 0.49923747, 0.00000000],
            [1.00000000, 0.51943421, 0.00000000],
            [1.00000000, 0.54360078, 0.08679949],     /* 2000K */
            [1.00000000, 0.56618736, 0.14065513],
            [1.00000000, 0.58734976, 0.18362641],
            [1.00000000, 0.60724493, 0.22137978],
            [1.00000000, 0.62600248, 0.25591950],
            [1.00000000, 0.64373109, 0.28819679],
            [1.00000000, 0.66052319, 0.31873863],
            [1.00000000, 0.67645822, 0.34786758],
            [1.00000000, 0.69160518, 0.37579588],
            [1.00000000, 0.70602449, 0.40267128],
            [1.00000000, 0.71976951, 0.42860152],     /* 3000K */
            [1.00000000, 0.73288760, 0.45366838],
            [1.00000000, 0.74542112, 0.47793608],
            [1.00000000, 0.75740814, 0.50145662],
            [1.00000000, 0.76888303, 0.52427322],
            [1.00000000, 0.77987699, 0.54642268],
            [1.00000000, 0.79041843, 0.56793692],
            [1.00000000, 0.80053332, 0.58884417],
            [1.00000000, 0.81024551, 0.60916971],
            [1.00000000, 0.81957693, 0.62893653],
            [1.00000000, 0.82854786, 0.64816570],     /* 4000K */
            [1.00000000, 0.83717703, 0.66687674],
            [1.00000000, 0.84548188, 0.68508786],
            [1.00000000, 0.85347859, 0.70281616],
            [1.00000000, 0.86118227, 0.72007777],
            [1.00000000, 0.86860704, 0.73688797],     /* 4500K */
            [1.00000000, 0.87576611, 0.75326132],
            [1.00000000, 0.88267187, 0.76921169],
            [1.00000000, 0.88933596, 0.78475236],
            [1.00000000, 0.89576933, 0.79989606],
            [1.00000000, 0.90198230, 0.81465502],     /* 5000K */
            [1.00000000, 0.90963069, 0.82838210],
            [1.00000000, 0.91710889, 0.84190889],
            [1.00000000, 0.92441842, 0.85523742],
            [1.00000000, 0.93156127, 0.86836903],
            [1.00000000, 0.93853986, 0.88130458],
            [1.00000000, 0.94535695, 0.89404470],
            [1.00000000, 0.95201559, 0.90658983],
            [1.00000000, 0.95851906, 0.91894041],
            [1.00000000, 0.96487079, 0.93109690],
            [1.00000000, 0.97107439, 0.94305985],     /* 6000K */
            [1.00000000, 0.97713351, 0.95482993],
            [1.00000000, 0.98305189, 0.96640795],
            [1.00000000, 0.98883326, 0.97779486],
            [1.00000000, 0.99448139, 0.98899179],
            [1.00000000, 1.00000000, 1.00000000]      /* 6500K */
        ]
        var rgb = temp2RGB[(temp - 1000)/100];
        var col = Qt.rgba(rgb[0], rgb[1], rgb[2], 1);
        if (isThemeDark() && col.hsvSaturation < 0.7) {
            col.hsvSaturation += 0.3;
        }
        return col;
    }
}
