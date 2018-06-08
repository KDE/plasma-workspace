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
    //FIXME: doesn't seem to properly fill otherwise
    Layout.preferredHeight: parent.height
    horizontalGridLineCount: 0

    dataSets: [
        KQuickAddons.PlotData {
            color: theme.highlightColor
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
            if (sourceName.indexOf(decodeURIComponent(model.source1)) != 0) {
                return;
            }

            var data1 = model.dataSource.data[decodeURIComponent(model.source1)];

            if (data1 === undefined || data1.value === undefined) {
                return;
            }

            plotter.addSample([data1.value]);

            if (plasmoid.formFactor != PlasmaCore.Types.Vertical) {
                nameLabel.text = plotter.sensorName
                speedLabel.text = formatData(data1)
            } else {
                nameLabel.text = plotter.sensorName+ "\n" + formatData(data1)
                speedLabel.text = ""
            }
        }
    }
}

