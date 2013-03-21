/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
//import org.kde.plasma.core 2.0 as PlasmaCore
//import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras


Rectangle {
    id: root
    color: "pink"
    width: 400
    height: 800

    ListView {
        anchors.fill: parent
        model: widgetExplorer.widgetsModel
        header: PlasmaExtras.Title { text: "Add Widgets" }
        delegate: Item {
            width: parent.width
            height: 48
            Text { text: "Applet: " + pluginName }

            MouseArea {
                anchors.fill: parent
                onClicked: widgetExplorer.addApplet(pluginName)
            }
        }
    }

    Component.onCompleted: {
        print("WidgetExplorer QML loaded");
        print(" found " + widgetExplorer.widgetsModel.count + " widgets");
    }
}
