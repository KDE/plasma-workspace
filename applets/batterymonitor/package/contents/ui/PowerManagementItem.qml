/*
    SPDX-FileCopyrightText: 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
    SPDX-FileCopyrightText: 2013, 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.kquickcontrolsaddons 2.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.1 as PlasmaCore

ColumnLayout {
    id: root

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

    // UI to manually inhibit sleep and screen locking
    PlasmaComponents3.CheckBox {
        id: pmCheckBox
        Layout.fillWidth: true
        text: i18nc("Minimize the length of this string as much as possible", "Manually block sleep and screen locking")
        checked: false
    }

    // Separator line
    PlasmaCore.SvgItem {
        Layout.fillWidth: true

        visible: inhibitionReasonsLayout.visibleChildren.length > 0

        elementId: "horizontal-line"
        svg: PlasmaCore.Svg {
            imagePath: "widgets/line"
        }
    }

    // list of automatic inhibitions
    ColumnLayout {
        id: inhibitionReasonsLayout

        Layout.fillWidth: true

        InhibitionHint {
            Layout.fillWidth: true
            visible: root.inhibitsLidAction
            iconSource: "computer-laptop"
            text: i18nc("Minimize the length of this string as much as possible", "Your notebook is configured not to sleep when closing the lid while an external monitor is connected.")
        }

        // UI to display when there is more than one inhibition
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
            visible: root.inhibitions.length > 1 && !root.disabled

            model: visible ? root.inhibitions.length : null

            InhibitionHint {
                Layout.fillWidth: true
                iconSource: root.inhibitions[index].Icon || ""
                text: root.inhibitions[index].Reason
                    ? i18nc("Application name: reason for preventing sleep and screen locking", "%1: %2", root.inhibitions[index].Name, root.inhibitions[index].Reason)
                    : i18nc("Application name: reason for preventing sleep and screen locking", "%1: unknown reason", root.inhibitions[index].Name)
            }
        }

        // UI to display when there is only one inhibition
        InhibitionHint {
            Layout.fillWidth: true
            // Don't need to show the inhibitions when power management
            // is manually disabled anyway
            visible: root.inhibitions.length === 1 && !root.disabled
            iconSource: (root.inhibitions.length === 1) ? root.inhibitions[0].Icon : ""
            text: (root.inhibitions.length === 1) ?
                    root.inhibitions[0].Reason ?
                        i18n("%1 is currently blocking sleep and screen locking (%2)", root.inhibitions[0].Name, root.inhibitions[0].Reason)
                    :
                        i18n("%1 is currently blocking sleep and screen locking (unknown reason)", root.inhibitions[0].Name)
                :
                    ""
        }
    }
}
