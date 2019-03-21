/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.8
import QtQuick.Window 2.2
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

import org.kde.notificationmanager 1.0 as NotificationManager
import org.kde.plasma.private.notifications 2.0 as Notifications

ColumnLayout {
    id: jobItem

    property int jobState
    property int error
    property string errorText

    property alias percentage: progressBar.value
    property alias suspendable: suspendButton.visible
    property alias killable: killButton.visible

    property bool hovered
    property QtObject jobDetails
    // TOOD make an alias on visible if we're not doing an animation
    property bool showDetails

    signal suspendJobClicked
    signal resumeJobClicked
    signal killJobClicked

    signal openUrl(string url)

    spacing: 0

    PlasmaComponents.Label {
        Layout.fillWidth: true
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
        text: jobItem.errorText || jobItem.jobDetails.text
        visible: text !== ""
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: units.smallSpacing

        PlasmaComponents.ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            minimumValue: 0
            maximumValue: 100
            // TODO do we actually need the window visible check? perhaps I do because it can be in popup or expanded plasmoid
            indeterminate: visible && Window.window && Window.window.visible && percentage < 1
                           && jobItem.jobState === NotificationManager.Notifications.JobStateRunning
                           // is this too annoying?
                           && (jobItem.jobDetails.processedBytes === 0 || jobItem.jobDetails.totalBytes === 0)
                           && jobItem.jobDetails.processedFiles === 0
                           //&& jobItem.jobDetails.processedDirectories === 0
        }

        RowLayout {
            id: jobActionsRow
            spacing: 0

            PlasmaComponents.ToolButton {
                id: suspendButton
                tooltip: i18nc("Pause running job", "Pause")
                iconSource: "media-playback-pause"
                onClicked: jobItem.jobState === NotificationManager.Notifications.JobStateSuspended ? jobItem.resumeJobClicked()
                                                                                                    : jobItem.suspendJobClicked()
            }

            PlasmaComponents.ToolButton {
                id: killButton
                tooltip: i18nc("Cancel running job", "Cancel")
                iconSource: "media-playback-stop"
                onClicked: jobItem.killJobClicked()
            }

            PlasmaComponents.ToolButton {
                id: expandButton
                Layout.leftMargin: units.smallSpacing
                iconSource: checked ? "arrow-down" : (LayoutMirroring.enabled ? "arrow-left" : "arrow-right")
                tooltip: checked ? i18nc("A button tooltip; hides item details", "Hide Details")
                                 : i18nc("A button tooltip; expands the item to show details", "Show Details")
                checkable: true
            }
        }
    }

    Loader {
        Layout.fillWidth: true
        active: expandButton.checked
        // Loader doesn't reset its height when unloaded, just hide it altogether
        visible: active
        sourceComponent: JobDetails {
            jobDetails: jobItem.jobDetails
        }
    }

    Flow { // it's a Flow so it can wrap if too long
        id: jobDoneActions
        Layout.fillWidth: true
        spacing: units.smallSpacing
        // We want the actions to be right-aligned but Flow also reverses
        // the order of items, so we put them in reverse order
        layoutDirection: Qt.RightToLeft
        visible: url && url.toString() !== ""

        property var url: {
            if (jobItem.jobState !== NotificationManager.Notifications.JobStateStopped
                    || jobItem.error
                    || jobItem.jobDetails.totalFiles <= 0) {
                return null;
            }

            // For a single file show actions for it
            if (jobItem.jobDetails.totalFiles === 1) {
                return jobItem.jobDetails.descriptionUrl;
            } else {
                return jobItem.jobDetails.destUrl;
            }
        }

        PlasmaComponents.Button {
            id: otherFileActionsButton
            iconName: "application-menu"
            tooltip: i18n("More Options...")
            onClicked: otherFileActionsMenu.open(-1, -1)

            Notifications.FileMenu {
                id: otherFileActionsMenu
                url: jobDoneActions.url
                visualParent: otherFileActionsButton
            }
        }

        PlasmaComponents.Button {
            // would be nice to have the file icon here?
            text: jobItem.jobDetails.totalFiles > 1 ? i18n("Open Containing Folder") : i18n("Open")
            onClicked: jobItem.openUrl(jobDoneActions.url)
            width: minimumWidth
        }
    }

    states: [
        State {
            when: jobItem.jobState === NotificationManager.Notifications.JobStateSuspended
            PropertyChanges {
                target: suspendButton
                tooltip: i18nc("Resume paused job", "Resume")
                iconSource: "media-playback-start"
            }
            PropertyChanges {
                target: progressBar
                enabled: false
            }
        },
        State {
            when: jobItem.jobState === NotificationManager.Notifications.JobStateStopped
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
