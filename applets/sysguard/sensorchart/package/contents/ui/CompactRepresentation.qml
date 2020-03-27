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

import QtQuick 2.9
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.quickcharts 1.0 as Charts
import org.kde.kirigami 2.8 as Kirigami

Item {
    Layout.fillWidth: chartFace.item ? chartFace.item.Layout.fillWidth : false
    Layout.fillHeight: chartFace.item ? chartFace.item.Layout.fillHeight : false

    Layout.minimumWidth: chartFace.item ? chartFace.item.Layout.minimumWidth : 0
    Layout.minimumHeight: chartFace.item ? chartFace.item.Layout.minimumHeight : 0

    Layout.preferredWidth: chartFace.item ? chartFace.item.Layout.preferredWidth : 0
    Layout.preferredHeight: chartFace.item ? chartFace.item.Layout.preferredHeight : 0

    Layout.maximumWidth: chartFace.item ? chartFace.item.Layout.maximumWidth : 0
    Layout.maximumHeight: chartFace.item ? chartFace.item.Layout.maximumHeight : 0

    Kirigami.Theme.textColor: PlasmaCore.ColorScope.textColor

    Charts.ArraySource {
        id: globalColorSource
        array: plasmoid.configuration.sensorColors
    }
    Loader {
        id: chartFace
        anchors.fill: parent
        source: plasmoid.nativeInterface.compactRepresentationPath
    }
    MouseArea {
        anchors.fill: parent
        onClicked: plasmoid.expanded = !plasmoid.expanded
    }
}
