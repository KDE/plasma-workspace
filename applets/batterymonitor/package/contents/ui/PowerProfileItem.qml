/*
 *   SPDX-FileCopyrightText: 2021 Kai Uwe Broulik <kde@broulik.de>
 *   SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

RowLayout {
    id: profileItem

    property string activeProfile
    property var profiles: []
    property string inhibitionReason
    property string degradationReason
    property var profileHolds: []

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

            readonly property var profileData:  [{
                label: i18n("Power Save"),
                profile: "power-saver",
                inhibited: false
            }, {
                label: i18n("Balanced"),
                profile: "balanced",
                inhibited: false
            }, {
                label: i18n("Performance"),
                profile: "performance",
                inhibited: profileItem.inhibitionReason != ""
            }]

            from: 0
            to: 2
            stepSize: 1
            value: profileData.findIndex((profile => profile.profile == profileItem.activeProfile))
            snapMode: PlasmaComponents3.Slider.SnapAlways
            onMoved: {
                if (!profileData[value].inhibited) {
                    activateProfileRequested(profileData[value].profile)
                } else {
                    value = Qt.binding(() => profileData.findIndex((profile => profile.profile == profileItem.activeProfile)))
                }
            }
            // fake having a disabled second half
            Rectangle {
                z: -1
                visible: profileItem.inhibitionReason != ""
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
                model: profileSlider.profileData
                PlasmaComponents3.Label {
                    horizontalAlignment: index == 0 ? Text.AlignLeft : (index == profileSlider.profileData.length - 1 ? Text.AlignRight : Text.AlignHCenter)
                    Layout.fillWidth: true
                    Layout.preferredWidth: 50   // Common width for better alignment
                    // Disable label for inhibited items to reinforce unavailability
                    enabled: !profileSlider.profileData[index].inhibited

                    text: modelData.label
                }
            }
        }

        // NOTE Only one of these will be visible at a time since the daemon will only set one depending
        // on its version
        InhibitionHint {
            visible: inhibitionReason
            width: parent.width
            iconSource: "dialog-information"
            text: {
                switch(inhibitionReason) {
                case "lap-detected":
                    return i18n("Performance mode has been disabled to reduce heat generation because the computer has detected that it may be sitting on your lap.")
                case "high-operating-temperature":
                    return i18n("Performance mode is unavailable because the computer is running too hot.")
                default:
                    return i18n("Performance mode is unavailable.")
                }
            }
        }
        InhibitionHint {
            visible: activeProfile == "performance" && degradationReason
            width: parent.width
            iconSource: "dialog-information"
            text: switch(degradationReason) {
                case "lap-detected":
                    return i18n("Performance may be lowered to reduce heat generation because the computer has detected that it may be sitting on your lap.")
                case "high-operating-temperature":
                     return i18n("Performance may be reduced because the computer is running too hot.")
                default:
                  return i18n("Performance may be reduced.")
            }
        }

        InhibitionHint {
            visible: holdRepeater.count > 0
            text: i18np("One application has requested activating %2:",
                        "%1 applications have requested activating %2:", holdRepeater.count,
                        profileSlider.profileData.find((profile) => profile.profile == profileItem.activeProfile).label)
        }
        Repeater {
            id: holdRepeater
            model: profileItem.profileHolds.filter((hold) => hold.Profile == profileItem.activeProfile)
            InhibitionHint {
                x: PlasmaCore.Units.smallSpacing
                iconSource: modelData.Icon
                text: i18nc("%1 is the name of the application, %2 is the reason provided by it for activating performance mode","%1: %2", modelData.Name, modelData.Reason)
            }
        }

    }
}
