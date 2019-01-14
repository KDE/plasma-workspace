/*
 *   Copyright 2015 Marco Martin <mart@kde.org>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0 as KQuickAddons

KQuickAddons.Plotter {
    id: plotter
    property string sensorName: model.friendlyName1

    Layout.fillWidth: true
    Layout.fillHeight: true
    horizontalGridLineCount: 0

    function cycle(color, degrees) {
        var min = Math.min(color.r, Math.min(color.g, color.b));
        var max = Math.max(color.r, Math.max(color.g, color.b));
        var c = max - min;
        var h;

        if (c == 0) {
            h = 0
        } else if (max == color.r) {
            h = ((color.g - color.b) / c) % 6;
        } else if (max == color.g) {
            h = ((color.b - color.r) / c) + 2;
        } else if (max == color.b) {
            h = ((color.r - color.g) / c) + 4;
        }
        var hue = (1/6) * h + (degrees/360);
        var saturation = c / (1 - Math.abs(2 * ((max+min)/2) - 1));
        var lightness = (max + min)/2;

        return Qt.hsla(hue, saturation, lightness, 1.0);
    }

    property string downloadColor: theme.highlightColor
    property string uploadColor: cycle(theme.highlightColor, -90)

    dataSets: [
        KQuickAddons.PlotData {
            color: downloadColor
        },
        KQuickAddons.PlotData {
            color: uploadColor
        }
    ]

    PlasmaComponents.Label {
        id: nameLabel
        anchors {
            left: parent.left
            top: parent.top
        }
    }

    PlasmaComponents.Label {
        id: speedLabel
        wrapMode: Text.WordWrap
        visible: plasmoid.formFactor != PlasmaCore.Types.Vertical
        anchors {
            right: parent.right
        }
    }

    Connections {
        target: model.dataSource
        onNewData: {
            if (sourceName.indexOf(decodeURIComponent(model.source1)) != 0 && sourceName.indexOf(decodeURIComponent(model.source2)) != 0) {
                return;
            }

            var data1 = model.dataSource.data[decodeURIComponent(model.source2)];
            var data2 = model.dataSource.data[decodeURIComponent(model.source1)];

            if (data1 === undefined || data1.value === undefined ||
                data2 === undefined || data2.value === undefined) {
                return;
            }

            plotter.addSample([data1.value, data2.value]);

            if (plasmoid.formFactor != PlasmaCore.Types.Vertical) {
                nameLabel.text = plotter.sensorName
                speedLabel.text = i18n("<font color='%1'>⬇</font> %2 | <font color='%3'>⬆</font> %4",
                                        downloadColor,
                                        formatData(data1),
                                        uploadColor,
                                        formatData(data2))
            } else {
                nameLabel.text = plotter.sensorName+ "\n" + formatData(data1) + "\n" + formatData(data2)
                speedLabel.text = ""
            }
        }
    }
}
