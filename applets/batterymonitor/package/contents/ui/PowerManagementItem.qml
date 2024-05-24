/*
    SPDX-FileCopyrightText: 2012-2013 Daniel Nicoletti <dantti12@gmail.com>
    SPDX-FileCopyrightText: 2013, 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import org.kde.notification
import org.kde.kwindowsystem
import org.kde.plasma.components as PlasmaComponents3
import org.kde.ksvg as KSvg
import org.kde.kirigami as Kirigami

PlasmaComponents3.ItemDelegate {
    id: root

    property bool pluggedIn

    signal inhibitionChangeRequested(bool inhibit)

    property bool isManuallyInhibited
    property bool isManuallyInhibitedError
    // List of active power management inhibitions (applications that are
    // blocking sleep and screen locking).
    //
    // type: [{
    //  Name: string,
    //  PrettyName: string,
    //  Icon: string,
    //  Reason: string,
    // }]
    property var inhibitions: []
    property var blockedInhibitions: []
    property bool inhibitsLidAction

    property alias manualInhibitionSwitch: manualInhibitionSwitch

    background.visible: highlighted
    highlighted: activeFocus
    hoverEnabled: false
    text: i18nc("@title:group", "Sleep and Screen Locking after Inactivity")

    Notification {
        id: inhibitionError
        componentName: "plasma_workspace"
        eventId: "warning"
        iconName: "system-suspend-uninhibited"
        title: i18n("Power Management")
    }

    Accessible.description: pmStatusLabel.text
    Accessible.role: Accessible.CheckBox
    KeyNavigation.tab: manualInhibitionSwitch

    contentItem: RowLayout {
        spacing: Kirigami.Units.gridUnit

        Kirigami.Icon {
            id: icon
            source: root.isManuallyInhibited || root.inhibitions.length > 0 ? "system-suspend-inhibited" : "system-suspend-uninhibited"
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: Kirigami.Units.iconSizes.medium
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                PlasmaComponents3.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: root.text
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }

                PlasmaComponents3.Label {
                    id: pmStatusLabel
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    text: root.isManuallyInhibited || root.inhibitions.length > 0 ? i18nc("Sleep and Screen Locking after Inactivity", "Blocked") : i18nc("Sleep and Screen Locking after Inactivity", "Automatic")
                    textFormat: Text.PlainText
                }
            }

            RowLayout {
                // UI to manually inhibit sleep and screen locking
                PlasmaComponents3.Switch {
                    id: manualInhibitionSwitch
                    Layout.fillWidth: true
                    text: i18nc("@option:check Manually block sleep and screen locking after inactivity", "Manually block")
                    checked: root.isManuallyInhibited
                    focus: true

                    KeyNavigation.up: root.KeyNavigation.up
                    KeyNavigation.down: root.KeyNavigation.down
                    KeyNavigation.tab: root.KeyNavigation.down
                    KeyNavigation.backtab: root.KeyNavigation.backtab

                    Keys.onPressed: (event) => {
                        if (event.key == Qt.Key_Space || event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                            clicked();
                        }
                    }

                    onToggled: {
                        inhibitionChangeRequested(checked);
                    }

                    Connections {
                        target: root
                        function onIsManuallyInhibitedChanged() {
                            manualInhibitionSwitch.checked = isManuallyInhibited;
                        }
                    }

                    Connections {
                        target: root
                        function onIsManuallyInhibitedErrorChanged() {
                            if (root.isManuallyInhibitedError) {
                                manualInhibitionSwitch.checked = !manualInhibitionSwitch.checked
                                busyIndicator.visible = false;
                                root.isManuallyInhibitedError = false;
                                if (!root.isManuallyInhibited) {
                                    inhibitionError.text = i18n("Failed to unblock automatic sleep and screen locking");
                                    inhibitionError.sendEvent();
                                } else {
                                    inhibitionError.text = i18n("Failed to block automatic sleep and screen locking");
                                    inhibitionError.sendEvent();
                                }
                            }
                        }
                    }
                }

            BusyIndicator {
                id: busyIndicator
                visible: false
                Layout.preferredHeight: manualInhibitionSwitch.implicitHeight

                Connections {
                    target: root

                    function onInhibitionChangeRequested(inhibit) {
                        busyIndicator.visible = inhibit != root.isManuallyInhibited;
                    }

                    function onIsManuallyInhibitedChanged(val) {
                        busyIndicator.visible = false;
                    }
                }
            }
        }

            // list of automatic inhibitions
            ColumnLayout {
                id: inhibitionReasonsLayout

                Layout.fillWidth: true
                visible: root.inhibitsLidAction || (root.inhibitions.length > 0) || (root.blockedInhibitions.length > 0)

                InhibitionHint {
                    readonly property var pmControl: root.pmControl

                    Layout.fillWidth: true
                    visible: root.inhibitsLidAction
                    iconSource: "computer-laptop"
                    text: i18nc("Minimize the length of this string as much as possible", "Your laptop is configured not to sleep when closing the lid while an external monitor is connected.")
                }

                PlasmaComponents3.Label {
                    id: inhibitionExplanation
                    Layout.fillWidth: true
                    visible: root.inhibitions.length > 1
                    font: Kirigami.Theme.smallFont
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 3
                    text: i18np("%1 application is currently blocking sleep and screen locking:",
                                "%1 applications are currently blocking sleep and screen locking:",
                                root.inhibitions.length)
                    textFormat: Text.PlainText
                }

                Repeater {
                    model: root.inhibitions

                    InhibitionHint {
                        property string icon: modelData.Icon
                            || (KWindowSystem.isPlatformWayland ? "wayland" : "xorg")
                        property string app: modelData.Name
                        property string name: modelData.PrettyName
                        property string reason: modelData.Reason

                        Layout.fillWidth: true
                        iconSource: icon
                        text: {
                            if (root.inhibitions.length === 1) {
                                if (reason && name) {
                                    return i18n("%1 is currently blocking sleep and screen locking (%2)", name, reason)
                                } else if (name) {
                                    return i18n("%1 is currently blocking sleep and screen locking (unknown reason)", name)
                                } else if (reason) {
                                    return i18n("An application is currently blocking sleep and screen locking (%1)", reason)
                                } else {
                                    return i18n("An application is currently blocking sleep and screen locking (unknown reason)")
                                }
                            } else {
                                if (reason && name) {
                                    return i18nc("Application name: reason for preventing sleep and screen locking", "%1: %2", name, reason)
                                } else if (name) {
                                    return i18nc("Application name: reason for preventing sleep and screen locking", "%1: unknown reason", name)
                                } else if (reason) {
                                    return i18nc("Application name: reason for preventing sleep and screen locking", "Unknown application: %1", reason)
                                } else {
                                    return i18nc("Application name: reason for preventing sleep and screen locking", "Unknown application: unknown reason")
                                }
                            }
                        }

                        Item {
                            width: blockButton.width
                            height: blockButton.height

                            PlasmaComponents3.Button {
                                id: blockButton
                                text: i18nc("@action:button Prevent an app from blocking automatic sleep and screen locking after inactivity", "Unblock…")
                                icon.name: "edit-delete-remove"
                                onClicked: blockButtonMenu.open()
                            }

                            Menu {
                                id: blockButtonMenu
                                x: blockButton.x + blockButton.width - blockButtonMenu.width
                                y: blockButton.y + blockButton.height

                                MenuItem {
                                    text: i18nc("@action:button Prevent an app from blocking automatic sleep and screen locking after inactivity", "Only this time")
                                    onTriggered: pmControl.blockInhibition(app, reason, false)
                                }

                                MenuItem {
                                    text: i18nc("@action:button Prevent an app from blocking automatic sleep and screen locking after inactivity", "Every time for this app and reason")
                                    onTriggered: pmControl.blockInhibition(app, reason, true)
                                }
                            }
                        }
                    }
                }

                // list of blocked inhibitions
                PlasmaComponents3.Label {
                    id: blockedInhibitionExplanation
                    Layout.fillWidth: true
                    visible: root.blockedInhibitions.length > 1
                    font: Kirigami.Theme.smallFont
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 3
                    text: i18np("%1 application has been prevented from blocking sleep and screen locking:",
                                "%1 applications have been prevented from blocking sleep and screen locking:",
                                root.blockedInhibitions.length)
                    textFormat: Text.PlainText
                }

                Repeater {
                    model: root.blockedInhibitions

                    InhibitionHint {
                        property string icon: modelData.Icon
                            || (KWindowSystem.isPlatformWayland ? "wayland" : "xorg")
                        property string app: modelData.Name
                        property string name: modelData.PrettyName
                        property string reason: modelData.Reason

                        Layout.fillWidth: true
                        iconSource: icon
                        text: {
                            if (root.blockedInhibitions.length === 1) {
                                return i18nc("Application name; reason", "%1 has been prevented from blocking sleep and screen locking for %2", name, reason)
                            } else {
                                return i18nc("Application name: reason for preventing sleep and screen locking", "%1: %2", name, reason)
                            }
                        }

                        Item {
                            width: unblockButton.width
                            height: unblockButton.height

                            PlasmaComponents3.Button {
                                id: unblockButton
                                text: i18nc("@action:button Undo preventing an app from blocking automatic sleep and screen locking after inactivity", "Block again…")
                                icon.name: "dialog-cancel"
                                onClicked: unblockButtonMenu.open()
                            }

                            Menu {
                                id: unblockButtonMenu
                                x: unblockButton.x + unblockButton.width - unblockButtonMenu.width
                                y: unblockButton.y + unblockButton.height

                                MenuItem {
                                    text: i18nc("@action:button Prevent an app from blocking automatic sleep and screen locking after inactivity", "Only this time")
                                    onTriggered: pmControl.unblockInhibition(app, reason, false)
                                }

                                MenuItem {
                                    text: i18nc("@action:button Prevent an app from blocking automatic sleep and screen locking after inactivity", "Every time for this app and reason")
                                    onTriggered: pmControl.unblockInhibition(app, reason, true)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
