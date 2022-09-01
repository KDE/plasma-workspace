/*
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

import org.kde.kirigami 2.15 as Kirigami

import org.kde.private.kcms.nightcolor 1.0

Item {
    id: root
    property int sliderWidth: 500

    readonly property bool rtl: (Qt.application as Application).layoutDirection

    property bool interactive: kcm.nightColorSettings.mode === NightColorMode.Timings
    property bool sameTransitionDurations: !(kcm.nightColorSettings.mode === NightColorMode.Automatic || kcm.nightColorSettings.mode === NightColorMode.Location)

    implicitHeight: Kirigami.Units.gridUnit * 2
    implicitWidth: sliderWidth

    function isThemeDark() {
        var bg = Kirigami.Theme.backgroundColor;
        var gray = (bg.r + bg.g + bg.b)/3;
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

    readonly property color day: colorForTemp(kcm.nightColorSettings.dayTemperature || 6500)
    readonly property color night: colorForTemp(kcm.nightColorSettings.nightTemperature)

    property alias startTime: startTimeSlider.value
    property alias startFinishTime: startFinTimeSlider.value
    property alias endTime: endTimeSlider.value
    property alias endFinishTime: endFinTimeSlider.value

    property int endTimeBackend: {
        if (kcm.nightColorSettings.mode === NightColorMode.Automatic || kcm.nightColorSettings.mode === NightColorMode.Location) {
            var latitude = kcm.nightColorSettings.mode === NightColorMode.Automatic && (locator !== undefined) ? locator.latitude : kcm.nightColorSettings.latitudeFixed;
            var longitude = kcm.nightColorSettings.mode === NightColorMode.Automatic && (locator !== undefined) ? locator.longitude : kcm.nightColorSettings.longitudeFixed;
            var t = sunCalc.getMorningTimings(latitude, longitude).begin;
            return t.getHours() * 60 + t.getMinutes();
        } else {
            var backend = kcm.nightColorSettings.morningBeginFixed;
            var hours = parseInt(backend.slice(0, 2), 10);
            var minutes = parseInt(backend.slice(2, 4), 10);
            return hours * 60 + minutes;
        }
    }

    property int startTimeBackend: {
        if (kcm.nightColorSettings.mode === NightColorMode.Automatic || kcm.nightColorSettings.mode === NightColorMode.Location) {
            var latitude = kcm.nightColorSettings.mode === NightColorMode.Automatic && (locator !== undefined) ? locator.latitude : kcm.nightColorSettings.latitudeFixed;
            var longitude = kcm.nightColorSettings.mode === NightColorMode.Automatic && (locator !== undefined) ? locator.longitude : kcm.nightColorSettings.longitudeFixed;
            var t = sunCalc.getEveningTimings(latitude, longitude).begin;
            return t.getHours() * 60 + t.getMinutes();
        } else {
            var backend = kcm.nightColorSettings.eveningBeginFixed;
            var hours = parseInt(backend.slice(0, 2), 10);
            var minutes = parseInt(backend.slice(2, 4), 10);
            return hours * 60 + minutes;
        }
    }

    onStartTimeBackendChanged: {
        startTime = startTimeBackend
    }
    onEndTimeBackendChanged: {
        endTime = endTimeBackend
    }

    property int startFinishTimeBackend: {
        if (kcm.nightColorSettings.mode === NightColorMode.Automatic || kcm.nightColorSettings.mode === NightColorMode.Location) {
            var latitude = kcm.nightColorSettings.mode === NightColorMode.Automatic && (locator !== undefined) ? locator.latitude : kcm.nightColorSettings.latitudeFixed;
            var longitude = kcm.nightColorSettings.mode === NightColorMode.Automatic && (locator !== undefined) ? locator.longitude : kcm.nightColorSettings.longitudeFixed;
            var t = sunCalc.getEveningTimings(latitude, longitude).end;
            return t.getHours() * 60 + t.getMinutes();
        } else {
            var backend = kcm.nightColorSettings.eveningBeginFixed;
            var hours = parseInt(backend.slice(0, 2), 10);
            var minutes = parseInt(backend.slice(2, 4), 10);
            return hours * 60 + minutes + kcm.nightColorSettings.transitionTime;
        }
    }

    onStartFinishTimeBackendChanged: {
        startFinishTime = startFinishTimeBackend
    }

    onStartFinishTimeChanged: {
        if (kcm.nightColorSettings.mode === NightColorMode.Timings) {
            var startTimeBackend = kcm.nightColorSettings.eveningBeginFixed;
            var hours = parseInt(startTimeBackend.slice(0, 2), 10);
            var minutes = parseInt(startTimeBackend.slice(2, 4), 10);
            kcm.nightColorSettings.transitionTime = startFinishTime - (hours * 60 + minutes);
        }
    }

    property int endFinishTimeBackend: {
        if (kcm.nightColorSettings.mode === NightColorMode.Automatic || kcm.nightColorSettings.mode === NightColorMode.Location) {
            var latitude = kcm.nightColorSettings.mode === NightColorMode.Automatic && (locator !== undefined) ? locator.latitude : kcm.nightColorSettings.latitudeFixed;
            var longitude = kcm.nightColorSettings.mode === NightColorMode.Automatic && (locator !== undefined) ? locator.longitude : kcm.nightColorSettings.longitudeFixed;
            var t = sunCalc.getMorningTimings(latitude, longitude).end;
            return t.getHours() * 60 + t.getMinutes();
        } else {
            var backend = kcm.nightColorSettings.morningBeginFixed;
            var hours = parseInt(backend.slice(0, 2), 10);
            var minutes = parseInt(backend.slice(2, 4), 10);
            return hours * 60 + minutes + kcm.nightColorSettings.transitionTime;
        }
    }

    onEndFinishTimeBackendChanged: {
        endFinishTime = endFinishTimeBackend
    }

    onEndFinishTimeChanged: {
        if (kcm.nightColorSettings.mode === NightColorMode.Timings) {
            var endTimeBackend = kcm.nightColorSettings.morningBeginFixed;
            var hours = parseInt(endTimeBackend.slice(0, 2), 10);
            var minutes = parseInt(endTimeBackend.slice(2, 4), 10);
            kcm.nightColorSettings.transitionTime = endFinishTime - (hours * 60 + minutes);
        }
    }

    signal userChangedStartTime(value: int)
    signal userChangedEndTime(value: int)

    function changeStartTime(value) {
        startTimeSlider.changeValue(value)
    }
    function changeEndTime(value) {
        endTimeSlider.changeValue(value)
    }

    function prettyTime(minutes) {
        var d = new Date();
        d.setHours(Math.trunc(minutes/60));
        d.setMinutes(minutes%60);
        return d.toLocaleString(Qt.locale(), "hh:mm");
    }

    Accessible.description: i18nc("@info Night Color starting, ending, and transition times in hh:mm format",
        "Night Color begins changing at %1 and is fully changed by %2. Night Color begins changing back at %3 and ends at %4.",
        prettyTime(startTime), prettyTime(startFinishTime),
        prettyTime(endTime), prettyTime(endFinishTime))

    HandleOnlySlider {
        id: startTimeSlider
        from: 0
        to: 1440 // 24h in min
        interactive: root.interactive
        value: -1
        pointerLabel: i18nc("@info night color starts to take effect", "Color starts changing at %1", prettyTime(value))
        handleToolTip: "When Night Color starts to take effect"
        stepSize: 1
        live: true
        onUserChangedValue: {
            var hours = Math.trunc(value/60);
            var minutes = value%60;
            kcm.nightColorSettings.eveningBeginFixed = String(hours).padStart(2, "0") + String(minutes).padStart(2, "0")
        }
        overlapping: {
            if (endTimeSlider.visualPosition < startTimeSlider.visualPosition) {
                return (endTimeSlider.handle.x + endTimeSlider.handle.lblWidth/2) > (startTimeSlider.handle.x - startTimeSlider.handle.lblWidth/2)
            } else {
                return (startTimeSlider.handle.x + startTimeSlider.handle.lblWidth/2) > (endTimeSlider.handle.x - endTimeSlider.handle.lblWidth/2)
            }
        }
        pointerOnBottom: false
        width: sliderWidth
    }
    HandleOnlySlider {
        id: endTimeSlider
        from: 0
        to: 1440 // 24h in min
        value: -1
        interactive: root.interactive
        onUserChangedValue: {
            var hours = Math.trunc(value/60);
            var minutes = value%60;
            kcm.nightColorSettings.morningBeginFixed = String(hours).padStart(2, "0") + String(minutes).padStart(2, "0")
        }
        pointerLabel: i18nc("@info night color starts to end", "Color starts changing back at %1", prettyTime(value))
        handleToolTip: "When Night Color starts ending"
        stepSize: 1
        live: true
        pointerOnBottom: false
        width: sliderWidth
    }
    HandleOnlySlider {
        id: startFinTimeSlider
        from: 0
        to: 1440 // 24h in min
        interactive: root.interactive
        minDrag: rtl ? 0.0 : ((value - startTimeSlider.value >= 0) ? startTimeSlider.visualPosition : 0.0)
        maxDrag: rtl ? ((value - startTimeSlider.value >= 0) ? startTimeSlider.visualPosition : 0.0) : 1.0
        pointerLabel: i18nc("@info night color has fully taken effect", "Color fully changed at %1", prettyTime(value))
        onUserChangedValue: {
            startFinTimeSlider.value = value
        }
        overlapping: {
            if (endFinTimeSlider.visualPosition < startFinTimeSlider.visualPosition) {
                return (endFinTimeSlider.handle.x + endFinTimeSlider.handle.lblWidth/2) > (startFinTimeSlider.handle.x - startFinTimeSlider.handle.lblWidth/2)
            } else {
                return (startFinTimeSlider.handle.x + startFinTimeSlider.handle.lblWidth/2) > (endFinTimeSlider.handle.x - endFinTimeSlider.handle.lblWidth/2)
            }
        }
        handleToolTip: "When Night Color is completely applied"
        stepSize: 1
        live: true
        width: sliderWidth
    }
    HandleOnlySlider {
        id: endFinTimeSlider
        from: 0
        to: 1440 // 24h in min
        interactive: root.interactive
        minDrag: rtl ? 0.0 : ((value - endTimeSlider.value >= 0) ? endTimeSlider.visualPosition : 0.0)
        maxDrag: rtl ? ((value - endTimeSlider.value >= 0) ? endTimeSlider.visualPosition : 0.0) : 1.0
        pointerLabel: i18nc("@info night color has fully taken effect", "Color fully changed back at %1", prettyTime(value))
        onUserChangedValue: {
            endFinTimeSlider.value = value
        }
        handleToolTip: "When Night Color completely ends"
        stepSize: 1
        live: true
        width: sliderWidth
    }

    Rectangle {
        id: visualizer
        width: sliderWidth
        height: 10
        radius: 5

        border.color: Qt.rgba(0, 0, 0, 0.5)
        border.width: 1

        LinearGradient {
            anchors.fill: parent
            source: visualizer
            start: Qt.point(0, 0)
            end: Qt.point(sliderWidth, 0)
            gradient: Gradient {
                GradientStop { position: endTimeSlider.visualPosition; color: night }
                GradientStop { position: endFinTimeSlider.visualPosition + (rtl ? -0.01 : +0.01); color: day }
                GradientStop { position: startTimeSlider.visualPosition; color: day }
                GradientStop { position: startFinTimeSlider.visualPosition + (rtl ? -0.01 : +0.01); color: night }
                GradientStop { position: 1.0; color: (startTimeSlider.visualPosition > endTimeSlider.visualPosition) ? (rtl ? day : night) : (rtl ? night : day) }
            }
        }
    }
}
