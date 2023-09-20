/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import org.kde.pipewire.monitor as Monitor

PlasmaComponents.Page {
    implicitWidth: Kirigami.Units.gridUnit * 12
    implicitHeight: Kirigami.Units.gridUnit * 12

    PlasmaComponents.ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth - contentItem.leftMargin - contentItem.rightMargin
        focus: true

        contentItem: ListView {
            id: cameraList

            focus: cameraList.count > 0
            model: !root.expanded || monitor.count <= 1 ? null : monitor

            delegate: PlasmaExtras.ExpandableListItem {
                // TODO: switch to PlasmaExtras.ListItem once it has subtitle
                icon: switch (model.state) {
                case Monitor.NodeState.Running:
                    return "camera-on-symbolic";
                case Monitor.NodeState.Idle:
                    return "camera-ready-symbolic";
                default:
                    return "camera-off-symbolic";
                }
                title: model.display || i18nc("@title", "Camera")
                subtitle: switch (model.state) {
                case Monitor.NodeState.Running:
                    return i18nc("@info:status", "Active");
                case Monitor.NodeState.Idle:
                    return i18nc("@info:status", "Idle");
                case Monitor.NodeState.Error:
                    return i18nc("@info:status", "Error");
                case Monitor.NodeState.Creating:
                    return i18nc("@info:status", "Creating");
                case Monitor.NodeState.Suspended:
                    return i18nc("@info:status", "Suspended");
                default:
                    return i18nc("@info:status", "Unknown");
                }
            }

            Instantiator {
                active: cameraList.count === 0
                asynchronous: true
                model: monitor.count === 1 ? monitor : 1

                delegate: PlasmaExtras.PlaceholderMessage {
                    parent: cameraList
                    anchors.centerIn: parent
                    width: parent.width - (Kirigami.Units.gridUnit * 4)
                    iconName: Plasmoid.icon
                    text: {
                        if (monitor.count === 1 && model.state === Monitor.NodeState.Running && model.display) {
                            return i18nc("@info:status %1 camera name", "%1 is in use", model.display);
                        }
                        return root.toolTipSubText;
                    }
                }
            }
        }
    }
}