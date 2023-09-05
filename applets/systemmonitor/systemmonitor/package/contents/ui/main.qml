/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Window 2.12

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.20 as Kirigami

import org.kde.ksysguard.sensors 1.0 as Sensors


PlasmoidItem {
    id: root
    Plasmoid.backgroundHints: PlasmaCore.Types.DefaultBackground | PlasmaCore.Types.ConfigurableBackground

    switchWidth: Plasmoid.formFactor === PlasmaCore.Types.Planar
        ? -1
        : (fullRepresentationItem ? fullRepresentationItem.Layout.minimumWidth : Kirigami.Units.gridUnit * 8)
    switchHeight: Plasmoid.formFactor === PlasmaCore.Types.Planar
        ? -1
        : (fullRepresentationItem ? fullRepresentationItem.Layout.minimumHeight : Kirigami.Units.gridUnit * 12)

    preferredRepresentation: Plasmoid.formFactor === PlasmaCore.Types.Planar ? fullRepresentation : null

    Plasmoid.title: Plasmoid.faceController.title || i18n("System Monitor")
    toolTipSubText: totalSensor.sensorId ? i18nc("Sensor name: value", "%1: %2", totalSensor.name, totalSensor.formattedValue) : ""

    compactRepresentation: CompactRepresentation {
    }
    fullRepresentation: FullRepresentation {
    }

    Plasmoid.configurationRequired: Plasmoid.faceController.highPrioritySensorIds.length == 0 && Plasmoid.faceController.lowPrioritySensorIds.length == 0 && Plasmoid.faceController.totalSensors.length == 0

    Sensors.Sensor {
        id: totalSensor
        sensorId: Plasmoid.faceController.totalSensors[0] || ""
        updateRateLimit: Plasmoid.faceController.updateRateLimit
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.MiddleButton
        onClicked: Plasmoid.openSystemMonitor()
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: i18nc("@action", "Open System Monitor…")
            icon.name: "utilities-system-monitor"
            onTriggered: Plasmoid.openSystemMonitor()
        }
    ]
}
