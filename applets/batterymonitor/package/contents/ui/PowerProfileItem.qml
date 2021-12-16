/*
 *   SPDX-FileCopyrightText: 2021 Kai Uwe Broulik <kde@broulik.de>
 *   SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore

RowLayout {
    id: root

    property string activeProfile

    property string inhibitionReason
    readonly property bool inhibited: inhibitionReason !== ""

    property string degradationReason

    // type: [{ Name: string, Icon: string, Profile: string, Reason: string }]
    required property var profileHolds

    // The canBeInhibited property mean that this profile's availability
    // depends on root.inhibited value (and thus on the
    // inhibitionReason string).
    readonly property var profileData: [
        {
            label: i18n("Power Save"),
            profile: "power-saver",
            canBeInhibited: false,
        }, {
            label: i18n("Balanced"),
            profile: "balanced",
            canBeInhibited: false,
        }, {
            label: i18n("Performance"),
            profile: "performance",
            canBeInhibited: true,
        }
    ]

    readonly property int activeProfileIndex: profileData.findIndex(data => data.profile === activeProfile)
    // type: typeof(profileData[])?
    readonly property var activeProfileData: activeProfileIndex !== -1 ? profileData[activeProfileIndex] : undefined
    // type: typeof(profileHolds)
    readonly property var activeHolds: profileHolds.filter(hold => hold.Profile === activeProfile)

    signal activateProfileRequested(string profile)

    spacing: PlasmaCore.Units.gridUnit

    PlasmaCore.IconItem {
        source: "speedometer"
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: PlasmaCore.Units.iconSizes.medium
        Layout.preferredHeight: width
    }
    Column {
        id: profileColumn
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        spacing: 0

        PlasmaComponents3.Label {
            text: i18n("Power Profile")
        }

        PlasmaComponents3.Slider {
            id: profileSlider
            width: parent.width

            from: 0
            to: 2
            stepSize: 1
            value: root.activeProfileIndex
            snapMode: PlasmaComponents3.Slider.SnapAlways
            onMoved: {
                const { canBeInhibited, profile } = root.profileData[value];
                if (!(canBeInhibited && root.inhibited)) {
                    activateProfileRequested(profile);
                } else {
                    value = Qt.binding(() => root.activeProfileIndex);
                }
            }
            // fake having a disabled second half
            Rectangle {
                z: -1
                visible: root.inhibited
                color: PlasmaCore.Theme.backgroundColor
                anchors.left: parent.horizontalCenter
                anchors.leftMargin: 1
                anchors.right: parent.right
                anchors.top: parent.background.top
                height: parent.background.height
                opacity: 0.4
            }
        }
        RowLayout {
            width: parent.width
            spacing: 0
            Repeater {
                model: root.profileData
                PlasmaComponents3.Label {
                    // At the time of writing, QtQuick/Positioner QML Type does not support Layouts
                    readonly property bool isFirstItem: index === 0
                    readonly property bool isLastItem: index === root.profileData.length - 1

                    horizontalAlignment: isFirstItem ? Text.AlignLeft : (isLastItem ? Text.AlignRight : Text.AlignHCenter)
                    Layout.fillWidth: true
                    Layout.preferredWidth: 50 // Common width for better alignment
                    // Disable label for inhibited items to reinforce unavailability
                    enabled: !(root.profileData[index].canBeInhibited && root.inhibited)

                    text: modelData.label
                }
            }
        }

        // NOTE Only one of these will be visible at a time since the daemon will only set one depending
        // on its version
        InhibitionHint {
            visible: root.inhibited
            width: parent.width
            iconSource: "dialog-information"
            text: switch(root.inhibitionReason) {
                case "lap-detected":
                    return i18n("Performance mode has been disabled to reduce heat generation because the computer has detected that it may be sitting on your lap.")
                case "high-operating-temperature":
                    return i18n("Performance mode is unavailable because the computer is running too hot.")
                default:
                    return i18n("Performance mode is unavailable.")
            }
        }
        InhibitionHint {
            visible: root.activeProfile === "performance" && root.degradationReason !== ""
            width: parent.width
            iconSource: "dialog-information"
            text: switch(root.degradationReason) {
                case "lap-detected":
                    return i18n("Performance may be lowered to reduce heat generation because the computer has detected that it may be sitting on your lap.")
                case "high-operating-temperature":
                    return i18n("Performance may be reduced because the computer is running too hot.")
                default:
                    return i18n("Performance may be reduced.")
            }
        }

        InhibitionHint {
            visible: root.activeHolds.length > 0 && root.activeProfileData !== undefined
            text: root.activeProfileData !== undefined
                ? i18np("One application has requested activating %2:",
                        "%1 applications have requested activating %2:",
                        root.activeHolds.length,
                        i18n(root.activeProfileData.label))
                : ""
        }
        Repeater {
            model: root.activeHolds
            InhibitionHint {
                x: PlasmaCore.Units.smallSpacing
                iconSource: modelData.Icon
                text: i18nc("%1 is the name of the application, %2 is the reason provided by it for activating performance mode",
                            "%1: %2", modelData.Name, modelData.Reason)
            }
        }
    }
}
