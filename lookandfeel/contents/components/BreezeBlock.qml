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

Item {
    property alias main: view.sourceComponent
    property alias controls: controlsLayout.sourceComponent
    readonly property alias mainItem: view.item
    readonly property alias controlsItem: controlsLayout.item

    property bool canShutdown: false
    property bool canReboot: false

    Rectangle {
        color: theme.textColor
        opacity: 0.8
        anchors {
            fill: parent
        }
    }

    Loader {
        id: view
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
        color: theme.backgroundColor
        width: parent.width
        opacity: 0.4
        anchors {
            margins: units.largeSpacing

            bottom: controlsLayout.top
        }
    }
    Loader {
        id: controlsLayout
        focus: true
        anchors {
            margins: units.largeSpacing

            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
