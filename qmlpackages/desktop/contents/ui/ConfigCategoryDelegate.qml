/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.core 0.1 as PlasmaCore

MouseArea {
    id: delegate

    anchors {
        left: parent.left
        right: parent.right
    }
//BEGIN properties
    width: childrenRect.width
    height: childrenRect.height
    property bool current: main.sourceComponent == dataSource[modelData].component
//END properties

//BEGIN model
    property list<QtObject> dataSource
//END model

//BEGIN connections
    onClicked: {
        if (delegate.current) {
            return
        } else {
            main.sourceComponent = dataSource[modelData].component
            root.restoreConfig()
        }
    }
    onCurrentChanged: {
        if (current) {
            categoriesView.currentItem = delegate
        }
    }
//END connections

//BEGIN UI components
    Column {
        anchors {
            left: parent.left
            right: parent.right
        }
        PlasmaCore.IconItem {
            anchors.horizontalCenter: parent.horizontalCenter
            width: theme.IconSizeHuge
            height: width
            source: dataSource[modelData].icon
        }
        PlasmaComponents.Label {
            anchors {
                left: parent.left
                right: parent.right
            }
            text: dataSource[modelData].name
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
//END UI components
}
