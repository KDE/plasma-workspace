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
import QtQuick.Controls 2.2 as Controls

import org.kde.kirigami 2.5 as Kirigami
import org.kde.kquickcontrols 2.0


Loader {
    id: root

    signal configurationChanged

    function saveConfig() {
        if (item.saveConfig) {
            item.saveConfig()
        }
        for (var key in plasmoid.nativeInterface.faceController.faceConfiguration) {
            if (item.hasOwnProperty("cfg_" + key)) {
                plasmoid.nativeInterface.faceController.faceConfiguration[key] = item["cfg_"+key]
            }
        }
    }

    source: plasmoid.nativeInterface.configPath

    onItemChanged: {
        if (!item || !plasmoid.nativeInterface.faceController.faceConfiguration) {
            return;
        }

        for (var key in plasmoid.nativeInterface.faceController.faceConfiguration) {
            if (!item.hasOwnProperty("cfg_" + key)) {
                continue;
            }

            item["cfg_" + key] = plasmoid.nativeInterface.faceController.faceConfiguration[key];
            var changedSignal = item["cfg_" + key + "Changed"];
            if (changedSignal) {
                changedSignal.connect(root.configurationChanged);
            }
        }
    }
}
