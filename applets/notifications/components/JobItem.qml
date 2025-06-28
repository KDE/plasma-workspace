/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQml

import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

import org.kde.notificationmanager as NotificationManager

import plasma.applet.org.kde.plasma.notifications as Notifications

import "../global"

ColumnLayout {
    id: jobItem

    property ModelInterface modelInterface

    readonly property int totalFiles: modelInterface.jobDetails && modelInterface.jobDetails.totalFiles || 0

    readonly property alias menuOpen: otherFileActionsMenu.visible

    spacing: Kirigami.Units.smallSpacing

    Notifications.FileInfo {
        id: fileInfo
        url: jobItem.totalFiles === 1 ? modelInterface.jobDetails.effectiveDestUrl : ""
    }

    SpeedChart {
        id: speedChart
        width: jobItem.width
        modelInterface: jobItem.modelInterface
        expanded: expandButton.checked
    }

    RowLayout {
        id: jobActionsRow
        Layout.fillWidth: true

        spacing: Kirigami.Units.smallSpacing

        PlasmaComponents3.Button {
            id: expandButton

            Layout.fillWidth: true
            Layout.maximumWidth: implicitWidth

            icon.name: checked ? "collapse-symbolic" : "expand-symbolic"
            text: i18ndc("plasma_applet_org.kde.plasma.notifications", "Hides/expands item details", "Details")
            checkable: modelInterface.jobDetails && modelInterface.jobDetails.hasDetails
            visible: checkable

            Accessible.onPressAction: if (checkable) toggle()
        }

        Item { Layout.fillWidth: true }

        PlasmaComponents3.Button {
            id: suspendButton

            readonly property bool paused: modelInterface.jobState === NotificationManager.Notifications.JobStateSuspended

            visible: modelInterface.suspendable

            Layout.fillWidth: true
            Layout.maximumWidth: implicitWidth

            icon.name: paused ? "media-playback-start-symbolic"
                              : "media-playback-pause-symbolic"
            text: paused ? i18ndc("plasma_applet_org.kde.plasma.notifications", "Resume paused job", "Resume")
                         : i18ndc("plasma_applet_org.kde.plasma.notifications", "Pause running job", "Pause")
            onClicked: paused ? modelInterface.resumeJobClicked()
                              : modelInterface.suspendJobClicked()
        }

        PlasmaComponents3.Button {
            id: killButton

            Layout.fillWidth: true
            Layout.maximumWidth: implicitWidth

            visible: modelInterface.killable

            icon.name: "dialog-cancel-symbolic"
            text: i18ndc("plasma_applet_org.kde.plasma.notifications", "Cancel running job", "Cancel")
            onClicked: modelInterface.killJobClicked()
        }
    }

    Loader {
        Layout.fillWidth: true
        Layout.preferredWidth: Globals.popupWidth
        Layout.preferredHeight: item ? item.implicitHeight : 0
        active: expandButton.checked
        // Loader doesn't reset its height when unloaded, just hide it altogether
        visible: active
        sourceComponent: JobDetails {
            modelInterface: jobItem.modelInterface
        }
    }

    Row {
        id: fileActionsRow
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        // We want the actions to be right-aligned but Row also reverses
        // the order of items, so we put them in reverse order
        layoutDirection: Qt.RightToLeft
        visible: modelInterface.jobDetails.effectiveDestUrl.toString() !== "" && !fileInfo.error

        PlasmaComponents3.Button {
            id: otherFileActionsButton
            height: Math.max(implicitHeight, openButton.implicitHeight)
            icon.name: "application-menu-symbolic"
            checkable: true
            text: openButton.visible ? "" : Accessible.name
            Accessible.name: i18nd("plasma_applet_org.kde.plasma.notifications", "More Options…")
            onPressedChanged: {
                if (pressed) {
                    checked = Qt.binding(function() {
                        return otherFileActionsMenu.visible;
                    });
                    otherFileActionsMenu.visualParent = this;
                    // -1 tells it to "align bottom left of visualParent (this)"
                    otherFileActionsMenu.open(-1, -1);
                }
            }

            PlasmaComponents3.ToolTip {
                text: parent.Accessible.name
                enabled: parent.text === ""
            }

            Notifications.FileMenu {
                id: otherFileActionsMenu
                url: modelInterface.jobDetails.effectiveDestUrl
                onActionTriggered: action => modelInterface.fileActionInvoked(action)
            }
        }

        PlasmaComponents3.Button {
            id: openButton
            width: Math.min(implicitWidth, jobItem.width - otherFileActionsButton.width - fileActionsRow.spacing)
            height: Math.max(implicitHeight, otherFileActionsButton.implicitHeight)
            text: i18nd("plasma_applet_org.kde.plasma.notifications", "Open")
            onClicked: modelInterface.openUrl(modelInterface.jobDetails.effectiveDestUrl)

            states: [
                State {
                    when: modelInterface.jobDetails && modelInterface.jobDetails.totalFiles !== 1
                    PropertyChanges {
                        target: openButton
                        text: i18nd("plasma_applet_org.kde.plasma.notifications", "Open Containing Folder")
                        icon.name: "folder-open-symbolic"
                    }
                },
                State {
                    when: fileInfo.openAction !== null
                    PropertyChanges {
                        target: openButton
                        text: fileInfo.openAction.text
                        icon.name: fileInfo.openActionIconName
                        visible: fileInfo.openAction.enabled
                        onClicked: {
                            fileInfo.openAction.trigger();
                            modelInterface.fileActionInvoked(fileInfo.openAction);
                        }
                    }
                }
            ]
        }
    }

    states: [
        State {
            when: modelInterface.jobState === NotificationManager.Notifications.JobStateSuspended
            PropertyChanges {
                target: speedChart
                enabled: false
            }
        },
        State {
            when: modelInterface.jobState === NotificationManager.Notifications.JobStateStopped
            PropertyChanges {
                target: speedChart
                visible: false
            }
            PropertyChanges {
                target: jobActionsRow
                visible: false
            }
            PropertyChanges {
                target: expandButton
                checked: false
            }
        }
    ]
}
