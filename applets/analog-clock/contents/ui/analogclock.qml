/*
    SPDX-FileCopyrightText: 2012 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013-2023 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.clock 1.0
import org.kde.plasma.workspace.calendar 2.0 as PlasmaCalendar

Item {
    id: analogclock

    width: PlasmaCore.Units.gridUnit * 15
    height: PlasmaCore.Units.gridUnit * 15

    readonly property string currentTime: Qt.formatTime(clockSource.dateTime,  Qt.locale().timeFormat(Locale.LongFormat))
    readonly property string currentDate: Qt.formatDate(clockSource.dateTime, Qt.locale().dateFormat(Locale.LongFormat).replace(/(^dddd.?\s)|(,?\sdddd$)/, ""))

    property int hours: clockSource.dateTime.getHours()
    property int minutes: clockSource.dateTime.getMinutes()
    property int seconds: clockSource.dateTime.getSeconds()
    property bool showSecondsHand: Plasmoid.configuration.showSecondHand
    property bool showTimezone: Plasmoid.configuration.showTimezoneString

    Plasmoid.backgroundHints: "NoBackground";
    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation

    Plasmoid.toolTipMainText: Qt.formatDate(clockSource.dateTime,"dddd")
    Plasmoid.toolTipSubText: `${currentTime}\n${currentDate}`

    Clock {
        id: clockSource
        trackSeconds: showSecondsHand || Plasmoid.compactRepresentationItem.containsMouse
    }

    Plasmoid.compactRepresentation: MouseArea {
        id: representation

        Layout.minimumWidth: Plasmoid.formFactor !== PlasmaCore.Types.Vertical ? representation.height : PlasmaCore.Units.gridUnit
        Layout.minimumHeight: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? representation.width : PlasmaCore.Units.gridUnit

        property bool wasExpanded

        activeFocusOnTab: true
        hoverEnabled: true

        Accessible.name: Plasmoid.title
        Accessible.description: i18nc("@info:tooltip", "Current time is %1; Current date is %2", analogclock.currentTime, analogclock.currentDate)
        Accessible.role: Accessible.Button

        onPressed: wasExpanded = Plasmoid.expanded
        onClicked: Plasmoid.expanded = !wasExpanded

        PlasmaCore.Svg {
            id: clockSvg

            property double naturalHorizontalHandShadowOffset: estimateHorizontalHandShadowOffset()
            property double naturalVerticalHandShadowOffset: estimateVerticalHandShadowOffset()

            imagePath: "widgets/clock"
            function estimateHorizontalHandShadowOffset() {
                var id = "hint-hands-shadow-offset-to-west";
                if (hasElement(id)) {
                    return -elementSize(id).width;
                }
                id = "hint-hands-shadows-offset-to-east";
                if (hasElement(id)) {
                    return elementSize(id).width;
                }
                return 0;
            }
            function estimateVerticalHandShadowOffset() {
                var id = "hint-hands-shadow-offset-to-north";
                if (hasElement(id)) {
                    return -elementSize(id).height;
                }
                id = "hint-hands-shadow-offset-to-south";
                if (hasElement(id)) {
                    return elementSize(id).height;
                }
                return 0;
            }

            onRepaintNeeded: {
                naturalHorizontalHandShadowOffset = estimateHorizontalHandShadowOffset();
                naturalVerticalHandShadowOffset = estimateVerticalHandShadowOffset();
            }
        }

        Item {
            id: clock

            anchors {
                top: parent.top
                bottom: showTimezone ? timezoneBg.top : parent.bottom
            }
            width: parent.width

            readonly property double svgScale: face.width / face.naturalSize.width
            readonly property double horizontalShadowOffset:
                Math.round(clockSvg.naturalHorizontalHandShadowOffset * svgScale) + Math.round(clockSvg.naturalHorizontalHandShadowOffset * svgScale) % 2
            readonly property double verticalShadowOffset:
                Math.round(clockSvg.naturalVerticalHandShadowOffset * svgScale) + Math.round(clockSvg.naturalVerticalHandShadowOffset * svgScale) % 2

            PlasmaCore.SvgItem {
                id: face
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: Math.min(parent.width, parent.height)
                svg: clockSvg
                elementId: "ClockFace"
            }

            Hand {
                elementId: "HourHandShadow"
                rotationCenterHintId: "hint-hourhandshadow-rotation-center-offset"
                horizontalRotationOffset: clock.horizontalShadowOffset
                verticalRotationOffset: clock.verticalShadowOffset
                rotation: 180 + hours * 30 + (minutes/2)
                svgScale: clock.svgScale

            }
            Hand {
                elementId: "HourHand"
                rotationCenterHintId: "hint-hourhand-rotation-center-offset"
                rotation: 180 + hours * 30 + (minutes/2)
                svgScale: clock.svgScale
            }

            Hand {
                elementId: "MinuteHandShadow"
                rotationCenterHintId: "hint-minutehandshadow-rotation-center-offset"
                horizontalRotationOffset: clock.horizontalShadowOffset
                verticalRotationOffset: clock.verticalShadowOffset
                rotation: 180 + minutes * 6
                svgScale: clock.svgScale
            }
            Hand {
                elementId: "MinuteHand"
                rotationCenterHintId: "hint-minutehand-rotation-center-offset"
                rotation: 180 + minutes * 6
                svgScale: clock.svgScale
            }

            Hand {
                visible: showSecondsHand
                elementId: "SecondHandShadow"
                rotationCenterHintId: "hint-secondhandshadow-rotation-center-offset"
                horizontalRotationOffset: clock.horizontalShadowOffset
                verticalRotationOffset: clock.verticalShadowOffset
                rotation: 180 + seconds * 6
                svgScale: clock.svgScale
            }
            Hand {
                visible: showSecondsHand
                elementId: "SecondHand"
                rotationCenterHintId: "hint-secondhand-rotation-center-offset"
                rotation: 180 + seconds * 6
                svgScale: clock.svgScale
            }

            PlasmaCore.SvgItem {
                id: center
                anchors.centerIn: clock
                width: naturalSize.width * clock.svgScale
                height: naturalSize.height * clock.svgScale
                svg: clockSvg
                elementId: "HandCenterScrew"
                z: 1000
            }

            PlasmaCore.SvgItem {
                anchors.fill: face
                width: naturalSize.width * clock.svgScale
                height: naturalSize.height * clock.svgScale
                svg: clockSvg
                elementId: "Glass"
            }
        }

        PlasmaCore.FrameSvgItem {
            id: timezoneBg

            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: 10
            }
            width: childrenRect.width + margins.right + margins.left
            height: childrenRect.height + margins.top + margins.bottom
            visible: showTimezone

            imagePath: "widgets/background"

            PlasmaComponents.Label {
                id: timezoneText
                x: timezoneBg.margins.left
                y: timezoneBg.margins.top
                text: clockSource.timeZoneName
            }
        }
    }

    Plasmoid.fullRepresentation: PlasmaCalendar.MonthView {
        Layout.minimumWidth: PlasmaCore.Units.gridUnit * 22
        Layout.maximumWidth: PlasmaCore.Units.gridUnit * 80
        Layout.minimumHeight: PlasmaCore.Units.gridUnit * 22
        Layout.maximumHeight: PlasmaCore.Units.gridUnit * 40

        readonly property var appletInterface: Plasmoid.self

        today: clockSource.dateTime
    }
}
