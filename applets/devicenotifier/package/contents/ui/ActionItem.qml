/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2012 Jacopo De Simoi <wilderkde@gmail.com>
 *   Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>
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

MouseArea {
    id: area
    property string icon
    property alias label: actionText.text
    property string predicate

    height: row.height + 2 * row.y
    hoverEnabled: true

    onContainsMouseChanged: {
        area.ListView.view.currentIndex = (containsMouse ? index : -1)
    }

    onClicked: {
        var service = hpSource.serviceForSource(udi);
        var operation = service.operationDescription("invokeAction");
        operation.predicate = predicate;
        service.startOperationCall(operation);
        devicenotifier.expandedDevice = "";
        devicenotifier.currentIndex = -1;
    }

    RowLayout {
        id: row
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 2 * units.smallSpacing
        y: units.smallSpacing
        spacing: units.smallSpacing

        PlasmaCore.IconItem {
            source: area.icon
            width: units.iconSizes.smallMedium
            height: width
        }

        PlasmaComponents.Label {
            id: actionText
            Layout.fillWidth: true
            Layout.fillHeight: true
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
        }
    }
}
