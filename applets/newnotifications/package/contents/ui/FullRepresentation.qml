/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
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
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.notificationmanager 1.0 as NotificationManager

ColumnLayout {
    Layout.preferredWidth: units.gridUnit * 18
    Layout.preferredHeight: units.gridUnit * 24
    Layout.fillHeight: plasmoid.formFactor === PlasmaCore.Types.Vertical

    RowLayout {
        Layout.fillWidth: true

        MouseArea {
            width: dndRow.width + units.gridUnit
            height: dndRow.height + units.gridUnit / 2
            onClicked: dndCheck.checked = !dndCheck.checked

            RowLayout {
                id: dndRow
                anchors.centerIn: parent

                PlasmaComponents.CheckBox {
                    id: dndCheck
                    tooltip: i18n("Do not disturb")
                }

                PlasmaCore.IconItem {
                    // FIXME proper icon
                    source: "face-quiet"
                    width: height
                    Layout.fillHeight: true
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        PlasmaComponents.ToolButton {
            iconName: "configure"
            tooltip: plasmoid.action("configure").text
            visible: plasmoid.action("configure").enabled
            onClicked: plasmoid.action("configure").trigger()
        }
    }

    RowLayout {
        Layout.fillWidth: true

        PlasmaExtras.DescriptiveLabel {
            Layout.fillWidth: true
            // TODO
            text: "Do not disturb mode is configured to automatically enable between 22:00 and 06:00."
            wrapMode: Text.WordWrap
            font: theme.smallestFont
        }
    }

    PlasmaCore.SvgItem {
        elementId: "horizontal-line"
        Layout.fillWidth: true
        Layout.preferredHeight: 2 // FIXME
        svg: PlasmaCore.Svg {
            id: lineSvg
            imagePath: "widgets/line"
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: units.gridUnit * 18
        Layout.preferredHeight: units.gridUnit * 20

        PlasmaExtras.Heading {
            width: parent.width
            level: 3
            opacity: 0.6
            visible: list.count === 0
            text: i18n("No missed notifications.")
        }

        PlasmaExtras.ScrollArea {
            anchors.fill: parent

            ListView {
                id: list
                model: historyModel

                remove: Transition {
                    ParallelAnimation {
                        NumberAnimation { property: "opacity"; to: 0; duration: units.longDuration }
                        NumberAnimation { property: "x"; to: list.width; duration: units.longDuration }
                    }
                }
                removeDisplaced: Transition {
                    PauseAnimation { duration: units.longDuration }
                    NumberAnimation { properties: "y"; duration:  units.longDuration }
                }

                delegate: Loader {
                    sourceComponent: model.isGroup ? groupDelegate : notificationDelegate

                    Component {
                        id: groupDelegate
                        NotificationHeader {
                            width: list.width

                            applicationName: model.applicationName
                            applicationIconSource: model.applicationIconName

                            time: model.updated || model.created
                        }

                    }

                    Component {
                        id: notificationDelegate
                        NotificationDelegate {
                            width: list.width

                            notificationType: model.type

                            applicationName: model.applicationName
                            applicatonIconSource: model.applicationIconName

                            time: model.updated || model.created

                            configurable: model.configurable

                            // FIXME make the dismiss button a undismiss button
                            dismissable: model.type === NotificationManager.Notifications.JobType
                                && model.jobState !== NotificationManager.Notifications.JobStateStopped
                            closable: model.type === NotificationManager.Notifications.NotificationType
                                || model.jobState === NotificationManager.Notifications.JobStateStopped

                            summary: model.summary
                            body: model.body || "" // TODO
                            icon: model.image || model.iconName

                            urls: model.urls || []

                            jobState: model.jobState || 0
                            percentage: model.percentage || 0
                            error: model.error || 0
                            errorText: model.errorText || ""
                            suspendable: !!model.suspendable
                            killable: !!model.killable
                            jobDetails: model.jobDetails || null

                            configureActionLabel: model.configureActionLabel || ""
                            actionNames: model.actionNames
                            actionLabels: model.actionLabels

                            onCloseClicked: historyModel.close(historyModel.index(index, 0))
                            onDismissClicked: model.dismissed = false
                            onConfigureClicked: historyModel.configure(historyModel.index(index, 0))

                            onActionInvoked: historyModel.invokeAction(historyModel.index(index, 0), actionName)
                            onOpenUrl: {
                                Qt.openUrlExternally(url);
                                //historyModel.close(historyModel.index(index, 0))
                            }

                            onSuspendJobClicked: historyModel.suspendJob(historyModel.index(index, 0))
                            onResumeJobClicked: historyModel.resumeJob(historyModel.index(index, 0))
                            onKillJobClicked: historyModel.killJob(historyModel.index(index, 0))

                            // FIXME
                            svg: lineSvg
                        }
                    }
                }
            }
        }
    }
}
