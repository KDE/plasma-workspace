/*
 *   Copyright 2019 Marco Martin <mart@kde.org>
 *   Copyright 2019 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2019 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
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
import QtQuick.Window 2.12

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0

import org.kde.ksysguard.sensors 1.0 as Sensors
import org.kde.ksysguard.faces 1.0 as Faces

import org.kde.quickcharts 1.0 as Charts

Item {
    Plasmoid.backgroundHints: PlasmaCore.Types.DefaultBackground | PlasmaCore.Types.ConfigurableBackground

    Plasmoid.switchWidth: Plasmoid.formFactor === PlasmaCore.Types.Planar
        ? -1
        : (Plasmoid.fullRepresentationItem ? Plasmoid.fullRepresentationItem.Layout.minimumWidth : units.gridUnit * 8)
    Plasmoid.switchHeight: Plasmoid.formFactor === PlasmaCore.Types.Planar
        ? -1
        : (Plasmoid.fullRepresentationItem ? Plasmoid.fullRepresentationItem.Layout.minimumHeight : units.gridUnit * 12)

    Plasmoid.preferredRepresentation: Plasmoid.formFactor === PlasmaCore.Types.Planar ? Plasmoid.fullRepresentation : null

    Plasmoid.title: plasmoid.nativeInterface.faceController.title || i18n("System Monitor")
    Plasmoid.toolTipSubText: ""

    Plasmoid.compactRepresentation: CompactRepresentation {
    }
    Plasmoid.fullRepresentation: FullRepresentation {
    }

    Plasmoid.configurationRequired: plasmoid.nativeInterface.faceController.highPrioritySensorIds.length == 0 && plasmoid.nativeInterface.faceController.lowPrioritySensorIds.length == 0 && plasmoid.nativeInterface.faceController.totalSensor.length == 0
}
