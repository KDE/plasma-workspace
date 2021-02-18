/*
 *   Copyright 2021 Kai Uwe Broulik <kde@broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import QtQuick.Controls 2.0 as QQC2

RowLayout {
    id: profileItem

    property string activeProfile
    property var profiles: []
    readonly property int profilesCount: profileRepeater.count

    signal activateProfileRequested(string profile)

    spacing: PlasmaCore.Units.gridUnit

    PlasmaCore.IconItem {
        id: brightnessIcon
        source: "battery-ups" // FIXME proper icon
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: PlasmaCore.Units.iconSizes.medium
        Layout.preferredHeight: width
    }

    Column {
        id: brightnessColumn
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        spacing: 0

        PlasmaComponents3.Label {
            id: brightnessLabel
            Layout.fillWidth: true
            text: i18n("Power Profile")
        }

        RowLayout {
            width: parent.width
            // HACK to make a segmented control
            spacing: -PlasmaCore.Units.smallSpacing

            QQC2.ButtonGroup {
                id: group
            }

            Repeater {
                id: profileRepeater
                model: [{
                    label: i18n("Power Save"),
                    icon: "document-save-all", // FIXME
                    profile: "power-saver"
                }, {
                    label: i18n("Balanced"),
                    icon: "auto-scale-all", // scale, balance, get it? FIXME
                    profile: "balanced"
                }, {
                    label: i18n("Performance"),
                    icon: "battery-ups", // FIXME
                    profile: "performance"
                // Filter out non-supported profiles
                }].filter((item) => profileItem.profiles.includes(item.profile))

                PlasmaComponents3.Button {
                    // spread uniformly
                    Layout.preferredWidth: 1
                    Layout.fillWidth: true
                    text: modelData.label
                    icon.name: modelData.icon
                    // TODO performance inhibited
                    // enabled:
                    // Only let it reflect the currnt state, not what the user might have licked
                    checkable: false
                    checked: profileItem.activeProfile === modelData.profile
                    // HACK to make a segmented control :)
                    z: checked ? 1 : 0
                    QQC2.ButtonGroup.group: group

                    onClicked: profileItem.activateProfileRequested(modelData.profile)
                }
            }
        }

        // TODO performance inhibited
        /*InhibitionHint {
            width: parent.width
            text: i18n("Performance profile currently isn't available because reasons.")
        }*/
    }
}
