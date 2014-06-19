/***************************************************************************
 *   Copyright (C) 2014 by David Edmundson <davidedmundson@kde.org>        *
 *   Copyright (C) 2014 by Aleix Pol Gonzalez <aleixpol@blue-systems.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1

import org.kde.plasma.core 2.0 as PlasmaCore

PlasmaCore.ColorScope {
    colorGroup: PlasmaCore.Theme.ComplementaryColorGroup
    property alias main: mainView.sourceComponent
    property alias controls: controlsView.sourceComponent
    readonly property alias mainItem: mainView.item
    readonly property alias controlsItem: controlsView.item

    property bool canShutdown: false
    property bool canReboot: false

    Rectangle {
        color: PlasmaCore.ColorScope.backgroundColor
        opacity: 0.8
        anchors {
            fill: parent
        }
    }

    Loader {
        id: mainView
        anchors {
            margins: units.largeSpacing

            left: parent.left
            right: parent.right
            top: parent.top
            bottom: separator.top
        }
    }

    Rectangle {
        id: separator
        height: 1
        color: PlasmaCore.ColorScope.textColor
        width: parent.width
        opacity: 0.4
        anchors {
            margins: units.largeSpacing

            bottom: controlsView.top
        }
    }
    Loader {
        id: controlsView
        focus: true
        anchors {
            margins: units.largeSpacing

            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
