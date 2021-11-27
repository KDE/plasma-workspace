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
    id: profileItem

    property string activeProfile

    property string inhibitionReason
    readonly property bool inhibited: inhibitionReason !== ""

    property string degradationReason

    // type: [{ Name: string, Icon: string, Profile: string, Reason: string }]
    required property var profileHolds

    // The canBeInhibited property mean that this profile's availability
    // depends on profileItem.inhibited value (and thus on the
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
            value: profileItem.activeProfileIndex
            snapMode: PlasmaComponents3.Slider.SnapAlways
            onMoved: {
                const { canBeInhibited, profile } = profileItem.profileData[value];
                if (!(canBeInhibited && profileItem.inhibited)) {
                    activateProfileRequested(profile);
                } else {
                    value = Qt.binding(() => profileItem.activeProfileIndex);
                }
            }
            // fake having a disabled second half
            Rectangle {
                z: -1
                visible: profileItem.inhibited
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
                model: profileItem.profileData
                PlasmaComponents3.Label {
                    // At the time of writing, QtQuick/Positioner QML Type does not support Layouts
                    readonly property bool isFirstItem: index === 0
                    readonly property bool isLastItem: index === profileItem.profileData.length - 1

                    horizontalAlignment: isFirstItem ? Text.AlignLeft : (isLastItem ? Text.AlignRight : Text.AlignHCenter)
                    Layout.fillWidth: true
                    Layout.preferredWidth: 50 // Common width for better alignment
                    // Disable label for inhibited items to reinforce unavailability
                    enabled: !(profileItem.profileData[index].canBeInhibited && profileItem.inhibited)

                    text: modelData.label
                }
            }
        }

        // NOTE Only one of these will be visible at a time since the daemon will only set one depending
        // on its version
        InhibitionHint {
            visible: profileItem.inhibited
            width: parent.width
            iconSource: "dialog-information"
            text: switch(profileItem.inhibitionReason) {
                case "lap-detected":
                    return i18n("Performance mode has been disabled to reduce heat generation because the computer has detected that it may be sitting on your lap.")
                case "high-operating-temperature":
                    return i18n("Performance mode is unavailable because the computer is running too hot.")
                default:
                    return i18n("Performance mode is unavailable.")
            }
        }
        InhibitionHint {
            visible: profileItem.activeProfile === "performance" && profileItem.degradationReason !== ""
            width: parent.width
            iconSource: "dialog-information"
            text: switch(profileItem.degradationReason) {
                case "lap-detected":
                    return i18n("Performance may be lowered to reduce heat generation because the computer has detected that it may be sitting on your lap.")
                case "high-operating-temperature":
                    return i18n("Performance may be reduced because the computer is running too hot.")
                default:
                    return i18n("Performance may be reduced.")
            }
        }

        InhibitionHint {
            visible: profileItem.activeHolds.length > 0 && profileItem.activeProfileData !== undefined
            text: profileItem.activeProfileData !== undefined
                ? i18np("One application has requested activating %2:",
                        "%1 applications have requested activating %2:",
                        profileItem.activeHolds.length,
                        i18n(profileItem.activeProfileData.label))
                : ""
        }
        Repeater {
            model: profileItem.activeHolds
            InhibitionHint {
                x: PlasmaCore.Units.smallSpacing
                iconSource: modelData.Icon
                text: i18nc("%1 is the name of the application, %2 is the reason provided by it for activating performance mode",
                            "%1: %2", modelData.Name, modelData.Reason)
            }
        }
    }
}
