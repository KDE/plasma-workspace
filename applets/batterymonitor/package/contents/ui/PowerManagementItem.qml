/*
    SPDX-FileCopyrightText: 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
    SPDX-FileCopyrightText: 2013, 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.kquickcontrolsaddons 2.1
import org.kde.kwindowsystem 1.0
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore

ColumnLayout {
    id: root

    property alias pmCheckBox: pmCheckBox
    property alias disabled: pmCheckBox.checked
    property bool pluggedIn

    // List of active power management inhibitions (applications that are
    // blocking sleep and screen locking).
    //
    // type: [{
    //  Icon: string,
    //  Name: string,
    //  Reason: string,
    // }]
    property var inhibitions: []
    property bool inhibitsLidAction

    KWindowSystem {
        id: kwindowsystem
    }

    // UI to manually inhibit sleep and screen locking
    PlasmaComponents3.CheckBox {
        id: pmCheckBox
        Layout.fillWidth: true
        text: i18nc("Minimize the length of this string as much as possible", "Manually block sleep and screen locking")
        checked: false
        focus: true

        KeyNavigation.up: dialog.KeyNavigation.up
        KeyNavigation.down: batteryList.children[0]
        KeyNavigation.backtab: dialog.KeyNavigation.backtab
        KeyNavigation.tab: KeyNavigation.down
    }

    // Separator line
    PlasmaCore.SvgItem {
        Layout.fillWidth: true

        visible: inhibitionReasonsLayout.visible

        elementId: "horizontal-line"
        svg: PlasmaCore.Svg {
            imagePath: "widgets/line"
        }
    }

    // list of automatic inhibitions
    ColumnLayout {
        id: inhibitionReasonsLayout

        Layout.fillWidth: true
        visible: root.inhibitsLidAction || (root.inhibitions.length > 0 && !root.disabled)

        InhibitionHint {
            Layout.fillWidth: true
            visible: root.inhibitsLidAction
            iconSource: "computer-laptop"
            text: i18nc("Minimize the length of this string as much as possible", "Your laptop is configured not to sleep when closing the lid while an external monitor is connected.")
        }

        PlasmaComponents3.Label {
            id: inhibitionExplanation
            Layout.fillWidth: true
            // Don't need to show the inhibitions when power management
            // isn't enabled anyway
            visible: root.inhibitions.length > 1 && !root.disabled
            font: PlasmaCore.Theme.smallestFont
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 3
            text: i18np("%1 application is currently blocking sleep and screen locking:",
                        "%1 applications are currently blocking sleep and screen locking:",
                        root.inhibitions.length)
        }

        Repeater {
            // Don't need to show the inhibitions when power management
            // is manually disabled anyway
            visible: !root.disabled
            model: !root.disabled ? root.inhibitions : null

            InhibitionHint {
                property string icon: modelData.Icon
                    || (kwindowsystem.isPlatformWayland ? "wayland" : "xorg")
                property string name: modelData.Name
                property string reason: modelData.Reason

                Layout.fillWidth: true
                iconSource: icon
                text: (root.inhibitions.length === 1)
                    ? (reason
                        ? i18n("%1 is currently blocking sleep and screen locking (%2)", name, reason)
                        : i18n("%1 is currently blocking sleep and screen locking (unknown reason)", name))
                    : (reason
                        ? i18nc("Application name: reason for preventing sleep and screen locking", "%1: %2", name, reason)
                        : i18nc("Application name: reason for preventing sleep and screen locking", "%1: unknown reason", name))
            }
        }
    }
}
