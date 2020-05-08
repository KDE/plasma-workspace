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
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as QQC2

import org.kde.kirigami 2.5 as Kirigami
import org.kde.kquickcontrols 2.0
import org.kde.kconfig 1.0 // for KAuthorized
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.newstuff 1.62 as NewStuff

import org.kde.ksysguard.sensors 1.0 as Sensors
import org.kde.ksysguard.faces 1.0 as Faces

QQC2.Control {
    id: root

    signal configurationChanged

    function saveConfig() {
        contentItem.saveConfig();
    }
    
    contentItem: plasmoid.nativeInterface.faceController.appearanceConfigUi
    
    Connections {
        target: contentItem
        onConfigurationChanged: root.configurationChanged()
    }
}
