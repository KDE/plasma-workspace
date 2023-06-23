/*
    SPDX-FileCopyrightText: 2016 David Rosca <nowrep@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/
import QtQuick 2.4
import QtQuick.Layouts 1.1
import org.kde.ksvg 1.0 as KSvg
import org.kde.kirigami 2.20 as Kirigami
import org.kde.private.kcms.desktoptheme 1.0 as Private

Item {
    id: root
    property string themeName
    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    Item {
        id: backgroundMask
        anchors.fill: parent
        clip: true

        KSvg.FrameSvgItem {
            id: background
            // Normalize margins around background.
            // Some themes like "Air" have huge transparent margins which would result in too small container area.
            // Sadly all of the breathing, shadow and border sizes are in one single margin value,
            // but for typical themes the border is the smaller part the margin and should be in the size of
            // Units.largeSpacing, to which we add another Units.largeSpacing for margin of the visual content
            // Ideally Plasma::FrameSvg exposes the transparent margins one day.
            readonly property int generalMargin: 2 * Kirigami.Units.largeSpacing
            anchors {
                fill: parent
                topMargin: -margins.top + generalMargin
                bottomMargin: -margins.bottom + generalMargin
                leftMargin: -margins.left + generalMargin
                rightMargin: -margins.right + generalMargin
            }
            imagePath: "widgets/background"
        }
    }

    RowLayout {
        id: contents
        spacing: 0
        anchors {
            fill: parent
            topMargin: background.generalMargin
            bottomMargin: background.generalMargin
            leftMargin: background.generalMargin
            rightMargin: background.generalMargin
        }

        // Analog clock
        Item {
            id: clock
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: height
            Layout.alignment: Qt.AlignHCenter
            property int hours: 9
            property int minutes: 5

            readonly property double svgScale: face.width / face.naturalSize.width
            readonly property double horizontalShadowOffset:
                Math.round(clockSvg.naturalHorizontalHandShadowOffset * svgScale) + Math.round(clockSvg.naturalHorizontalHandShadowOffset * svgScale) % 2
            readonly property double verticalShadowOffset:
                Math.round(clockSvg.naturalVerticalHandShadowOffset * svgScale) + Math.round(clockSvg.naturalVerticalHandShadowOffset * svgScale) % 2

            KSvg.Svg {
                id: clockSvg
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
                property double naturalHorizontalHandShadowOffset: estimateHorizontalHandShadowOffset()
                property double naturalVerticalHandShadowOffset: estimateVerticalHandShadowOffset()
                onRepaintNeeded: {
                    naturalHorizontalHandShadowOffset = estimateHorizontalHandShadowOffset();
                    naturalVerticalHandShadowOffset = estimateVerticalHandShadowOffset();
                }
            }

            KSvg.SvgItem {
                id: face
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: Math.min(parent.width, parent.height)
                svg: clockSvg
                elementId: "ClockFace"
            }

            Hand {
                elementId: "HourHand"
                rotationCenterHintId: "hint-hourhand-rotation-center-offset"
                rotation: 180 + clock.hours * 30 + (clock.minutes/2)
                svgScale: clock.svgScale
            }

            Hand {
                elementId: "MinuteHand"
                rotationCenterHintId: "hint-minutehand-rotation-center-offset"
                rotation: 180 + clock.minutes * 6
                svgScale: clock.svgScale
            }

            KSvg.SvgItem {
                id: center
                width: naturalSize.width * clock.svgScale
                height: naturalSize.height * clock.svgScale
                anchors.centerIn: clock
                svg: clockSvg
                elementId: "HandCenterScrew"
                z: 1000
            }

            KSvg.SvgItem {
                anchors.fill: face
                svg: clockSvg
                elementId: "Glass"
                width: naturalSize.width * clock.svgScale
                height: naturalSize.height * clock.svgScale
            }
        }
        Kirigami.Icon {
            visible: model.colorType === Private.ThemesModel.FollowsColorTheme
            source: "color-profile"
            width: Kirigami.Units.iconSizes.smallMedium
            height: width
            Layout.alignment: Qt.AlignRight && Qt.AlignTop
        }
    }

    Component.onCompleted: {
        kcm.applyPlasmaTheme(root, themeName);
    }
}
