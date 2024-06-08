/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import org.kde.pipewire.monitor as Monitor

PlasmaComponents.Page {
    implicitWidth: root.switchWidth
    implicitHeight: root.switchHeight

    PlasmaComponents.ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth - (contentItem as ListView).leftMargin - (contentItem as ListView).rightMargin
        focus: true

        contentItem: ListView {
            id: cameraList

            focus: cameraList.count > 0
            model: !root.expanded || monitor.count <= 1 ? null : monitor

            delegate: PlasmaExtras.ExpandableListItem {
                id: item

                required property string display
                required property int state

                // TODO: switch to PlasmaExtras.ListItem once it has subtitle
                icon: switch (item.state) {
                case Monitor.NodeState.Running:
                    return "camera-on-symbolic";
                case Monitor.NodeState.Idle:
                    return "camera-ready-symbolic";
                default:
                    return "camera-off-symbolic";
                }
                title: item.display || i18nc("@title", "Camera")
                subtitle: switch (item.state) {
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
                    id: item
                    property string display
                    property int state
                    parent: cameraList
                    anchors.centerIn: parent
                    width: parent.width - (Kirigami.Units.gridUnit * 4)
                    iconName: Plasmoid.icon
                    text: {
                        if (monitor.count === 1 && item.state === Monitor.NodeState.Running && item.display) {
                            return i18nc("@info:status %1 camera name", "%1 is in use", item.display);
                        }
                        return root.toolTipSubText;
                    }
                }
            }
        }
    }
}