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

import org.kde.quickcharts 1.0 as Charts

Item {
    Plasmoid.backgroundHints: PlasmaCore.Types.DefaultBackground | PlasmaCore.Types.ConfigurableBackground

    Plasmoid.switchWidth: Plasmoid.fullRepresentationItem ? Plasmoid.fullRepresentationItem.Layout.minimumWidth : units.gridUnit * 8
    Plasmoid.switchHeight: Plasmoid.fullRepresentationItem ? Plasmoid.fullRepresentationItem.Layout.minimumHeight : units.gridUnit * 12

    Plasmoid.title: plasmoid.nativeInterface.faceController.title || i18n("System Monitor")
    Plasmoid.toolTipSubText: ""

    Plasmoid.compactRepresentation: CompactRepresentation {
    }
    Plasmoid.fullRepresentation: FullRepresentation {
    }

    Plasmoid.configurationRequired: plasmoid.nativeInterface.faceController.sensorIds.length == 0 && plasmoid.nativeInterface.faceController.textOnlySensorIds.length == 0 && plasmoid.nativeInterface.faceController.totalSensor.length == 0 

    //FIXME: things in faces refer to this id in the global context, should probably be fixed
    Charts.ColorGradientSource {
        id: colorSource
        // originally ColorGenerator used Kirigami.Theme.highlightColor
        baseColor: theme.highlightColor
        itemCount: plasmoid.nativeInterface.faceController.sensorIds.length

        onItemCountChanged: generate()
        Component.onCompleted: generate()

        function generate() {
            var colors = colorSource.colors;
            var savedColors = plasmoid.nativeInterface.faceController.sensorColors;
            for (var i = 0; i < plasmoid.nativeInterface.faceController.sensorIds.length; ++i) {
                if (savedColors.length <= i) {
                    savedColors.push(colors[i]);
                } else {
                    // Use the darker trick to make Qt validate the scring as a valid color;
                    var currentColor = Qt.darker(savedColors[i], 1);
                    if (!currentColor) {
                        savedColors[i] = (colors[i]);
                    }
                }
            }
            plasmoid.nativeInterface.faceController.sensorColors = savedColors;
        }
    }
}
